#include <QtTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QUuid>
#include <QtEndian>
#include "discordpresence.h"
using namespace moly;
class DiscordTests : public QObject {
    Q_OBJECT
private slots:
    void privateAddressesNeverLeavePresence() {
        for(const auto &address:QStringList{"88.2.2.2","88.2.2.2.2","127.0.0.1:25565","[2001:db8::1]:25565","localhost","localhost:25565","my-pc","server.local","home.lan","foo.internal","https://nope.com"})QCOMPARE(publicPresenceServer(address),QString("Private server"));
        QCOMPARE(publicPresenceServer("Play.Example.com:25566"),QString("play.example.com"));
    }
    void ipcHandshakeActivityAndClear() {
        const auto prefix="moly-discord-test-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        QLocalServer server;QVERIFY(server.listen(prefix+"0"));QList<QJsonObject> messages;
        connect(&server,&QLocalServer::newConnection,this,[&]{auto socket=server.nextPendingConnection();auto buffer=std::make_shared<QByteArray>();
            connect(socket,&QLocalSocket::readyRead,socket,[socket,buffer,&messages]{
                *buffer+=socket->readAll();while(buffer->size()>=8){const auto op=qFromLittleEndian<quint32>(buffer->constData()),length=qFromLittleEndian<quint32>(buffer->constData()+4);if(buffer->size()<length+8)return;
                    const auto body=QJsonDocument::fromJson(buffer->mid(8,length)).object();buffer->remove(0,8+length);messages.append(body);
                    if(op==0){const auto ready=QJsonDocument(QJsonObject{{"evt","READY"}}).toJson(QJsonDocument::Compact);QByteArray header(8,Qt::Uninitialized);qToLittleEndian<quint32>(1,header.data());qToLittleEndian<quint32>(quint32(ready.size()),header.data()+4);socket->write(header.left(3));socket->write(header.mid(3)+ready);}
                }
            });
        });
        DiscordPresence presence("1564996413849346068",nullptr,prefix);
        presence.setActivityProvider([](QString,bool share){return QVariantMap{{"details","Playing Fixture"},{"server",share?"192.168.1.22":""},{"serverName","Office 192.168.1.22"}};});
        presence.configure(true,true);
        QTRY_VERIFY_WITH_TIMEOUT(messages.size()>=2,4000);
        QCOMPARE(messages[0]["client_id"].toString(),QString("1564996413849346068"));
        QCOMPARE(messages[1]["cmd"].toString(),QString("SET_ACTIVITY"));
        QCOMPARE(messages[1]["args"].toObject()["activity"].toObject()["state"].toString(),QString("Private server"));
        QVERIFY(!QJsonDocument(messages[1]).toJson().contains("192.168"));
        presence.configure(false,false);
        QTRY_VERIFY_WITH_TIMEOUT(messages.size()>=3,1000);
        QVERIFY(messages.last()["args"].toObject()["activity"].isNull());
    }
};
QTEST_GUILESS_MAIN(DiscordTests)
#include "discord_tests.moc"
