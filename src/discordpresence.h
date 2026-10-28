#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QJsonObject>
#include <functional>

namespace moly {
// Public server names may be shared; numeric hosts and private addresses never are.
QString publicPresenceServer(const QString &address);
class DiscordPresence : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    explicit DiscordPresence(QString applicationId, QObject *parent=nullptr, QString pipePrefix="discord-ipc-");
    ~DiscordPresence() override;
    bool configured() const;
    QString status() const {return status_;}
    void configure(bool enabled,bool shareServer);
    void setActivityProvider(std::function<QVariantMap(QString,bool)> provider);
    Q_INVOKABLE void setPage(const QString &page);
    void update();
signals:
    void statusChanged();
private:
    QString id_,page_="library",status_="Off";
    QString pipePrefix_;
    bool enabled_=false,shareServer_=true,ready_=false;
    int pipe_=0;
    QLocalSocket socket_;
    QTimer retry_,throttle_;
    QByteArray input_,lastActivity_;
    std::function<QVariantMap(QString,bool)> provider_;
    void connectPipe();
    void frame(quint32 opcode,const QByteArray &body);
    void read();
    void sendActivity();
    void setStatus(const QString &status);
};
}
