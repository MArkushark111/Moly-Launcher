// Opt-in live Modrinth test. Uses only a fresh temporary directory.
#include "launcher.h"
#include "core.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QTemporaryDir>
#include <QTimer>
#include <QTextStream>
#include <QSet>

using namespace moly;
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    QTemporaryDir storage;
    if(!storage.isValid()) return 2;
    const auto root=storage.path(),id=QString("confirm-check");
    writeObject(root+"/instances/"+id+"/instance.json",{{"id",id},{"name","Confirm check"},{"version","1.20.1"},{"loader","Fabric"},{"settings",QJsonObject{}}});
    QDir().mkpath(root+"/instances/"+id+"/game");
    Launcher launcher(root);
    const QVariantMap pack{{"provider","Modrinth"},{"projectId","fabulously-optimized"},{"title","Fabulously Optimized"},{"type","modpack"}};
    const QVariantMap mod{{"provider","Modrinth"},{"projectId","iris"},{"title","Iris"},{"type","mod"}};
    int stage=0,ticks=0,completedInstalls=0;
    QVariantList releaseChoices;QString expectedVersion;
    bool sawResolving=false,sawApplying=false;
    auto fail=[&](const QString &message){qCritical("%s",qPrintable(message));launcher.cancel();app.exit(1);};
    auto noPayload=[&] {
        for(const auto &dir:QStringList{"cache/packs","cache/imports","instances/"+id+"/game"}) {
            QDirIterator files(root+"/"+dir,QDir::Files,QDirIterator::Subdirectories);
            if(files.hasNext()) return false;
        }
        return launcher.jobs().isEmpty()&&launcher.pendingPlan().isEmpty()&&!launcher.busy();
    };
    QObject::connect(&launcher,&Launcher::installFailed,&app,[&](const QString &message){if(stage>0)fail(message);});
    QObject::connect(&launcher,&Launcher::statusChanged,&app,[&]{sawResolving|=launcher.taskOperation()=="resolve-auto";sawApplying|=launcher.taskOperation()=="content";});
    QObject::connect(&launcher,&Launcher::installCompleted,&app,[&](const QString &){
        try {
            const auto files=readObject(root+"/instances/"+id+"/content.json")["files"].toArray();
            if(files.isEmpty()||!sawResolving||!sawApplying)throw Error("Confirm did not resolve and apply content.");
            for(auto value:files){const auto f=value.toObject();if(fileHash(root+"/instances/"+id+"/game/"+f["path"].toString(),f["algorithm"].toString("sha1"))!=f["hash"].toString())throw Error("Installed checksum mismatch.");}
            QSet<QString> projects;
            for(auto value:files){const auto entry=value.toObject();const auto project=entry["projectId"].toString();if(projects.contains(project))throw Error("Two versions of a project remained installed.");projects.insert(project);}
            const auto iris=std::find_if(files.begin(),files.end(),[](const auto &entry){return entry.toObject()["projectId"]=="YL57xq9U";});
            if(iris==files.end()||(!expectedVersion.isEmpty()&&iris->toObject()["versionId"]!=expectedVersion))throw Error("Wrong requested version installed.");
            if(++completedInstalls<3) {
                releaseChoices=launcher.installVersions();
                const auto next=releaseChoices.at(completedInstalls==1?0:qMin(1,int(releaseChoices.size()-1))).toMap()["id"].toString();expectedVersion=next;
                QTimer::singleShot(0,&app,[&,next]{launcher.beginInstall(mod,id,next,true);});
            }else {
                writeObject(root+"/instances/"+id+"/content.json",{{"files",QJsonArray{}}});
                stage=8;QTimer::singleShot(0,&app,[&]{launcher.identifyContent();});
            }
        }catch(const std::exception &e){fail(QString::fromUtf8(e.what()));}
    });
    QTimer timer;timer.setInterval(20);
    QObject::connect(&timer,&QTimer::timeout,&app,[&]{
        if(++ticks>4500){fail("Confirm-first integration timed out.");timer.stop();return;}
        if(stage==8) {
            bool identified=false;for(auto value:launcher.installedContent())if(value.toMap()["projectId"]=="YL57xq9U"&&value.toMap()["versionId"]==expectedVersion)identified=true;
            if(identified){qInfo("Live content checks passed: metadata stayed idle, confirmed install, upgrade, downgrade, checksum verification, one file per project, and exact local hash identification.");timer.stop();app.exit(0);}return;
        }
        if(stage==0){if(launcher.busy()||ticks<5)return;stage=1;launcher.loadInstallChoices(pack);}
        else if(stage==1&&!launcher.loadingInstallChoices()){
            if(launcher.installVersions().isEmpty()||!noPayload()){fail("Pack preview started work before confirmation.");timer.stop();return;}
            stage=2;launcher.loadInstallChoices(mod,id);
        }else if(stage==2&&!launcher.loadingInstallChoices()){
            if(launcher.installVersions().isEmpty()||!noPayload()){fail("Mod preview started work before confirmation.");timer.stop();return;}
            // Re-open the cached version choices, as when switching the target back.
            stage=3;launcher.loadInstallChoices(mod,id);
        }else if(stage==3&&!launcher.loadingInstallChoices()){
            if(!noPayload()){fail("Changing version choices performed installation work.");timer.stop();return;}
            const auto choices=launcher.installVersions();
            stage=4;launcher.beginInstall(mod,id,choices.at(qMin(1,int(choices.size()-1))).toMap()["id"].toString(),true);
        }
    });timer.start();
    return app.exec();
}
