#include "playtargets.h"
#include "core.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QClipboard>
#include <QBuffer>
#include <QDir>
#include <QDnsLookup>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHostAddress>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QPointer>
#include <QSet>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent>
#include <QtEndian>
#include <QtZlib/zlib.h>
#include <algorithm>
#include <bit>
#include <limits>

namespace moly {
namespace {
constexpr qsizetype MaxNbtBytes = 32 * 1024 * 1024;
constexpr qsizetype MaxCompressedBytes = 16 * 1024 * 1024;
constexpr int MaxNbtNodes = 250000;
constexpr int MaxTargets = 2000;
constexpr int MaxStatusBytes = 1024 * 1024;

QByteArray inflateNbt(const QByteArray &input) {
    if (input.size() > MaxCompressedBytes)
        throw Error("Minecraft metadata is too large to read safely.");
    if (input.size() < 2 || uchar(input[0]) != 0x1f || uchar(input[1]) != 0x8b)
        return input;
    z_stream stream{};
    if (inflateInit2(&stream, 15 + 16) != Z_OK)
        throw Error("Cannot initialize Minecraft metadata decompression.");
    struct End { z_stream *s; ~End() { inflateEnd(s); } } end{&stream};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
    stream.avail_in = uInt(input.size());
    QByteArray result, chunk(32768, Qt::Uninitialized);
    int status = Z_OK;
    while (status == Z_OK) {
        stream.next_out = reinterpret_cast<Bytef *>(chunk.data());
        stream.avail_out = uInt(chunk.size());
        status = inflate(&stream, Z_NO_FLUSH);
        const auto produced = chunk.size() - stream.avail_out;
        if (result.size() + produced > MaxNbtBytes)
            throw Error("Minecraft metadata exceeds the decompression limit.");
        result.append(chunk.constData(), produced);
    }
    if (status != Z_STREAM_END || stream.avail_in != 0)
        throw Error("Minecraft metadata is damaged or contains trailing compressed data.");
    return result;
}

class NbtReader {
public:
    explicit NbtReader(QByteArray data) : data_(std::move(data)) {}
    QVariantMap read() {
        if (data_.size() > MaxNbtBytes || byte() != 10)
            throw Error("Minecraft metadata must contain an NBT compound.");
        string();
        auto result = payload(10, 0).toMap();
        if (pos_ != data_.size())
            throw Error("Minecraft metadata contains trailing data.");
        return result;
    }
private:
    QByteArray data_;
    qsizetype pos_ = 0;
    int remaining_ = MaxNbtNodes;
    void need(qsizetype n) {
        if (n < 0 || n > data_.size() - pos_)
            throw Error("Minecraft metadata is truncated.");
    }
    quint8 byte() { need(1); return quint8(data_[pos_++]); }
    template<class T> T integer() {
        need(sizeof(T)); const auto value = qFromBigEndian<T>(data_.constData() + pos_);
        pos_ += sizeof(T); return value;
    }
    QString string() {
        const auto n = integer<quint16>(); need(n);
        const auto value = QString::fromUtf8(data_.constData() + pos_, n); pos_ += n;
        return value;
    }
    int count(int width = 1) {
        const auto n = integer<qint32>();
        if (n < 0 || n > remaining_ || qint64(n) * width > data_.size() - pos_)
            throw Error("Minecraft metadata contains an excessive or invalid array.");
        remaining_ -= n;
        return n;
    }
    QVariant payload(int type, int depth) {
        if (depth > 48 || --remaining_ < 0)
            throw Error("Minecraft metadata is nested too deeply or contains too many values.");
        switch (type) {
        case 1: return qint8(byte());
        case 2: return integer<qint16>();
        case 3: return integer<qint32>();
        case 4: return integer<qint64>();
        case 5: return std::bit_cast<float>(integer<quint32>());
        case 6: return std::bit_cast<double>(integer<quint64>());
        case 7: { const auto n=count(); need(n); auto v=data_.mid(pos_,n); pos_+=n; return v; }
        case 8: return string();
        case 9: {
            const auto subtype=byte(); const auto n=count();
            if (subtype > 12 || (subtype == 0 && n != 0))
                throw Error("Minecraft metadata has an invalid list type.");
            QVariantList values; values.reserve(n);
            for(int i=0;i<n;i++) values.append(payload(subtype,depth+1));
            return values;
        }
        case 10: {
            QVariantMap values;
            while (true) {
                const auto child=byte(); if(child==0)break;
                const auto name=string();
                if(values.contains(name))throw Error("Minecraft metadata contains duplicate names.");
                values.insert(name,payload(child,depth+1));
            }
            return values;
        }
        case 11: { const auto n=count(4); QVariantList values; values.reserve(n); for(int i=0;i<n;i++)values.append(integer<qint32>()); return values; }
        case 12: { const auto n=count(8); QVariantList values; values.reserve(n); for(int i=0;i<n;i++)values.append(integer<qint64>()); return values; }
        default: throw Error("Minecraft metadata contains an unsupported NBT tag.");
        }
    }
};

QString plain(QString text) {
    text.remove(QRegularExpression(QStringLiteral("\u00a7.")));
    text.remove(QChar(0));
    return text.left(512);
}

QString fileImage(const QString &path) {
    QFileInfo info(path);
    if (!info.isFile() || info.isSymLink() || info.size() > 1024 * 1024) return {};
    QImageReader reader(path);
    const auto size=reader.size();
    if(size.isEmpty() || size.width()>1024 || size.height()>1024 || !reader.canRead())return {};
    auto url=QUrl::fromLocalFile(info.absoluteFilePath());
    url.setQuery("v="+QString::number(info.lastModified().toMSecsSinceEpoch()));
    return url.toString();
}

QString serverImage(const QString &value) {
    constexpr auto prefix="data:image/png;base64,";
    if(value.isEmpty() || value.size()>128*1024)return {};
    // servers.dat stores plain base64, while the status protocol uses a data URI.
    const auto encoded=value.startsWith(QLatin1String(prefix))?value.mid(qstrlen(prefix)):value;
    auto bytes=QByteArray::fromBase64(encoded.toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
    if(bytes.isEmpty() || bytes.size()>64*1024)return {};
    QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");const auto size=reader.size();
    if(size.width()<1||size.height()<1||size.width()>256||size.height()>256||reader.read().isNull())return {};
    return QLatin1String(prefix)+QString::fromLatin1(bytes.toBase64());
}
QString serverIconPath(const QString &root,const QString &address) {
    const auto key=QCryptographicHash::hash(address.trimmed().toLower().toUtf8(),QCryptographicHash::Sha256).toHex();
    return root+"/cache/server-icons/"+QString::fromLatin1(key)+".png";
}

QByteArray varInt(quint32 value) {
    QByteArray out;
    do { auto byte=quint8(value&127); value>>=7; if(value)byte|=128;out.append(char(byte)); }while(value);
    return out;
}
QByteArray packet(const QByteArray &body) {return varInt(quint32(body.size()))+body;}
// 0 = incomplete, 1 = complete, -1 = invalid. Neither malformed nor truncated
// packets are allowed to trigger unbounded allocations.
int getVarInt(const QByteArray &bytes,qsizetype &pos,quint32 &value) {
    auto at=pos;value=0;
    for(int i=0;i<5;i++) {
        if(at>=bytes.size())return 0;
        const auto b=quint8(bytes[at++]);
        if(i==4 && (b&0xf0))return -1;
        value|=quint32(b&127)<<(7*i);
        if(!(b&128)){pos=at;return 1;}
    }
    return -1;
}
QString chatText(const QJsonValue &value,int depth=0) {
    if(depth>8)return {};
    if(value.isString())return plain(value.toString());
    QString out;
    if(value.isArray())for(auto item:value.toArray()) {out+=chatText(item,depth+1);if(out.size()>512)break;}
    else if(value.isObject()) {
        const auto o=value.toObject();out=plain(o["text"].toString());
        for(auto item:o["extra"].toArray()){out+=chatText(item,depth+1);if(out.size()>512)break;}
    }
    return out.left(512);
}

class StatusProbe : public QObject {
public:
    StatusProbe(QString address,std::function<void(QVariantMap)> done,QObject *parent)
        : QObject(parent),address_(std::move(address)),done_(std::move(done)),socket_(this),timeout_(this),lookup_(this) {
        socket_.setReadBufferSize(MaxStatusBytes + 16);
        timeout_.setSingleShot(true);timeout_.setInterval(5000);
        connect(&timeout_,&QTimer::timeout,this,[this]{finish(false,"No response");});
        connect(&socket_,&QTcpSocket::connected,this,[this]{connected();});
        connect(&socket_,&QTcpSocket::readyRead,this,[this]{read();});
        connect(&socket_,&QTcpSocket::errorOccurred,this,[this](QAbstractSocket::SocketError){if(!finished_)finish(statusReceived_,"Cannot connect");});
        connect(&socket_,&QTcpSocket::disconnected,this,[this]{if(!finished_)finish(statusReceived_,"Connection closed");});
        connect(&lookup_,&QDnsLookup::finished,this,[this]{
            if(finished_)return;
            auto records=lookup_.serviceRecords();
            std::sort(records.begin(),records.end(),[](const auto&a,const auto&b){return a.priority()<b.priority();});
            if(lookup_.error()==QDnsLookup::NoError&&!records.isEmpty()) {
                host_=records.first().target();if(host_.endsWith('.'))host_.chop(1);
                if(host_.isEmpty()){finish(false,"Server unavailable");return;}
                port_=records.first().port();
            }
            socket_.connectToHost(host_,port_);
        });
        QTimer::singleShot(0,this,[this]{start();});
    }
private:
    QString address_,host_;quint16 port_=25565;
    std::function<void(QVariantMap)> done_;
    QTcpSocket socket_;QTimer timeout_;QDnsLookup lookup_;
    QElapsedTimer latency_;QByteArray buffer_;
    QVariantMap result_;
    bool finished_=false,statusReceived_=false,pongReceived_=false;
    static constexpr quint64 PingPayload=0x4d4f4c5950494e47ULL;
    void start() {
        const auto parsed=parseMinecraftAddress(address_);
        if(parsed.isEmpty()){finish(false,"Invalid address");return;}
        host_=parsed["host"].toString();port_=quint16(parsed["port"].toInt());timeout_.start();
        QHostAddress ip;
        if(!parsed["explicitPort"].toBool()&&!ip.setAddress(host_)&&host_.contains('.')) {
            lookup_.setType(QDnsLookup::SRV);lookup_.setName("_minecraft._tcp."+host_);lookup_.lookup();
        } else socket_.connectToHost(host_,port_);
    }
    void connected() {
        const auto host=host_.toUtf8();
        QByteArray hello=varInt(0)+varInt(quint32(-1))+varInt(quint32(host.size()))+host;
        hello.append(char(port_>>8));hello.append(char(port_&255));hello+=varInt(1);
        QByteArray ping=varInt(1);char payload[8];qToBigEndian<quint64>(PingPayload,payload);ping.append(payload,8);
        latency_.start();socket_.write(packet(hello)+packet(varInt(0))+packet(ping));
    }
    void read() {
        if(finished_)return;
        if(socket_.bytesAvailable()+buffer_.size()>MaxStatusBytes){finish(false,"Oversized response");return;}
        buffer_+=socket_.readAll();
        while(!buffer_.isEmpty()) {
            qsizetype cursor=0;quint32 length=0;
            auto state=getVarInt(buffer_,cursor,length);
            if(state==0)return;
            if(state<0 || length==0 || length>MaxStatusBytes){finish(false,"Invalid response");return;}
            if(length>quint32(buffer_.size()-cursor))return;
            auto body=buffer_.mid(cursor,length);buffer_.remove(0,cursor+length);
            cursor=0;quint32 id=0;if(getVarInt(body,cursor,id)!=1){finish(false,"Invalid packet");return;}
            if(id==0&&!statusReceived_) {
                quint32 textLength=0;
                if(getVarInt(body,cursor,textLength)!=1||textLength>32767||textLength!=quint32(body.size()-cursor)){finish(false,"Invalid status");return;}
                QJsonParseError error;const auto json=QJsonDocument::fromJson(body.mid(cursor),&error);
                if(error.error!=QJsonParseError::NoError||!json.isObject()){finish(false,"Invalid status JSON");return;}
                const auto object=json.object(),players=object["players"].toObject();
                const auto online=players["online"].toInt(-1),maximum=players["max"].toInt(-1);
                if(online<0||maximum<0||online>10000000||maximum>10000000){finish(false,"Invalid player count");return;}
                statusReceived_=true;
                result_["onlinePlayers"]=online;result_["maxPlayers"]=maximum;
                result_["motd"]=chatText(object["description"]);
                result_["serverVersion"]=plain(object["version"].toObject()["name"].toString());
                const auto icon=serverImage(object["favicon"].toString());if(!icon.isEmpty())result_["iconUrl"]=icon;
                result_["latencyMs"]=int(latency_.elapsed());
            } else if(id==1&&!pongReceived_) {
                if(body.size()-cursor!=8||qFromBigEndian<quint64>(body.constData()+cursor)!=PingPayload){finish(false,"Invalid ping response");return;}
                pongReceived_=true;result_["latencyMs"]=int(latency_.elapsed());
            } else {finish(false,"Unexpected packet");return;}
            if(statusReceived_&&pongReceived_){finish(true,{});return;}
        }
    }
    void finish(bool online,const QString &reason) {
        if(finished_)return;finished_=true;timeout_.stop();lookup_.abort();socket_.abort();
        result_["status"]=online?"online":"offline";
        result_["statusLabel"]=online?QString::number(result_["onlinePlayers"].toInt())+" / "+QString::number(result_["maxPlayers"].toInt())+" online":reason;
        if(!online){result_.remove("latencyMs");result_.remove("onlinePlayers");result_.remove("maxPlayers");}
        const auto done=std::move(done_);QPointer<StatusProbe> guard(this);if(done)done(result_);if(guard)deleteLater();
    }
};
}

QVariantMap readMinecraftNbt(const QByteArray &bytes) {return NbtReader(inflateNbt(bytes)).read();}
QVariantMap readMinecraftNbtFile(const QString &path) {
    QFile file(path);
    if(QFileInfo(file).isSymLink()||!file.open(QIODevice::ReadOnly)||file.size()>MaxCompressedBytes)
        throw Error("Cannot read Minecraft metadata safely.");
    return readMinecraftNbt(file.readAll());
}

QVariantMap parseMinecraftAddress(const QString &address) {
    const auto input=address.trimmed();
    if(input.isEmpty()||input.size()>255||input.contains(QRegularExpression("[\\s/\\\\?#@]")))return {};
    QString host;int port=25565;bool explicitPort=false;
    if(input.startsWith('[')) {
        const auto close=input.indexOf(']');if(close<2)return {};
        host=input.mid(1,close-1);
        QHostAddress ip;if(!ip.setAddress(host)||ip.protocol()!=QAbstractSocket::IPv6Protocol)return {};
        const auto suffix=input.mid(close+1);
        if(!suffix.isEmpty()){if(!suffix.startsWith(':'))return {};bool ok=false;port=suffix.mid(1).toInt(&ok);if(!ok)return {};explicitPort=true;}
    } else if(input.count(':')>1) {
        QHostAddress ip;if(!ip.setAddress(input)||ip.protocol()!=QAbstractSocket::IPv6Protocol)return {};host=input;
    } else {
        const auto colon=input.indexOf(':');host=colon<0?input:input.left(colon);
        if(colon>=0){bool ok=false;port=input.mid(colon+1).toInt(&ok);if(!ok)return {};explicitPort=true;}
        host=QString::fromLatin1(QUrl::toAce(host));
        if(host.isEmpty()||host.size()>253||!QRegularExpression("^[A-Za-z0-9_.-]+$").match(host).hasMatch())return {};
    }
    if(port<1||port>65535)return {};
    return {{"host",host},{"port",port},{"explicitPort",explicitPort}};
}

QVariantList savedPlayTargets(const QString &root,const QStringList &requestedIds) {
    QVariantList items;
    const auto instancesRoot=root+"/instances";
    auto ids=requestedIds;
    if(ids.isEmpty())ids=QDir(instancesRoot).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    if(ids.size()>256)ids=ids.mid(0,256);
    for(const auto &id:ids) {
        if(items.size()>=MaxTargets)break;
        try {
            const auto directory=safePath(instancesRoot,id);
            const auto instance=readObject(directory+"/instance.json");
            if(instance["id"].toString()!=id)continue;
            const auto game=safePath(directory,"game");
            auto common=[&]{return QVariantMap{{"instanceId",id},{"instanceName",instance["name"].toString()},{"minecraftVersion",instance["version"].toString()},{"loader",instance["loader"].toString()}};};
            const auto saves=safePath(game,"saves");
            auto worlds=QDir(saves).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Time);
            for(const auto &world:worlds) {
                if(items.size()>=MaxTargets)break;
                if(world.isSymLink())continue;
                const auto path=safePath(saves,world.fileName());
                if(!QFileInfo::exists(path+"/level.dat"))continue;
                auto row=common();row["type"]="world";row["value"]=world.fileName();row["folder"]=world.fileName();
                row["id"]=id+"/world/"+world.fileName();row["name"]=world.fileName();row["status"]="local";row["statusLabel"]="Singleplayer";
                row["iconUrl"]=fileImage(path+"/icon.png");
                try {
                    const auto parsed=readMinecraftNbtFile(path+"/level.dat"),data=parsed.value("Data",parsed).toMap();
                    const auto name=plain(data["LevelName"].toString());if(!name.isEmpty())row["name"]=name;
                    const auto mode=data["GameType"].toInt();row["gameMode"]=QStringList{"Survival","Creative","Adventure","Spectator"}.value(mode,"Unknown mode");
                    row["lastPlayed"]=data["LastPlayed"].toLongLong();row["hardcore"]=data["hardcore"].toBool();
                    row["subtitle"]=row["gameMode"].toString();
                } catch(const std::exception &e){row["metadataError"]=QString::fromUtf8(e.what());row["subtitle"]="World metadata unavailable";}
                items.append(row);
            }
            if(items.size()>=MaxTargets)break;
            const auto servers=safePath(game,"servers.dat");
            if(!QFileInfo::exists(servers))continue;
            try {
                const auto entries=readMinecraftNbtFile(servers)["servers"].toList();
                QSet<QString> seen;
                for(const auto &entry:entries) {
                    if(items.size()>=MaxTargets)break;
                    const auto server=entry.toMap();const auto address=server["ip"].toString().trimmed();
                    if(parseMinecraftAddress(address).isEmpty()||seen.contains(address.toLower()))continue;
                    seen.insert(address.toLower());
                    auto row=common();row["type"]="server";row["value"]=address;row["address"]=address;
                    row["id"]=id+"/server/"+address;row["name"]=plain(server["name"].toString());
                    if(row["name"].toString().isEmpty())row["name"]=address;
                    row["subtitle"]=address;row["status"]="unknown";row["statusLabel"]="Not checked";
                    auto icon=serverImage(server["icon"].toString());if(icon.isEmpty())icon=fileImage(serverIconPath(root,address));
                    row["iconUrl"]=icon;items.append(row);
                }
            }catch(const std::exception &){ /* Damaged servers.dat must not hide saved worlds. */ }
        }catch(const std::exception &){ /* One unavailable instance must not hide the others. */ }
    }
    std::stable_sort(items.begin(),items.end(),[](const QVariant&a,const QVariant&b){return a.toMap().value("lastPlayed").toLongLong()>b.toMap().value("lastPlayed").toLongLong();});
    return items;
}

PlayTargets::PlayTargets(QString root,QObject *parent):QObject(parent),root_(std::move(root)) {}
PlayTargets::~PlayTargets(){
    cancelPings();
    for(auto *watcher:findChildren<QFutureWatcherBase*>())watcher->waitForFinished();
}
QVariantList PlayTargets::items() const {
    if(selected_.isEmpty())return allItems_;
    QVariantList out;for(const auto &v:allItems_)if(v.toMap()["instanceId"]==selected_)out.append(v);return out;
}
void PlayTargets::setDataRoot(const QString &root){if(root_==root)return;root_=root;emit configurationChanged();refresh();}
void PlayTargets::setSelectedInstanceId(const QString &id){if(selected_==id)return;selected_=id;emit configurationChanged();emit itemsChanged();}
void PlayTargets::setInstanceIds(const QStringList &ids){if(instanceIds_==ids)return;instanceIds_=ids;emit configurationChanged();refresh();}
void PlayTargets::setPingEnabled(bool enabled){if(pingEnabled_==enabled)return;pingEnabled_=enabled;emit configurationChanged();if(enabled)refreshStatus();else{cancelPings();for(auto &v:allItems_){auto row=v.toMap();if(row["type"]=="server"){row["status"]="unknown";row["statusLabel"]="Status checks off";row.remove("latencyMs");v=row;}}emit itemsChanged();}}
void PlayTargets::cancelPings(){++pingGeneration_;if(pingGroup_){delete pingGroup_;pingGroup_=nullptr;}pendingPings_=0;emit checkingStatusChanged();}
void PlayTargets::refresh(){
    const auto ticket=++generation_;cancelPings();loading_=true;error_.clear();emit loadingChanged();emit errorChanged();
    auto watcher=new QFutureWatcher<QVariantList>(this);const auto root=root_;const auto ids=instanceIds_;
    connect(watcher,&QFutureWatcher<QVariantList>::finished,this,[this,watcher,ticket]{
        if(ticket!=generation_){watcher->deleteLater();return;}
        try{allItems_=watcher->result();for(auto &value:allItems_){auto row=value.toMap();const auto old=serverSnapshots_.value(row["address"].toString());if(row["type"]=="server"){if(row["iconUrl"].toString().isEmpty())row["iconUrl"]=old["iconUrl"];row["motd"]=old["motd"];}value=row;}}catch(const std::exception &e){error_=QString::fromUtf8(e.what());allItems_.clear();emit errorChanged();emit errorOccurred(error_);}
        loading_=false;emit loadingChanged();emit itemsChanged();watcher->deleteLater();if(pingEnabled_)refreshStatus();
    });
    watcher->setFuture(QtConcurrent::run([root,ids]{return savedPlayTargets(root,ids);}));
}
void PlayTargets::refreshStatus(){
    cancelPings();if(!pingEnabled_)return;
    const auto ticket=pingGeneration_;pingGroup_=new QObject(this);
    QMap<QString,QStringList> addresses;
    for(auto &v:allItems_){auto row=v.toMap();if(row["type"]!="server")continue;const auto address=row["address"].toString();if(addresses.size()>=128&&!addresses.contains(address))continue;addresses[address].append(row["id"].toString());row["status"]="checking";row["statusLabel"]="Checking…";v=row;}
    pendingPings_=addresses.size();emit itemsChanged();emit checkingStatusChanged();
    // Keep at most eight connections active. Each completion takes the next
    // saved address from the queue, rather than starting a timer per server.
    using Pending=QPair<QString,QStringList>;
    auto queue=std::make_shared<QList<Pending>>();
    for(auto it=addresses.cbegin();it!=addresses.cend();++it)queue->append({it.key(),it.value()});
    auto advance=std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> weakAdvance=advance;
    *advance=[this,queue,ticket,weakAdvance]{
        if(ticket!=pingGeneration_||queue->isEmpty())return;
        const auto pending=queue->takeFirst();const auto ids=pending.second;
        auto keepAlive=weakAdvance.lock();
        new StatusProbe(pending.first,[this,ids,ticket,keepAlive,address=pending.first](QVariantMap status){
            if(ticket!=pingGeneration_)return;
            rememberServer(address,status);
            for(auto &v:allItems_){auto row=v.toMap();if(!ids.contains(row["id"].toString()))continue;for(auto it=status.cbegin();it!=status.cend();++it)row[it.key()]=it.value();v=row;}
            pendingPings_=qMax(0,pendingPings_-1);emit itemsChanged();emit checkingStatusChanged();
            if(keepAlive)(*keepAlive)();
        },pingGroup_);
    };
    for(int i=0;i<8&&!queue->isEmpty();++i)(*advance)();
}
void PlayTargets::rememberServer(const QString &address,const QVariantMap &status) {
    auto saved=serverSnapshots_.value(address);
    for(const auto &key:QStringList{"iconUrl","motd","serverVersion"})if(status.contains(key)&&!status[key].toString().isEmpty())saved[key]=status[key];
    serverSnapshots_[address]=saved;
    const auto icon=serverImage(status.value("iconUrl").toString());if(icon.isEmpty())return;
    const auto bytes=QByteArray::fromBase64(icon.mid(QString("data:image/png;base64,").size()).toLatin1());
    try {
        const auto path=serverIconPath(root_,address);safePath(root_,"cache/server-icons/"+QFileInfo(path).fileName());
        QFile old(path);if(old.open(QIODevice::ReadOnly)&&old.readAll()==bytes)return;
        writeBytes(path,bytes);
    }catch(const std::exception &){} // A cache failure must not hide the live server status.
}
void PlayTargets::refreshServer(const QString &address) {
    if(!pingEnabled_||checkingStatus()||parseMinecraftAddress(address).isEmpty())return;
    bool found=false;for(auto &value:allItems_){auto row=value.toMap();if(row["type"]=="server"&&row["address"]==address){row["status"]="checking";row["statusLabel"]="Checking…";value=row;found=true;}}
    if(!found)return;
    cancelPings();pingGroup_=new QObject(this);pendingPings_=1;const auto ticket=pingGeneration_;
    emit itemsChanged();emit checkingStatusChanged();
    new StatusProbe(address,[this,address,ticket](QVariantMap status){
        if(ticket!=pingGeneration_)return;rememberServer(address,status);
        for(auto &value:allItems_){auto row=value.toMap();if(row["type"]!="server"||row["address"]!=address)continue;for(auto it=status.cbegin();it!=status.cend();++it)row[it.key()]=it.value();value=row;}
        pendingPings_=0;emit itemsChanged();emit checkingStatusChanged();
    },pingGroup_);
}
bool PlayTargets::copyAddress(const QString &address) {
    if(parseMinecraftAddress(address).isEmpty()||!qobject_cast<QGuiApplication*>(QCoreApplication::instance()))return false;
    QGuiApplication::clipboard()->setText(address);emit notice("Server address copied.");return true;
}
bool PlayTargets::setWorldIcon(const QString &instanceId,const QString &worldFolder,const QString &imageUrl){
    try{
        const auto instance=safePath(root_+"/instances",instanceId);const auto descriptor=readObject(instance+"/instance.json");
        if(descriptor["id"]!=instanceId)throw Error("The world does not belong to a valid instance.");
        if(worldFolder.contains('/')||worldFolder.contains('\\'))throw Error("Choose a world folder from this instance.");
        const auto world=safePath(instance+"/game/saves",worldFolder);
        if(!QFileInfo::exists(world+"/level.dat"))throw Error("That saved world could not be found.");
        safePath(world,"icon.png");
        const QUrl url(imageUrl);if(!url.isLocalFile())throw Error("Choose an image file on this computer.");
        QFileInfo source(url.toLocalFile());if(!source.isFile()||source.isSymLink()||source.size()>16*1024*1024)throw Error("Choose an image smaller than 16 MB.");
        QImageReader reader(source.absoluteFilePath());const auto size=reader.size();if(size.isEmpty()||size.width()>8192||size.height()>8192)throw Error("This image is too large or unsupported.");
        auto image=reader.read();if(image.isNull())throw Error("This image could not be opened.");
        image=image.scaled(64,64,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);image=image.copy((image.width()-64)/2,(image.height()-64)/2,64,64);
        const auto path=world+"/icon.png";QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);if(!image.save(&buffer,"PNG"))throw Error("The world icon could not be saved.");writeBytes(path,bytes);refresh();return true;
    }catch(const std::exception &e){error_=QString::fromUtf8(e.what());emit errorChanged();emit errorOccurred(error_);return false;}
}
void PlayTargets::activate(const QVariantMap &entry){
    const auto id=entry["id"].toString();for(const auto &item:allItems_){const auto row=item.toMap();if(row["id"]==id){emit targetChosen(row["instanceId"].toString(),row["type"].toString(),row["value"].toString());return;}}
}
}
