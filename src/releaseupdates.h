#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantMap>
#include <QJsonObject>

namespace moly {
// Invalid metadata is rejected before it can become an actionable download link.
QVariantMap parseLauncherRelease(const QJsonObject &release, const QString &currentVersion);

class ReleaseUpdates : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantMap release READ release NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY changed)
public:
    explicit ReleaseUpdates(QObject *parent = nullptr);
    bool checking() const { return checking_; }
    bool available() const { return release_.value("available").toBool(); }
    QString status() const { return status_; }
    QVariantMap release() const { return release_; }
    QString currentVersion() const;
    bool autoCheck() const { return autoCheck_; }
    void setAutoCheck(bool enabled);
    Q_INVOKABLE void check();
    Q_INVOKABLE bool openUpdate();
    Q_INVOKABLE bool openRelease();
signals:
    void changed();
private:
    QNetworkAccessManager network_;
    QVariantMap release_;
    QString status_ = "Check for new Moly releases on GitHub.";
    QByteArray etag_;
    bool checking_ = false;
    bool autoCheck_ = true;
};
}
