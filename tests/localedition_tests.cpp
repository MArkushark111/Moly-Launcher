#include <QtTest>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include "localedition.h"
#include "network.h"
#include "minecraft.h"
using namespace moly;
class LocalEditionTests:public QObject {
    Q_OBJECT
private slots:
    void importsLocalVanillaLibrary() {
        QTemporaryDir source, destination;
        const auto versionDir = source.path() + "/versions/1.21.11/";
        const auto client = versionDir + "1.21.11.jar";
        const auto index = source.path() + "/assets/indexes/test.json";
        const auto asset = QByteArray("local test asset");
        const auto hash = QString::fromLatin1(QCryptographicHash::hash(asset, QCryptographicHash::Sha1).toHex());
        const auto assetPath = "/assets/objects/" + hash.left(2) + "/" + hash;
        writeBytes(source.path() + assetPath, asset);
        writeObject(index, {{"objects", QJsonObject{{"test/asset", QJsonObject{{"hash", hash}, {"size", asset.size()}}}}}});
        writeBytes(client, "local client fixture");
        writeBytes(source.path() + "/launcher_accounts.json", "private fixture");
        const auto entry = [](const QString &path) {
            return QJsonObject{{"sha1", fileHash(path)}, {"size", QFileInfo(path).size()}};
        };
        auto indexEntry = entry(index);
        indexEntry["id"] = "test";
        writeObject(versionDir + "1.21.11.json", {
            {"id", "1.21.11"}, {"type", "release"}, {"mainClass", "net.minecraft.client.main.Main"},
            {"arguments", QJsonObject{}}, {"downloads", QJsonObject{{"client", entry(client)}}}, {"assetIndex", indexEntry}
        });
        QCOMPARE(Minecraft(destination.path()).importLocalLibrary(source.path()).size(), 1);
        QCOMPARE(Minecraft(destination.path()).versions(false).first().toObject()["id"].toString(), QString("1.21.11"));
        QCOMPARE(fileHash(destination.path() + "/cache/game" + assetPath), hash);
        QVERIFY(!QFileInfo::exists(destination.path() + "/launcher_accounts.json"));
        QVERIFY(!QFileInfo::exists(destination.path() + "/instances"));

        QTemporaryDir brokenDestination;
        writeBytes(source.path() + assetPath, "corrupted asset");
        QVERIFY_EXCEPTION_THROWN(Minecraft(brokenDestination.path()).importLocalLibrary(source.path()), Error);
        QVERIFY(Minecraft(brokenDestination.path()).versions(false).isEmpty());
    }
    void importsInstalledMinecraft() {
        const auto source = qEnvironmentVariable("MOLY_TEST_LOCAL_SOURCE");
        if (source.isEmpty()) QSKIP("An installed Minecraft library was not supplied.");
        QTemporaryDir destination;
        Minecraft importer(destination.path());
        const auto catalog = importer.importLocalLibrary(source);
        QVERIFY(!catalog.isEmpty());
        QCOMPARE(Minecraft(destination.path()).versions(true), catalog);
    }
    void noCloudRequests(){QVERIFY(localEdition());Network net;QVERIFY_EXCEPTION_THROWN(net.request("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"),Error);}
    void downloadsUseOnlyVerifiedLocalFiles(){QTemporaryDir dir;const auto p=dir.path()+"/game.jar";writeBytes(p,"private fixture");Network net;const auto hash=fileHash(p);net.downloads({{"https://invalid.example/game.jar",p,hash}});writeBytes(p,"broken");QVERIFY_EXCEPTION_THROWN(net.downloads({{"https://invalid.example/game.jar",p,hash}}),Error);QCOMPARE(QFile(p).size(),qint64(6));}
    void localLibraryCopiesWithoutAccounts(){QTemporaryDir src,dst;writeBytes(src.path()+"/cache/example.bin","fixture");const auto hash=fileHash(src.path()+"/cache/example.bin","sha256");writeObject(src.path()+"/manifest.json",{{"schemaVersion",1},{"privatePersonalBackup",true},{"files",QJsonArray{QJsonObject{{"path","cache/example.bin"},{"size",7},{"sha256",hash}}}}});seedLocalLibrary(src.path(),dst.path());QCOMPARE(fileHash(dst.path()+"/cache/example.bin","sha256"),hash);QVERIFY(!QFile::exists(dst.path()+"/accounts.json"));}
    void localLibraryRejectsNonGamePaths(){QTemporaryDir src,dst;writeObject(src.path()+"/manifest.json",{{"schemaVersion",1},{"privatePersonalBackup",true},{"files",QJsonArray{QJsonObject{{"path","accounts.json"},{"size",0},{"sha256","invalid"}}}}});QVERIFY_EXCEPTION_THROWN(seedLocalLibrary(src.path(),dst.path()),Error);QVERIFY(!QFile::exists(dst.path()+"/accounts.json"));}
};
QTEST_GUILESS_MAIN(LocalEditionTests)
#include "localedition_tests.moc"
