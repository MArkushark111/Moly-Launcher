#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <functional>
#include <QMap>

namespace moly {

// These parsers are also used by the bounded-file regression tests.
QVariantMap readMinecraftNbt(const QByteArray &bytes);
QVariantMap readMinecraftNbtFile(const QString &path);
QVariantList savedPlayTargets(const QString &root, const QStringList &instanceIds = {});
QVariantMap parseMinecraftAddress(const QString &address);

class PlayTargets : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString dataRoot READ dataRoot WRITE setDataRoot NOTIFY configurationChanged)
    Q_PROPERTY(QString selectedInstanceId READ selectedInstanceId WRITE setSelectedInstanceId NOTIFY configurationChanged)
    Q_PROPERTY(QStringList instanceIds READ instanceIds WRITE setInstanceIds NOTIFY configurationChanged)
    Q_PROPERTY(bool pingEnabled READ pingEnabled WRITE setPingEnabled NOTIFY configurationChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool checkingStatus READ checkingStatus NOTIFY checkingStatusChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit PlayTargets(QString root = {}, QObject *parent = nullptr);
    ~PlayTargets() override;
    QString dataRoot() const { return root_; }
    QString selectedInstanceId() const { return selected_; }
    QStringList instanceIds() const { return instanceIds_; }
    bool pingEnabled() const { return pingEnabled_; }
    QVariantList items() const;
    bool loading() const { return loading_; }
    bool checkingStatus() const { return pendingPings_ > 0; }
    QString error() const { return error_; }

    void setDataRoot(const QString &root);
    void setSelectedInstanceId(const QString &id);
    void setInstanceIds(const QStringList &ids);
    void setPingEnabled(bool enabled);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void refreshServer(const QString &address);
    Q_INVOKABLE bool copyAddress(const QString &address);
    // The launcher must reject this operation while the owning game is running.
    Q_INVOKABLE bool setWorldIcon(const QString &instanceId, const QString &worldFolder,
                                  const QString &imageUrl);
    Q_INVOKABLE void activate(const QVariantMap &entry);
signals:
    void configurationChanged();
    void itemsChanged();
    void loadingChanged();
    void checkingStatusChanged();
    void errorChanged();
    void errorOccurred(QString message);
    void notice(QString message);
    void targetChosen(QString instanceId, QString type, QString value);
private:
    QString root_, selected_, error_;
    QStringList instanceIds_;
    QVariantList allItems_;
    bool pingEnabled_ = true, loading_ = false;
    quint64 generation_ = 0, pingGeneration_ = 0;
    int pendingPings_ = 0;
    QObject *pingGroup_ = nullptr;
    QMap<QString,QVariantMap> serverSnapshots_;
    void rememberServer(const QString &address,const QVariantMap &status);
    void cancelPings();
};

}
