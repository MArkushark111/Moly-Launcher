// Opt-in, offline controller-flow regression. Always uses a fresh temporary
// child of the supplied isolated parent directory; never edits existing data.
// Usage: export_integration <isolated-data-parent>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QUuid>

#include "core.h"
#include "launcher.h"

using namespace moly;

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Moly");
    app.setApplicationName("Moly export integration");
    QTextStream output(stdout);
    const auto args=app.arguments();
    if(args.size()!=2) {
        output << "Usage: export_integration <isolated-data-parent>\n";
        return 2;
    }
    try {
        const auto parent=QDir(args[1]).absolutePath();
        if(!QDir().mkpath(parent))throw Error("Cannot create the isolated test parent directory.");
        QTemporaryDir storage(parent+"/moly-export-integration-XXXXXX");
        if(!storage.isValid())throw Error("Cannot create isolated integration storage.");
        const auto root=storage.path();
        const auto id="fixture-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto source=root+"/instances/"+id;
        const QJsonObject instance{{"schemaVersion",1},{"id",id},{"name","Export controller fixture"},
            {"version","1.20.1"},{"loader","Vanilla"},{"group","Integration"},{"iconFile","icon.png"},
            {"backgroundFile","background.gif"},
            {"settings",QJsonObject{{"memoryMb",2048},{"fullscreen",true},
                {"javaPath","C:/private/java.exe"},{"jvmArgs","-javaagent:C:/private/agent.jar"}}}};
        writeObject(source+"/instance.json",instance);
        writeObject(source+"/installation.json",{{"fixture",true}});
        writeBytes(source+"/game/mods/local.jar","local mod bytes");
        writeBytes(source+"/game/mods/paused.jar.disabled","disabled mod bytes");
        writeBytes(source+"/game/config/local.toml","enabled = true");
        writeBytes(source+"/game/saves/KeepWorld/level.dat","world should stay only in the original");
        writeBytes(source+"/icon.png",QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQIHWP4z8DwHwAFgAI/ScLbtAAAAABJRU5ErkJggg=="));
        writeBytes(source+"/background.gif",QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"));
        writeObject(source+"/content.json",{{"schemaVersion",1},{"files",QJsonArray{
            QJsonObject{{"path","mods/local.jar"},{"provider","Local file"},{"type","mod"},
                {"local","C:/private/source.jar"},{"access_token","private-value"},
                {"hash",fileHash(source+"/game/mods/local.jar")},{"algorithm","sha1"}}}}});

        Launcher launcher(root);
        launcher.selectInstance(id);
        const auto archive=root+"/fixture.mlypack";
        const auto originalWorldHash=fileHash(source+"/game/saves/KeepWorld/level.dat");
        enum class Phase { Exporting, ImportScheduled, Importing, Finished };
        Phase phase=Phase::Exporting;
        QString failure;
        bool sawExportBusy=false,sawImportBusy=false;
        auto fail=[&](const QString &message){
            if(phase==Phase::Finished)return;
            phase=Phase::Finished;failure=message;launcher.cancel();
            output << "FAIL: " << redact(message) << Qt::endl;
            app.exit(1);
        };
        QObject::connect(&launcher,&Launcher::installFailed,&app,[&](const QString &message){fail(message);});
        QObject::connect(&launcher,&Launcher::busyChanged,&app,[&]{
            if(launcher.busy()){
                if(phase==Phase::Exporting)sawExportBusy=true;
                if(phase==Phase::Importing)sawImportBusy=true;
                return;
            }
            if(phase==Phase::Exporting&&sawExportBusy){
                phase=Phase::ImportScheduled;
                QTimer::singleShot(0,&app,[&]{
                    if(phase!=Phase::ImportScheduled)return;
                    try{
                        if(!QFileInfo(archive).isFile())throw Error("Controller export did not commit the archive.");
                        if(launcher.instances().size()!=1||launcher.selectedId()!=id)
                            throw Error("Export changed the selected instance or library.");
                        const auto manifest=QJsonDocument::fromJson(zipEntry(archive,"moly-pack.json")).object();
                        if(manifest["format"]!="moly-instance-pack"||manifest["includesWorlds"].toBool())
                            throw Error("Controller export used the wrong format/world setting.");
                        const auto serialized=QJsonDocument(manifest).toJson();
                        if(serialized.contains("C:/private")||serialized.contains("private-value"))
                            throw Error("The controller export leaked private source paths or launch credentials.");
                        phase=Phase::Importing;
                        launcher.importPack(QUrl::fromLocalFile(archive).toString(),false);
                        if(!launcher.busy())throw Error("Controller import did not start its background task.");
                    }catch(const std::exception &error){fail(QString::fromUtf8(error.what()));}
                });
                return;
            }
            if(phase==Phase::Importing&&sawImportBusy){
                phase=Phase::ImportScheduled;
                QTimer::singleShot(0,&app,[&]{
                    if(phase!=Phase::ImportScheduled)return;
                    try{
                        if(launcher.instances().size()!=2||launcher.selectedId().isEmpty()||launcher.selectedId()==id)
                            throw Error("Portable import did not add and select a fresh instance.");
                        if(launcher.selected().value("installed").toBool())
                            throw Error("Portable import incorrectly reused the original installed state.");
                        const auto destination=safePath(root+"/instances",launcher.selectedId());
                        for(const auto &path:{"game/mods/local.jar","game/mods/paused.jar.disabled","game/config/local.toml","icon.png","background.gif"})
                            if(fileHash(destination+'/'+path)!=fileHash(source+'/'+path))
                                throw Error("Imported payload differs: "+QString(path));
                        if(QFileInfo::exists(destination+"/installation.json")||QFileInfo::exists(destination+"/game/saves"))
                            throw Error("Import restored a machine installation marker or unselected worlds.");
                        const auto imported=readObject(destination+"/instance.json");
                        if(imported["name"]!=instance["name"]||imported["settings"].toObject().contains("javaPath")||imported["settings"].toObject().contains("jvmArgs"))
                            throw Error("Imported metadata was not preserved and sanitized.");
                        if(fileHash(source+"/game/saves/KeepWorld/level.dat")!=originalWorldHash)
                            throw Error("Import modified the original world.");
                        if(!launcher.taskInstanceId().isEmpty()||!launcher.taskOperation().isEmpty())
                            throw Error("Controller left stale active task metadata.");
                        phase=Phase::Finished;
                        output << "PASS: Launcher export/import committed a verified portable setup, selected a fresh instance, preserved the original, and cleared machine installation state.\n";
                        app.exit(0);
                    }catch(const std::exception &error){fail(QString::fromUtf8(error.what()));}
                });
            }
        });
        QTimer timeout;timeout.setSingleShot(true);timeout.setInterval(30000);
        QObject::connect(&timeout,&QTimer::timeout,&app,[&]{fail("Export/import controller regression timed out after 30 seconds.");});
        timeout.start();
        // Begin before entering the event loop so the deferred catalog refresh
        // sees an active task and does not contact an external service.
        launcher.exportInstance(QUrl::fromLocalFile(archive).toString(),"mlypack",false);
        if(!launcher.busy()||!sawExportBusy)throw Error("Controller export did not start its background task.");
        return app.exec();
    }catch(const std::exception &error){
        output << "FAIL: " << redact(QString::fromUtf8(error.what())) << Qt::endl;
        return 1;
    }
}
