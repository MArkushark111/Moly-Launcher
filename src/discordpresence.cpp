#include "discordpresence.h"
#include "playtargets.h"
#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <QtEndian>

namespace moly {
QString publicPresenceServer(const QString &address) {
    const auto parsed=parseMinecraftAddress(address);
    if(parsed.isEmpty())return "Private server";
    const auto host=parsed["host"].toString().toLower();QHostAddress ip;
    if(ip.setAddress(host)||QRegularExpression("^[0-9.]+$").match(host).hasMatch()||host=="localhost"||!host.contains('.')||host.endsWith(".local")||host.endsWith(".lan")||host.endsWith(".internal")||host.endsWith(".localhost"))return "Private server";
    return host.left(110);
}
DiscordPresence::DiscordPresence(QString applicationId,QObject *parent,QString pipePrefix):QObject(parent),id_(std::move(applicationId)),pipePrefix_(std::move(pipePrefix)) {
    retry_.setSingleShot(true);retry_.setInterval(15000);
    throttle_.setSingleShot(true);throttle_.setInterval(1000);
    connect(&retry_,&QTimer::timeout,this,[this]{pipe_=0;connectPipe();});
    connect(&throttle_,&QTimer::timeout,this,&DiscordPresence::sendActivity);
    connect(&socket_,&QLocalSocket::connected,this,[this]{input_.clear();frame(0,QJsonDocument(QJsonObject{{"v",1},{"client_id",id_}}).toJson(QJsonDocument::Compact));});
    connect(&socket_,&QLocalSocket::readyRead,this,&DiscordPresence::read);
    connect(&socket_,&QLocalSocket::disconnected,this,[this]{ready_=false;lastActivity_.clear();if(enabled_){setStatus("Waiting for Discord");retry_.start();}});
    connect(&socket_,&QLocalSocket::errorOccurred,this,[this](QLocalSocket::LocalSocketError){ready_=false;if(!enabled_)return;if(++pipe_<10)QTimer::singleShot(0,this,&DiscordPresence::connectPipe);else{setStatus("Waiting for Discord");retry_.start();}});
}
DiscordPresence::~DiscordPresence(){if(ready_)frame(1,QJsonDocument(QJsonObject{{"cmd","SET_ACTIVITY"},{"args",QJsonObject{{"pid",double(QCoreApplication::applicationPid())},{"activity",QJsonValue::Null}}},{"nonce",QUuid::createUuid().toString()}}).toJson(QJsonDocument::Compact));socket_.flush();}
bool DiscordPresence::configured() const{return QRegularExpression("^[0-9]{16,22}$").match(id_).hasMatch();}
void DiscordPresence::setStatus(const QString &value){if(status_==value)return;status_=value;emit statusChanged();}
void DiscordPresence::configure(bool enabled,bool shareServer){shareServer_=shareServer;if(enabled_==enabled){update();return;}enabled_=enabled;if(!enabled){if(ready_){frame(1,QJsonDocument(QJsonObject{{"cmd","SET_ACTIVITY"},{"args",QJsonObject{{"pid",double(QCoreApplication::applicationPid())},{"activity",QJsonValue::Null}}},{"nonce",QUuid::createUuid().toString()}}).toJson(QJsonDocument::Compact));socket_.flush();}retry_.stop();throttle_.stop();socket_.abort();ready_=false;setStatus("Off");}else if(!configured())setStatus("Application registration needed");else{pipe_=0;connectPipe();}}
void DiscordPresence::setActivityProvider(std::function<QVariantMap(QString,bool)> provider){provider_=std::move(provider);update();}
void DiscordPresence::setPage(const QString &page){page_=page;update();}
void DiscordPresence::update(){if(enabled_&&ready_&&!throttle_.isActive())throttle_.start();}
void DiscordPresence::connectPipe(){if(!enabled_||!configured())return;retry_.stop();socket_.abort();setStatus("Connecting to Discord");socket_.connectToServer(pipePrefix_+QString::number(pipe_));}
void DiscordPresence::frame(quint32 opcode,const QByteArray &body){QByteArray data(8,Qt::Uninitialized);qToLittleEndian(opcode,data.data());qToLittleEndian(quint32(body.size()),data.data()+4);socket_.write(data+body);}
void DiscordPresence::read(){
    if(input_.size()+socket_.bytesAvailable()>1024*1024){socket_.abort();return;}
    input_+=socket_.readAll();
    while(input_.size()>=8){const auto opcode=qFromLittleEndian<quint32>(input_.constData()),size=qFromLittleEndian<quint32>(input_.constData()+4);if(size>1024*1024){socket_.abort();return;}if(input_.size()<8+size)return;const auto body=input_.mid(8,size);input_.remove(0,8+size);
        if(opcode==3){frame(4,body);continue;}if(opcode==2){socket_.abort();return;}if(opcode!=1)continue;
        const auto message=QJsonDocument::fromJson(body).object();if(message["evt"]=="READY"){ready_=true;lastActivity_.clear();setStatus("Connected");update();}else if(message["evt"]=="ERROR")setStatus("Discord rejected activity");
    }
}
void DiscordPresence::sendActivity(){if(!enabled_||!ready_||!provider_)return;const auto info=provider_(page_,shareServer_);QJsonObject activity{{"details",info["details"].toString().left(120)},{"type",0},{"state","Moly Launcher"}};
    const auto server=info["server"].toString();if(!server.isEmpty()) {
        const auto host=publicPresenceServer(server);auto name=info["serverName"].toString();name.remove(QRegularExpression("[\\x00-\\x1f]"));
        activity["state"]=host=="Private server"?QString("Private server"):name.isEmpty()?"Server · "+host:(name.left(50)+" · "+host).left(120);
    }
    const auto encoded=QJsonDocument(activity).toJson(QJsonDocument::Compact);if(encoded==lastActivity_)return;lastActivity_=encoded;
    frame(1,QJsonDocument(QJsonObject{{"cmd","SET_ACTIVITY"},{"args",QJsonObject{{"pid",double(QCoreApplication::applicationPid())},{"activity",activity}}},{"nonce",QUuid::createUuid().toString()}}).toJson(QJsonDocument::Compact));
}
}
