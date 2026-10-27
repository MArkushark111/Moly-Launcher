#pragma once
#include <QObject>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

class SkinStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList skins READ skins NOTIFY changed)
    Q_PROPERTY(QString selectedSkinId READ selectedSkinId NOTIFY changed)
    Q_PROPERTY(QString selectedSkinUrl READ selectedSkinUrl NOTIFY changed)
    Q_PROPERTY(QString selectedModel READ selectedModel NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  public:
    explicit SkinStore(const QString &dataRoot, QObject *parent = nullptr);
    QVariantList skins() const;
    QString selectedSkinId() const { return selection; }
    QString selectedSkinUrl() const;
    QString selectedModel() const;
    QString error() const { return errorText; }
    Q_INVOKABLE bool importSkin(const QString &url, const QString &model = "classic");
    Q_INVOKABLE void selectSkin(const QString &id);
    Q_INVOKABLE void setSelectedModel(const QString &model);
    Q_INVOKABLE void requestApply();
    static QImage defaultTexture();
    static QImage readSkin(const QString &path);
  signals:
    void changed();
    void errorChanged();
    void applyRequested(QString localPngPath, QString variant);
  private:
    QString root, selection = "default-steve", errorText;
    QJsonArray entries;
    QJsonObject builtinModels;
    QString modelFor(const QString &id) const;
    QString imagePath(const QString &id) const;
    void save();
    void fail(const QString &message);
};
