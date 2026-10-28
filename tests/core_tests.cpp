#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QUrl>
#include <private/qzipwriter_p.h>
#include "core.h"
#include "content.h"
#include "minecraft.h"
#include "accounts.h"
using namespace moly;
class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void minecraftAuthErrorsDoNotConfuseAuthorizationWithOwnership(){
        MinecraftServiceError denied(HttpError("raw",403,true));QVERIFY(denied.accessReview);QVERIFY(QString::fromUtf8(denied.what()).contains("app registration"));
        MinecraftServiceError unknown(HttpError("raw",403));QVERIFY(QString::fromUtf8(unknown.what()).contains("does not confirm"));
        MinecraftServiceError session(HttpError("raw",401));QVERIFY(!session.accessReview);QVERIFY(QString::fromUtf8(session.what()).contains("fresh sign-in"));
    }
    void offlineIdentity(){QCOMPARE(offlineUuid("Notch"),QString("b50ad385829d3141a2167e7d7539ba7f"));QVERIFY(offlineUuid("Notch")!=offlineUuid("notch"));}
    void pathSafety(){QTemporaryDir d;for(const auto &p:QStringList{"../escape","a/../../x","C:/x","\\\\server\\file","a:stream","CON.txt","a/./x","a/../x","name.","a/ /bad"}){if(p=="a/ /bad")continue;QVERIFY_EXCEPTION_THROWN(safePath(d.path(),p),Error);}QCOMPARE(safePath(d.path(),"mods/a.jar"),d.path()+"/mods/a.jar");QVERIFY_EXCEPTION_THROWN(contentPath(d.path(),"saves/world/level.dat"),Error);}
    void windowsRules(){QVERIFY(rulesAllow({}));QVERIFY(!rulesAllow(QJsonArray{QJsonObject{{"action","allow"},{"os",QJsonObject{{"name","linux"}}}}}));QVERIFY(rulesAllow(QJsonArray{QJsonObject{{"action","allow"}},QJsonObject{{"action","disallow"},{"os",QJsonObject{{"name","osx"}}}}}));QVERIFY(!rulesAllow(QJsonArray{QJsonObject{{"action","allow"},{"features",QJsonObject{{"is_demo_user",true}}}}}));}
    void argsAndMaven(){QCOMPARE(mavenPath("net.fabricmc:fabric-loader:0.16.0"),QString("net/fabricmc/fabric-loader/0.16.0/fabric-loader-0.16.0.jar"));QCOMPARE(mavenPath("org.lwjgl:lwjgl:3.3.3:natives-windows"),QString("org/lwjgl/lwjgl/3.3.3/lwjgl-3.3.3-natives-windows.jar"));QCOMPARE(argumentValues(QJsonArray{"hello",QJsonObject{{"rules",QJsonArray{QJsonObject{{"action","allow"},{"features",QJsonObject{{"has_custom_resolution",true}}}}}},{"value",QJsonArray{"--width","${resolution_width}"}}}},{{"has_custom_resolution",true}}).size(),3);}
    void logRedaction(){auto r=redact("--accessToken supersecret\nAuthorization: Bearer abc.def.xyz\nrefresh_token=abcdef",{"supersecret"});QVERIFY(!r.contains("supersecret"));QVERIFY(!r.contains("abc.def.xyz"));QVERIFY(!r.contains("abcdef"));}
    void atomicJson(){QTemporaryDir d;writeObject(d.path()+"/nested/state.json",{{"version",1}});QCOMPARE(readObject(d.path()+"/nested/state.json")["version"].toInt(),1);writeObject(d.path()+"/nested/state.json",{{"version",2}});QCOMPARE(readObject(d.path()+"/nested/state.json")["version"].toInt(),2);}
    void unsafeArchive(){QTemporaryDir d;QString zip=d.path()+"/bad.zip";{QZipWriter w(zip);w.addFile("../escape.txt",QByteArray("bad"));w.close();}QVERIFY_EXCEPTION_THROWN(extractZip(zip,d.path()+"/out"),Error);QVERIFY(!QFile::exists(d.path()+"/escape.txt"));}
    void validArchive(){QTemporaryDir d;QString zip=d.path()+"/good.zip";{QZipWriter w(zip);w.addFile("overrides/config/example.txt",QByteArray("hello"));w.close();}extractZip(zip,d.path()+"/out","overrides/");QFile f(d.path()+"/out/config/example.txt");QVERIFY(f.open(QIODevice::ReadOnly));QCOMPARE(f.readAll(),QByteArray("hello"));}
    void archiveDirectoryWithoutAttributes(){QTemporaryDir d;QString zip=d.path()+"/folders.zip";{QZipWriter w(zip);w.addFile("overrides/",QByteArray());w.addFile("overrides/config/nested/settings.json",QByteArray("{}"));w.close();}extractZip(zip,d.path()+"/out");QVERIFY(QFileInfo(d.path()+"/out/overrides").isDir());QCOMPARE(readObject(d.path()+"/out/overrides/config/nested/settings.json"),QJsonObject{});}
    void packPreparationKeepsClientPrecedenceAndSkipsServerPayload(){
        QTemporaryDir d;const auto archive=d.path()+"/client.mrpack";
        const QJsonObject index{{"formatVersion",1},{"game","minecraft"},{"name","Client pack"},{"dependencies",QJsonObject{{"minecraft","1.20.1"}}},{"files",QJsonArray{}}};
        {QZipWriter writer(archive);writer.addFile("modrinth.index.json",QJsonDocument(index).toJson());writer.addFile("overrides/config/example.txt",QByteArray("common"));writer.addFile("client-overrides/config/example.txt",QByteArray("client"));writer.addFile("server-overrides/server-only.txt",QByteArray("server"));writer.close();}
        const auto plan=Content(d.path()+"/storage").pack(archive);const auto files=plan["files"].toArray();
        QCOMPARE(files.size(),1);const auto file=files[0].toObject();QCOMPARE(file["path"].toString(),QString("config/example.txt"));
        QFile payload(file["local"].toString());QVERIFY(payload.open(QIODevice::ReadOnly));QCOMPARE(payload.readAll(),QByteArray("client"));
        QCOMPARE(fileHash(file["local"].toString(),"sha512"),file["hash"].toString());
        QVERIFY(!QFileInfo::exists(QFileInfo(file["local"].toString()).absolutePath()+"/server-only.txt"));
    }
    void cancelledPackPreparationRemovesItsPrivateStage(){
        QTemporaryDir d;const auto archive=d.path()+"/cancel.mrpack";
        const QJsonObject index{{"formatVersion",1},{"game","minecraft"},{"name","Cancelled pack"},{"dependencies",QJsonObject{{"minecraft","1.20.1"}}},{"files",QJsonArray{}}};
        {QZipWriter writer(archive);writer.addFile("modrinth.index.json",QJsonDocument(index).toJson());writer.addFile("overrides/config/example.txt",QByteArray("unchanged"));writer.close();}
        std::atomic_bool cancelled=false;Context context{&cancelled,[&](QString stage,qint64 done,qint64){if(stage=="Preparing pack files"&&done>0)cancelled=true;}};
        QVERIFY_EXCEPTION_THROWN(Content(d.path()+"/storage",{},context).pack(archive),Error);
        QVERIFY(QDir(d.path()+"/storage/cache/imports").entryList(QDir::Dirs|QDir::NoDotAndDotDot).isEmpty());
        QVERIFY(QFileInfo::exists(archive));
    }
    void transactionalContent(){QTemporaryDir d;auto source=d.path()+"/source.jar";writeBytes(source,"new");auto base=d.path()+"/instances/test";writeBytes(base+"/game/mods/test.jar","original");writeBytes(base+"/game/saves/world/level.dat","world");QJsonObject instance{{"id","test"}};QJsonObject file{{"path","mods/test.jar"},{"local",source},{"hash",fileHash(source,"sha512")},{"algorithm","sha512"},{"provider","Local file"}};Content content(d.path());content.apply(instance,{{"files",QJsonArray{file}}});QCOMPARE(fileHash(base+"/game/mods/test.jar"),fileHash(source));auto backups=QDir(base+"/backups").entryList(QDir::Dirs|QDir::NoDotAndDotDot);QCOMPARE(backups.size(),1);content.restore(instance,backups.first());QFile restored(base+"/game/mods/test.jar");QVERIFY(restored.open(QIODevice::ReadOnly));QCOMPARE(restored.readAll(),QByteArray("original"));QFile world(base+"/game/saves/world/level.dat");QVERIFY(world.open(QIODevice::ReadOnly));QCOMPARE(world.readAll(),QByteArray("world"));}
    void removalOnlyKeepsRestorableBackup() {
        QTemporaryDir storage;
        const auto base=storage.path()+"/instances/test";
        writeBytes(base+"/game/mods/remove.jar","original mod");
        writeBytes(base+"/game/mods/keep.jar","neighbor");
        writeBytes(base+"/game/saves/world/level.dat","world");
        const auto originalHash=fileHash(base+"/game/mods/remove.jar");
        const QJsonObject manifest{{"schemaVersion",1},{"files",QJsonArray{QJsonObject{{"path","mods/remove.jar"}},QJsonObject{{"path","mods/keep.jar"}}}}};
        writeObject(base+"/content.json",manifest);
        Content content(storage.path());
        const QJsonObject instance{{"id","test"}};
        content.apply(instance,{{"files",QJsonArray{}},{"remove",QJsonArray{"mods/remove.jar"}}});
        QVERIFY(!QFileInfo::exists(base+"/game/mods/remove.jar"));
        QCOMPARE(readObject(base+"/content.json")["files"].toArray().size(),1);
        QVERIFY(QFileInfo::exists(base+"/game/mods/keep.jar"));
        QVERIFY(QFileInfo::exists(base+"/game/saves/world/level.dat"));
        QVERIFY(!QFileInfo::exists(base+"/transaction.json"));
        const auto backups=QDir(base+"/backups").entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        QCOMPARE(backups.size(),1);
        QCOMPARE(fileHash(base+"/backups/"+backups.first()+"/files/mods/remove.jar"),originalHash);
        content.restore(instance,backups.first());
        QCOMPARE(fileHash(base+"/game/mods/remove.jar"),originalHash);
        QCOMPARE(readObject(base+"/content.json"),manifest);
    }
    void localPackArtworkIsExtracted() {
        QTemporaryDir root;const auto bytes=QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jGv8AAAAASUVORK5CYII=");
        for(const auto &type:QStringList{"resourcepack","shader"}) {
            const auto folder=contentFolder(type);QDir().mkpath(root.path()+"/instances/test/game/"+folder);const auto path=root.path()+"/instances/test/game/"+folder+"/example.zip";
            {QZipWriter writer(path);writer.addFile("pack.png",bytes);writer.close();}
            const auto rows=Content(root.path()).files({{"id","test"}},type);QCOMPARE(rows.size(),1);const auto icon=QUrl(rows[0].toObject()["icon"].toString()).toLocalFile();QVERIFY(!icon.isEmpty());QFile file(icon);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),bytes);
        }
    }
    void versionReplacementRemovesOldJarAndCanRestoreIt() {
        QTemporaryDir root;const auto base=root.path()+"/instances/test";const auto source=root.path()+"/new.jar";writeBytes(source,"new-version");writeBytes(base+"/game/mods/old.jar.disabled","old-version");
        writeObject(base+"/content.json",{{"files",QJsonArray{QJsonObject{{"path","mods/old.jar.disabled"},{"provider","Modrinth"},{"projectId","fixture"},{"versionId","v1"}}}}});
        Content content(root.path());content.apply({{"id","test"}},{{"files",QJsonArray{QJsonObject{{"path","mods/new.jar"},{"provider","Modrinth"},{"projectId","fixture"},{"versionId","v2"},{"local",source},{"hash",fileHash(source)},{"algorithm","sha1"}}}}});
        QVERIFY(!QFileInfo::exists(base+"/game/mods/old.jar.disabled"));QVERIFY(QFileInfo::exists(base+"/game/mods/new.jar"));
        const auto backups=QDir(base+"/backups").entryList(QDir::Dirs|QDir::NoDotAndDotDot);QCOMPARE(backups.size(),1);content.restore({{"id","test"}},backups.first());QVERIFY(QFileInfo::exists(base+"/game/mods/old.jar.disabled"));QVERIFY(!QFileInfo::exists(base+"/game/mods/new.jar"));
    }
    void cancelledContent(){QTemporaryDir d;auto source=d.path()+"/s.jar";writeBytes(source,"new");writeBytes(d.path()+"/instances/test/game/mods/test.jar","original");std::atomic_bool cancel=true;Content content(d.path(),{},Context{&cancel,{}});QVERIFY_EXCEPTION_THROWN(content.apply({{"id","test"}},{{"files",QJsonArray{QJsonObject{{"path","mods/test.jar"},{"local",source},{"hash",fileHash(source,"sha1")},{"algorithm","sha1"}}}}}),Error);QFile f(d.path()+"/instances/test/game/mods/test.jar");QVERIFY(f.open(QIODevice::ReadOnly));QCOMPARE(f.readAll(),QByteArray("original"));}
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
