#include <QtTest>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QImage>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QtEndian>
#include <QtZlib/zlib.h>
#include "core.h"
#include "playtargets.h"

using namespace moly;
namespace {
QByteArray str(const QString &value) { auto text=value.toUtf8();QByteArray out(2,Qt::Uninitialized);qToBigEndian<quint16>(quint16(text.size()),out.data());return out+text; }
QByteArray tag(int type,const QString &name,const QByteArray &body){return QByteArray(1,char(type))+str(name)+body;}
QByteArray number(qint32 value){QByteArray out(4,Qt::Uninitialized);qToBigEndian<qint32>(value,out.data());return out;}
QByteArray longNumber(qint64 value){QByteArray out(8,Qt::Uninitialized);qToBigEndian<qint64>(value,out.data());return out;}
QByteArray worldNbt(){return QByteArray("\x0a\0\0",3)+tag(10,"Data",tag(8,"LevelName",str("Test world"))+tag(3,"GameType",number(1))+tag(4,"LastPlayed",longNumber(1750000000000))+QByteArray(1,'\0'))+QByteArray(1,'\0');}
QByteArray serverNbt(const QString &address,const QString &icon={}){return QByteArray("\x0a\0\0",3)+tag(9,"servers",QByteArray(1,char(10))+number(1)+tag(8,"name",str("Test server"))+tag(8,"ip",str(address))+(icon.isEmpty()?QByteArray():tag(8,"icon",str(icon)))+QByteArray(1,'\0'))+QByteArray(1,'\0');}
QString pngIcon(){QImage image(64,64,QImage::Format_ARGB32);image.fill(Qt::green);QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");return "data:image/png;base64,"+QString::fromLatin1(bytes.toBase64());}
QByteArray gzip(const QByteArray &input){z_stream stream{};if(deflateInit2(&stream,6,Z_DEFLATED,31,8,Z_DEFAULT_STRATEGY)!=Z_OK)return {};QByteArray out(1024+input.size()*2,Qt::Uninitialized);stream.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));stream.avail_in=uInt(input.size());stream.next_out=reinterpret_cast<Bytef*>(out.data());stream.avail_out=uInt(out.size());const auto state=deflate(&stream,Z_FINISH);out.resize(qsizetype(stream.total_out));deflateEnd(&stream);return state==Z_STREAM_END?out:QByteArray{};}
void makeInstance(const QString &root){writeObject(root+"/instances/one/instance.json",{{"id","one"},{"name","First setup"},{"version","1.20.1"},{"loader","Fabric"}});}
QByteArray vi(quint32 v){QByteArray out;do{auto b=quint8(v&127);v>>=7;if(v)b|=128;out.append(char(b));}while(v);return out;}
QByteArray frame(const QByteArray &body){return vi(quint32(body.size()))+body;}
bool take(QByteArray &buffer,QByteArray &out){quint32 length=0;int count=0;for(;count<5;count++){if(count>=buffer.size())return false;const auto b=quint8(buffer[count]);length|=quint32(b&127)<<(count*7);if(!(b&128)){++count;break;}}if(length>quint32(buffer.size()-count))return false;out=buffer.mid(count,length);buffer.remove(0,count+length);return true;}
}

class PlayTargetTests:public QObject {
    Q_OBJECT
private slots:
    void nbtUncompressedAndGzip(){auto plain=readMinecraftNbt(worldNbt());QCOMPARE(plain["Data"].toMap()["LevelName"].toString(),QString("Test world"));QCOMPARE(readMinecraftNbt(gzip(worldNbt())),plain);QCOMPARE(plain["Data"].toMap()["LastPlayed"].toLongLong(),qint64(1750000000000));}
    void malformedNbtIsBounded(){QVERIFY_EXCEPTION_THROWN(readMinecraftNbt(QByteArray("garbage")),Error);auto bytes=worldNbt();bytes.chop(1);QVERIFY_EXCEPTION_THROWN(readMinecraftNbt(bytes),Error);bytes=QByteArray("\x0a\0\0",3)+tag(7,"huge",number(std::numeric_limits<qint32>::max()));QVERIFY_EXCEPTION_THROWN(readMinecraftNbt(bytes),Error);auto compressed=gzip(worldNbt());compressed[compressed.size()-1]=char(quint8(compressed.back())^128);QVERIFY_EXCEPTION_THROWN(readMinecraftNbt(compressed),Error);}
    void addressValidation(){QCOMPARE(parseMinecraftAddress("example.com")["port"].toInt(),25565);QCOMPARE(parseMinecraftAddress("localhost:25600")["port"].toInt(),25600);QCOMPARE(parseMinecraftAddress("[::1]:25566")["host"].toString(),QString("::1"));for(const auto &bad:QStringList{"https://example.com","example.com:0","example.com:99999","host/path","[bad]","host:abc","name with spaces"})QVERIFY2(parseMinecraftAddress(bad).isEmpty(),qPrintable(bad));}
    void scanWorldsAndServers(){QTemporaryDir root;makeInstance(root.path());writeBytes(root.path()+"/instances/one/game/saves/demo/level.dat",gzip(worldNbt()));writeBytes(root.path()+"/instances/one/game/servers.dat",serverNbt("localhost:25565"));auto rows=savedPlayTargets(root.path());QCOMPARE(rows.size(),2);QCOMPARE(rows[0].toMap()["type"].toString(),QString("world"));QCOMPARE(rows[0].toMap()["name"].toString(),QString("Test world"));QCOMPARE(rows[0].toMap()["gameMode"].toString(),QString("Creative"));QCOMPARE(rows[1].toMap()["address"].toString(),QString("localhost:25565"));QVERIFY(savedPlayTargets(root.path(),{"missing"}).isEmpty());}
    void customWorldIconAndUnsafePaths(){QTemporaryDir root;makeInstance(root.path());writeBytes(root.path()+"/instances/one/game/saves/demo/level.dat",worldNbt());QImage source(200,100,QImage::Format_ARGB32);source.fill(Qt::red);const auto path=root.path()+"/source.png";QVERIFY(source.save(path));PlayTargets targets(root.path());targets.setPingEnabled(false);QVERIFY(!targets.setWorldIcon("one","../outside",QUrl::fromLocalFile(path).toString()));QVERIFY(!targets.setWorldIcon("one","demo","https://example.com/image.png"));QVERIFY(targets.setWorldIcon("one","demo",QUrl::fromLocalFile(path).toString()));QImage result(root.path()+"/instances/one/game/saves/demo/icon.png");QCOMPARE(result.size(),QSize(64,64));}
    void savedServerIconsAcceptMinecraftBase64(){QTemporaryDir root;makeInstance(root.path());const auto uri=pngIcon();writeBytes(root.path()+"/instances/one/game/servers.dat",serverNbt("localhost:25565",uri.mid(QString("data:image/png;base64,").size())));const auto rows=savedPlayTargets(root.path());QCOMPARE(rows.size(),1);QCOMPARE(rows.first().toMap()["iconUrl"].toString(),uri);}
    void localServerStatusIsRealAndAsync(){const auto favicon=pngIcon();QTemporaryDir root;makeInstance(root.path());QTcpServer server;QVERIFY(server.listen(QHostAddress::LocalHost,0));writeBytes(root.path()+"/instances/one/game/servers.dat",serverNbt("127.0.0.1:"+QString::number(server.serverPort())));connect(&server,&QTcpServer::newConnection,&server,[&]{auto socket=server.nextPendingConnection();auto incoming=std::make_shared<QByteArray>();auto handshake=std::make_shared<bool>(false);connect(socket,&QTcpSocket::readyRead,socket,[socket,incoming,handshake,favicon]{*incoming+=socket->readAll();QByteArray body;while(take(*incoming,body)){if(!*handshake){*handshake=true;continue;}if(body==QByteArray(1,'\0')){const auto json=QJsonDocument(QJsonObject{{"favicon",favicon},{"version",QJsonObject{{"name","Test"},{"protocol",765}}},{"players",QJsonObject{{"online",7},{"max",20}}},{"description",QJsonObject{{"text","Local mock server"}}}}).toJson(QJsonDocument::Compact);socket->write(frame(QByteArray(1,'\0')+vi(quint32(json.size()))+json));}else if(!body.isEmpty()&&body[0]=='\1')socket->write(frame(body));}});});PlayTargets targets(root.path());targets.refresh();QTRY_COMPARE_WITH_TIMEOUT(targets.loading(),false,3000);QTRY_COMPARE_WITH_TIMEOUT(targets.items().size(),1,3000);QTRY_COMPARE_WITH_TIMEOUT(targets.items()[0].toMap()["status"].toString(),QString("online"),6000);const auto row=targets.items()[0].toMap();QCOMPARE(row["onlinePlayers"].toInt(),7);QCOMPARE(row["maxPlayers"].toInt(),20);QVERIFY(row["latencyMs"].toInt()>=0);QCOMPARE(row["motd"].toString(),QString("Local mock server"));QCOMPARE(row["iconUrl"].toString(),favicon);QCOMPARE(QDir(root.path()+"/cache/server-icons").entryList({"*.png"},QDir::Files).size(),1);QVERIFY(QUrl(savedPlayTargets(root.path()).first().toMap()["iconUrl"].toString()).isLocalFile());targets.setPingEnabled(false);QCOMPARE(targets.items()[0].toMap()["status"].toString(),QString("unknown"));}
};
QTEST_GUILESS_MAIN(PlayTargetTests)
#include "playtargets_tests.moc"
