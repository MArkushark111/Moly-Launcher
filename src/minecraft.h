#pragma once
#include "network.h"
namespace moly {
struct LaunchCommand { QString program,workingDirectory; QStringList arguments; };
class Minecraft {
public:
    Minecraft(QString storage, Context context = {}) : root(std::move(storage)), net(context) {}
    QJsonArray versions(bool snapshots);
    QStringList loaders(const QString &game,const QString &loader);
    QJsonObject install(const QJsonObject &instance,const QJsonObject &settings);
    QJsonArray importLocalLibrary(const QString &source);
    QString javaFor(int major,const QString &custom = {});
    static LaunchCommand command(const QJsonObject &installation,const QJsonObject &instance,const QJsonObject &settings,const QJsonObject &account,const QString &token);
private:
    QString root;
    QString importSource, importWorkspace;
    Network net;
    QJsonObject gameVersion(const QString &version);
    void libraries(const QJsonObject &profile,const QString &natives,QStringList &classpath);
    QJsonObject loaderProfile(const QJsonObject &instance,const QJsonObject &vanilla,const QString &java);
};
}
