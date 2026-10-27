#include "instanceimport.h"
#include "core.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include <QDir>
#include <QUrl>
using namespace moly;

class ImportTests:public QObject {
    Q_OBJECT
    static void data(const QString &folder) {
        writeBytes(folder+"/mods/example.jar", "example-mod");
        writeBytes(folder+"/mods/disabled.jar.disabled", "disabled-mod");
        writeBytes(folder+"/config/example.json", "{}\n");
        writeBytes(folder+"/saves/World/level.dat", "world-data");
        writeBytes(folder+"/options.txt", "fullscreen:false\n");
        writeBytes(folder+"/accounts.json", "do-not-copy-this-token");
        writeBytes(folder+"/launcher_accounts.json", "do-not-copy-this-token");
        writeBytes(folder+"/essential/microsoft-account.json", "do-not-copy-this-token");
    }
private slots:
    void prismAndMultiMcMetadata() {
        QTemporaryDir source;
        const auto folder=source.path()+"/instances/My setup";
        writeObject(folder+"/mmc-pack.json",{{"components",QJsonArray{QJsonObject{{"uid","net.minecraft"},{"version","1.20.1"}},QJsonObject{{"uid","net.fabricmc.fabric-loader"},{"version","0.16.14"}}}}});
        writeBytes(folder+"/instance.cfg","[General]\nname=My setup\n");data(folder+"/.minecraft");
        for(const auto &provider:QStringList{"Prism","MultiMC"}) {
            const auto found=InstanceImport::scan(provider,{source.path()});QCOMPARE(found.size(),1);
            const auto row=found.first().toObject();QCOMPARE(row["name"].toString(),QString("My setup"));QCOMPARE(row["version"].toString(),QString("1.20.1"));QCOMPARE(row["loader"].toString(),QString("Fabric"));QVERIFY(row["supported"].toBool());
        }
    }
    void curseForgeAndATLauncherMetadata() {
        QTemporaryDir source;
        const auto curse=source.path()+"/Curse";data(curse);
        writeObject(curse+"/minecraftinstance.json",{{"name","Curse setup"},{"gameVersion","1.21.1"},{"baseModLoader",QJsonObject{{"name","neoforge-21.1.90"}}}});
        const auto row=InstanceImport::describe("CurseForge",curse);
        QCOMPARE(row["loader"].toString(),QString("NeoForge"));QCOMPARE(row["loaderVersion"].toString(),QString("21.1.90"));
        const auto at=source.path()+"/AT";data(at);
        writeObject(at+"/instance.json",{{"id","1.20.1"},{"launcher",QJsonObject{{"name","AT setup"},{"loaderVersion",QJsonObject{{"type","Fabric"},{"version","0.16.14"}}}}}});
        const auto found=InstanceImport::describe("ATLauncher",at);QCOMPARE(found["version"].toString(),QString("1.20.1"));QCOMPARE(found["loader"].toString(),QString("Fabric"));QVERIFY(found["supported"].toBool());
    }
    void modrinthDatabaseIsReadOnly() {
        QTemporaryDir source;const auto folder=source.path()+"/profiles/A setup";data(folder);
        const auto dbPath=source.path()+"/app.db";
        {
            auto db=QSqlDatabase::addDatabase("QSQLITE","fixture");db.setDatabaseName(dbPath);QVERIFY(db.open());QSqlQuery query(db);
            QVERIFY(query.exec("CREATE TABLE instances (path TEXT,name TEXT,icon_path TEXT,applied_content_set_id TEXT)"));
            QVERIFY(query.exec("CREATE TABLE instance_content_sets (id TEXT,game_version TEXT,loader TEXT,loader_version TEXT)"));
            QVERIFY(query.exec("CREATE TABLE minecraft_users (token TEXT)"));QVERIFY(query.exec("INSERT INTO minecraft_users VALUES ('do-not-import')"));
            QVERIFY(query.exec("INSERT INTO instances VALUES ('A setup','A setup',NULL,'content')"));
            QVERIFY(query.exec("INSERT INTO instance_content_sets VALUES ('content','1.21.1','fabric','0.16.14')"));db.close();
        }
        QSqlDatabase::removeDatabase("fixture");const auto before=fileHash(dbPath,"sha256");
        const auto found=InstanceImport::scan("Modrinth",{source.path()});QCOMPARE(found.size(),1);
        QCOMPARE(found.first().toObject()["loader"].toString(),QString("Fabric"));QCOMPARE(found.first().toObject()["gameDir"].toString(),QDir(folder).canonicalPath());
        QCOMPARE(fileHash(dbPath,"sha256"),before);
        QVERIFY(!QJsonDocument(found).toJson().contains("do-not-import"));
    }
    void copiesDataWithoutSecretsOrChangingOriginals() {
        QTemporaryDir source,destination;data(source.path());
        auto candidate=InstanceImport::describe("Folder",source.path());
        QVERIFY(!candidate["supported"].toBool());candidate["version"]="1.20.1";candidate["loader"]="Fabric";candidate["loaderVersion"]="0.16.14";
        const auto before=fileHash(source.path()+"/saves/World/level.dat");
        auto imported=InstanceImport::copy(destination.path(),candidate,"copied",true);
        const auto game=destination.path()+"/instances/copied/game";
        QCOMPARE(fileHash(game+"/mods/example.jar"),fileHash(source.path()+"/mods/example.jar"));QVERIFY(QFileInfo::exists(game+"/mods/disabled.jar.disabled"));
        QCOMPARE(fileHash(game+"/saves/World/level.dat"),before);QCOMPARE(fileHash(source.path()+"/saves/World/level.dat"),before);
        QVERIFY(!QFileInfo::exists(game+"/accounts.json"));QVERIFY(!QFileInfo::exists(game+"/launcher_accounts.json"));QVERIFY(!QFileInfo::exists(game+"/essential"));QVERIFY(!QFileInfo::exists(destination.path()+"/instances/copied/installation.json"));
        QVERIFY(QFileInfo::exists(destination.path()+"/instances/copied/instance.json"));
        InstanceImport::copy(destination.path(),candidate,"without-worlds",false);QVERIFY(!QFileInfo::exists(destination.path()+"/instances/without-worlds/game/saves"));
    }
    void cancellationRemovesUncommittedInstance() {
        QTemporaryDir source,destination;data(source.path());auto candidate=InstanceImport::describe("Folder",source.path());candidate["version"]="1.20.1";
        std::atomic_bool cancelled=false;Context context{&cancelled,[&](QString,qint64,qint64){cancelled=true;}};
        QVERIFY_EXCEPTION_THROWN(InstanceImport::copy(destination.path(),candidate,"cancelled",true,context),Error);
        QVERIFY(!QFileInfo::exists(destination.path()+"/instances/cancelled"));QVERIFY(QFileInfo::exists(source.path()+"/mods/example.jar"));
    }
    void refusesSelfImportAndMissingVersions() {
        QTemporaryDir source;data(source.path());auto candidate=InstanceImport::describe("Folder",source.path());
        QVERIFY_EXCEPTION_THROWN(InstanceImport::copy(source.path()+"/Moly",candidate,"bad",true),Error);
        candidate["version"]="1.20.1";QDir().mkpath(source.path()+"/Moly");
        QVERIFY_EXCEPTION_THROWN(InstanceImport::copy(source.path()+"/Moly",candidate,"bad",true),Error);
        candidate["gameDir"]=QDir(source.path()).rootPath();QVERIFY_EXCEPTION_THROWN(InstanceImport::copy(source.path()+"/Moly",candidate,"bad",true),Error);
    }
};
QTEST_GUILESS_MAIN(ImportTests)
#include "instanceimport_tests.moc"
