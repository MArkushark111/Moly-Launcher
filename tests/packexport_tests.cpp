#include <QtTest>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <private/qzipwriter_p.h>
#include "packexport.h"

using namespace moly;
class PackExportTests : public QObject {
    Q_OBJECT
    static QJsonObject fixture(const QString &root) {
        QJsonObject instance{{"id", "original"}, {"name", "Portable test"}, {"version", "1.20.1"},
                             {"loader", "Fabric"}, {"loaderVersion", "0.16.0"}, {"group", "Friends"},
                             {"iconFile", "icon.gif"}, {"backgroundFile", "background.png"},
                             {"settings", QJsonObject{{"memoryMb", 3072}, {"fullscreen", true},
                                 {"imageDim", 0.45}, {"javaPath", "C:/private/java.exe"},
                                 {"jvmArgs", "-Daccess_token=secret-value"}}}};
        const auto instanceRoot = root + "/instances/original";
        writeObject(instanceRoot + "/instance.json", instance);
        writeBytes(instanceRoot + "/game/mods/example.jar", "managed mod");
        writeBytes(instanceRoot + "/game/mods/disabled.jar.disabled", "disabled mod");
        writeBytes(instanceRoot + "/game/config/example.toml", "example = true");
        writeBytes(instanceRoot + "/game/options.txt", "fov:1.0");
        writeBytes(instanceRoot + "/game/saves/World/level.dat", "world data");
        writeBytes(instanceRoot + "/game/logs/latest.log", "private output");
        writeBytes(instanceRoot + "/game/assets/game.jar", "game binaries");
        writeBytes(instanceRoot + "/game/screenshots/private.png", "screenshot");
        writeBytes(instanceRoot + "/game/usercache.json", "private username");
        writeBytes(instanceRoot + "/game/config/accounts.json", "private account");
        writeBytes(instanceRoot + "/icon.gif", "GIF89a art");
        writeBytes(instanceRoot + "/background.png", "PNG art");
        writeObject(root + "/settings.json", {{"access_token", "secret-value"}});
        const auto mod = instanceRoot + "/game/mods/example.jar";
        writeObject(instanceRoot + "/content.json", {{"schemaVersion", 1},
            {"files", QJsonArray{QJsonObject{{"path", "mods/example.jar"}, {"provider", "Modrinth"},
                {"projectId", "example"}, {"versionId", "exact-version"}, {"type", "mod"},
                {"url", "https://cdn.modrinth.com/data/example/versions/exact/example.jar"},
                {"hash", fileHash(mod)}, {"algorithm", "sha1"}, {"local", "C:/private/download.jar"},
                {"access_token", "secret-value"}}}}});
        return instance;
    }
    static QString importedDirectory(const QString &root, const QJsonObject &instance) {
        return root + "/instances/" + instance["id"].toString();
    }
  private slots:
    void portableRoundtripExcludesWorldsAndSecrets() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto archive = storage.path() + "/portable.mlypack";
        PackExport::write(storage.path(), instance, archive, "mlypack");
        const auto manifestBytes = zipEntry(archive, "moly-pack.json");
        QVERIFY(!manifestBytes.contains("secret-value"));
        QVERIFY(!manifestBytes.contains("C:/private"));
        const auto imported = PackExport::importMoly(storage.path(), archive);
        QVERIFY(imported["id"] != instance["id"]);
        QCOMPARE(imported["name"], instance["name"]);
        QCOMPARE(imported["loaderVersion"], instance["loaderVersion"]);
        QCOMPARE(imported["settings"].toObject()["memoryMb"].toInt(), 3072);
        QVERIFY(!imported["settings"].toObject().contains("javaPath"));
        QVERIFY(!imported["settings"].toObject().contains("jvmArgs"));
        const auto destination = importedDirectory(storage.path(), imported);
        const auto original = storage.path() + "/instances/original";
        for (const auto &path : {"game/mods/example.jar", "game/mods/disabled.jar.disabled", "game/config/example.toml", "game/options.txt", "icon.gif", "background.png"})
            QCOMPARE(fileHash(destination + '/' + path), fileHash(original + '/' + path));
        for (const auto &path : {"game/saves", "game/logs", "game/assets", "game/screenshots", "game/usercache.json", "game/config/accounts.json", "installation.json"})
            QVERIFY2(!QFileInfo::exists(destination + '/' + path), path);
        QCOMPARE(imported["iconFile"].toString(), QString("icon.gif"));
        QCOMPARE(imported["backgroundFile"].toString(), QString("background.png"));
        QVERIFY(!QJsonDocument(readObject(destination + "/content.json")).toJson().contains("secret-value"));
    }
    void zipRoundtripCanIncludeWorlds() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto archive = storage.path() + "/portable.zip";
        PackExport::write(storage.path(), instance, archive, "zip", true);
        const auto imported = PackExport::importMoly(storage.path(), archive);
        QCOMPARE(fileHash(importedDirectory(storage.path(), imported) + "/game/saves/World/level.dat"),
                 fileHash(storage.path() + "/instances/original/game/saves/World/level.dat"));
        QCOMPARE(zipEntry(archive, "instance.json").contains("Portable test"), true);
    }
    void mrpackHasExactDownloadsAndLocalOverrides() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto archive = storage.path() + "/portable.mrpack";
        PackExport::write(storage.path(), instance, archive, "mrpack", true);
        const auto index = QJsonDocument::fromJson(zipEntry(archive, "modrinth.index.json")).object();
        QCOMPARE(index["formatVersion"].toInt(), 1);
        QCOMPARE(index["game"].toString(), QString("minecraft"));
        QCOMPARE(index["dependencies"].toObject()["fabric-loader"].toString(), QString("0.16.0"));
        QCOMPARE(index["files"].toArray().size(), 1);
        const auto file = index["files"].toArray().first().toObject();
        QCOMPARE(file["path"].toString(), QString("mods/example.jar"));
        QCOMPARE(file["hashes"].toObject()["sha1"].toString().size(), 40);
        QCOMPARE(file["hashes"].toObject()["sha512"].toString().size(), 128);
        QCOMPARE(file["fileSize"].toInt(), 11);
        QCOMPARE(zipEntry(archive, "overrides/mods/disabled.jar.disabled"), QByteArray("disabled mod"));
        QVERIFY_EXCEPTION_THROWN(zipEntry(archive, "overrides/saves/World/level.dat"), Error);
        QVERIFY_EXCEPTION_THROWN(zipEntry(archive, "overrides/usercache.json"), Error);
    }
    void modifiedManagedModRequiresPortableFormat() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        writeBytes(storage.path() + "/instances/original/game/mods/example.jar", "local changes");
        const auto archive = storage.path() + "/portable.mrpack";
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(), instance, archive, "mrpack"), Error);
        QVERIFY(!QFileInfo::exists(archive));
        PackExport::write(storage.path(), instance, storage.path() + "/portable.mlypack", "mlypack");
    }
    void restrictedCurseForgeBytesAreNotEmbedded() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto manifestPath = storage.path() + "/instances/original/content.json";
        auto manifest = readObject(manifestPath);
        auto file = manifest["files"].toArray().first().toObject();
        file["provider"] = "CurseForge";
        file["url"] = "";
        manifest["files"] = QJsonArray{file};
        writeObject(manifestPath, manifest);
        const auto archive = storage.path() + "/restricted.mlypack";
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(), instance, archive, "mlypack"), Error);
        QVERIFY(!QFileInfo::exists(archive));
    }
    void cannotOverwriteSource() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto original = storage.path() + "/instances/original/game/mods/example.jar";
        const auto hash = fileHash(original);
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(), instance, original, "mlypack"), Error);
        QCOMPARE(fileHash(original), hash);
        const auto settings = storage.path()+"/settings.json";
        const auto settingsHash = fileHash(settings);
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(),instance,storage.path()+"/SETTINGS.JSON","mlypack"),Error);
        QCOMPARE(fileHash(settings),settingsHash);
    }
    void cancelledExportPreservesExistingDestination() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto destination = storage.path() + "/existing.mlypack";
        writeBytes(destination, "keep original");
        const auto hash = fileHash(destination);
        std::atomic_bool cancelled = false;
        Context context{&cancelled, [&cancelled](QString text, qint64, qint64) {
            if (text.startsWith("Writing ")) cancelled = true;
        }};
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(), instance, destination, "mlypack", false, context), Error);
        QCOMPARE(fileHash(destination), hash);
    }
    void unsafeArchiveDoesNotCreateInstance() {
        QTemporaryDir storage;
        const auto archive = storage.path() + "/unsafe.mlypack";
        { QZipWriter zip(archive); zip.addFile("../escape.txt", "bad"); zip.close(); }
        QVERIFY_EXCEPTION_THROWN(PackExport::importMoly(storage.path(), archive), Error);
        QVERIFY(!QFileInfo::exists(storage.path() + "/escape.txt"));
        QVERIFY(!QDir(storage.path() + "/instances").exists());
    }
    void sourceChangeDuringExportDoesNotReplaceOldArchive() {
        QTemporaryDir storage;
        const auto instance = fixture(storage.path());
        const auto destination = storage.path() + "/existing.mlypack";
        writeBytes(destination, "old archive");
        const auto hash = fileHash(destination);
        const auto source = storage.path() + "/instances/original/game/config/example.toml";
        Context context{nullptr, [source](QString text, qint64, qint64) {
            if (text == "Writing game/config/example.toml") writeBytes(source, "changed while exporting");
        }};
        QVERIFY_EXCEPTION_THROWN(PackExport::write(storage.path(), instance, destination, "mlypack", false, context), Error);
        QCOMPARE(fileHash(destination), hash);
    }
    void unlistedPayloadDoesNotCommitInstance() {
        QTemporaryDir storage;
        const auto archive = storage.path() + "/extra.mlypack";
        const QJsonObject manifest{{"format", "moly-instance-pack"}, {"schemaVersion", 1},
            {"instance", QJsonObject{{"name", "Extra file"}, {"version", "1.20.1"}, {"loader", "Vanilla"}}}, {"files", QJsonArray{}}};
        { QZipWriter zip(archive); zip.addFile("moly-pack.json", QJsonDocument(manifest).toJson()); zip.addFile("game/config/unlisted.txt", "extra"); zip.close(); }
        QVERIFY_EXCEPTION_THROWN(PackExport::importMoly(storage.path(), archive), Error);
        QVERIFY(QDir(storage.path() + "/instances").entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
    }
    void incorrectHashDoesNotCommitInstance() {
        QTemporaryDir storage;
        const auto archive = storage.path() + "/corrupt.mlypack";
        const QJsonObject instance{{"name", "Bad pack"}, {"version", "1.20.1"}, {"loader", "Vanilla"}};
        const QJsonObject manifest{{"format", "moly-instance-pack"}, {"schemaVersion", 1},
            {"instance", instance}, {"files", QJsonArray{QJsonObject{{"path", "game/config/file.txt"}, {"size", 3}, {"sha512", QString(128, '0')}}}}};
        { QZipWriter zip(archive); zip.addFile("moly-pack.json", QJsonDocument(manifest).toJson()); zip.addFile("game/config/file.txt", "bad"); zip.close(); }
        QVERIFY_EXCEPTION_THROWN(PackExport::importMoly(storage.path(), archive), Error);
        QVERIFY(QDir(storage.path() + "/instances").entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
    }
    void untrustedImportCannotChooseJavaOrInjectLaunchArguments() {
        QTemporaryDir storage;
        const auto archive = storage.path() + "/launch-settings.mlypack";
        const QJsonObject descriptor{{"id", "existing-instance"}, {"name", "Imported settings"},
            {"version", "1.20.1"}, {"loader", "Vanilla"}, {"javaPath", "C:/untrusted/java.exe"},
            {"jvmArgs", "-javaagent:C:/untrusted/agent.jar"}, {"installed", true},
            {"settings", QJsonObject{{"memoryMb", 2048}, {"width", 1280}, {"height", 720},
                {"fullscreen", true}, {"javaPath", "C:/untrusted/java.exe"},
                {"jvmArgs", "-javaagent:C:/untrusted/agent.jar"}, {"access_token", "private"}}}};
        const QJsonObject manifest{{"format", "moly-instance-pack"}, {"schemaVersion", 1},
            {"instance", descriptor}, {"files", QJsonArray{}}};
        { QZipWriter zip(archive); zip.addFile("moly-pack.json", QJsonDocument(manifest).toJson()); zip.close(); }
        const auto imported = PackExport::importMoly(storage.path(), archive);
        QVERIFY(imported["id"].toString() != "existing-instance");
        QVERIFY(!imported.contains("javaPath"));
        QVERIFY(!imported.contains("jvmArgs"));
        QVERIFY(!imported.contains("installed"));
        const auto settings = imported["settings"].toObject();
        QCOMPARE(settings["memoryMb"].toInt(), 2048);
        QCOMPARE(settings["fullscreen"].toBool(), true);
        QVERIFY(!settings.contains("javaPath"));
        QVERIFY(!settings.contains("jvmArgs"));
        QVERIFY(!settings.contains("access_token"));
        QVERIFY(!QFileInfo::exists(importedDirectory(storage.path(), imported) + "/installation.json"));
    }
};
QTEST_GUILESS_MAIN(PackExportTests)
#include "packexport_tests.moc"
