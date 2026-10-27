#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>
#include <atomic>
#include <stdexcept>

namespace moly {
struct Error : std::runtime_error { explicit Error(const QString &s) : std::runtime_error(s.toStdString()) {} };
using Progress = std::function<void(QString, qint64, qint64)>;
struct Context {
    std::atomic_bool *cancel = nullptr;
    Progress progress;
    std::function<void(qint64, qint64, double)> transfer;
    void check() const;
    void report(const QString &s, qint64 done = 0, qint64 total = 0) const;
};
QJsonObject readObject(const QString &path, bool optional = false);
void writeObject(const QString &path, const QJsonObject &object);
void writeBytes(const QString &path, const QByteArray &bytes);
QString safePath(const QString &root, const QString &relative);
QString offlineUuid(const QString &name);
QString redact(QString text, const QStringList &secrets = {});
QString fileHash(const QString &path, const QString &algorithm = "sha1");
void extractZip(const QString &zip, const QString &destination, const QString &prefix = {}, bool natives = false, const Context &context = {}, bool atomicWrites = true);
QByteArray zipEntry(const QString &zip, const QString &entry);
void copyTree(const QString &from, const QString &to, const Context &ctx = {});
void replaceTree(const QString &from, const QString &to);
bool rulesAllow(const QJsonArray &rules, const QJsonObject &features = {});
QStringList argumentValues(const QJsonArray &args, const QJsonObject &features = {});
QString mavenPath(const QString &name);
qint64 physicalMemoryMb();
QString timestamp();
}
