#pragma once
#include "network.h"
namespace moly {
class Content {
  public:
    Content(QString storage, QString curseKey = {}, Context context = {})
        : root(std::move(storage)), key(std::move(curseKey)), net(context) {}
    QJsonArray search(const QString &provider, const QString &type, const QString &query,
                      const QJsonObject &instance, const QString &category = QString());
    QJsonObject searchPage(const QString &provider, const QString &type, const QString &query,
                           const QJsonObject &instance, const QString &category, int offset, int limit);
    QJsonObject plan(const QJsonObject &project, const QJsonObject &instance);
    QJsonArray versions(const QJsonObject &project, const QJsonObject &instance);
    QJsonObject updates(const QJsonObject &instance);
    QJsonObject pack(const QString &archive, const QJsonObject &source = {});
    void apply(const QJsonObject &instance, const QJsonObject &plan);
    void restore(const QJsonObject &instance, const QString &backup);
    void recover(const QJsonObject &instance);
    QJsonArray files(const QJsonObject &instance, const QString &type);

  private:
    QString root, key;
    Network net;
    QMap<QString, QJsonObject> metadataCache;
    QJsonObject projectMetadata(const QString &provider, const QString &project,
                                const QJsonObject &known = {});
    QJsonObject modrinthVersion(const QString &project, const QJsonObject &instance,
                                const QString &version = {});
    QJsonObject curseFile(qint64 project, const QJsonObject &instance, qint64 file = 0);
    void resolve(const QString &provider, const QString &project, const QString &type,
                 const QJsonObject &instance, QJsonArray &files, QSet<QString> &seen,
                 const QString &version = {}, const QJsonObject &metadata = {});
};
QString contentFolder(const QString &type);
QString contentPath(const QString &root, const QString &relative);
QJsonObject mergeSearchPage(const QJsonArray &previous, const QJsonObject &page, int offset, int pageSize);
} // namespace moly
