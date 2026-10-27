#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QUrl>
#include <QtTest>

#include "content.h"
#include "core.h"
#include "launcher.h"

using namespace moly;

class LauncherTests : public QObject {
    Q_OBJECT

    static void makeInstance(const QString &root, const QString &id = "test-instance") {
        writeObject(root + "/instances/" + id + "/instance.json", {{"schemaVersion", 1},
                                                                   {"id", id},
                                                                   {"name", "Test instance"},
                                                                   {"version", "1.20.1"},
                                                                   {"loader", "Fabric"},
                                                                   {"loaderVersion", "0.16.0"},
                                                                   {"settings", QJsonObject{}}});
        QDir().mkpath(root + "/instances/" + id + "/game");
    }

  private slots:
    // These synchronous tests intentionally never enter an event loop. The launcher's deferred
    // catalog refresh is cancelled with object destruction, so the suite does not contact services.
    void instanceWindowOverridesValidateAndPersist() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        Launcher launcher(storage.path());
        QSignalSpy notices(&launcher,&Launcher::notice);
        launcher.saveInstance({{"name","Window test"},{"settings",QVariantMap{{"width",1600},{"height",900},{"fullscreen",true},{"keepVisible",false},{"memoryMb",2048}}}});
        auto saved=readObject(storage.path()+"/instances/test-instance/instance.json")["settings"].toObject();
        QCOMPARE(saved["width"].toInt(),1600);
        QVERIFY(saved["fullscreen"].toBool());
        QVERIFY(!saved["keepVisible"].toBool());
        launcher.saveInstance({{"settings",QVariantMap{{"width",1},{"height",900}}}});
        QCOMPARE(readObject(storage.path()+"/instances/test-instance/instance.json")["settings"].toObject(),saved);
        QVERIFY(notices.last().at(1).toBool());
    }
    void invalidInstallTargetDoesNotStartPreviewWork() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        launcher.loadInstallChoices({{"provider","Modrinth"},{"projectId","iris"},{"type","mod"}},"missing");
        QVERIFY(!launcher.busy());
        QVERIFY(!launcher.loadingInstallChoices());
        QVERIFY(launcher.jobs().isEmpty());
        QVERIFY(launcher.pendingPlan().isEmpty());
        QVERIFY(!launcher.installError().isEmpty());
        QVERIFY(!QFileInfo::exists(storage.path()+"/cache"));
    }
    void accentValidationAndPersistence() {
        QTemporaryDir storage;
        QVERIFY(storage.isValid());
        Launcher launcher(storage.path());
        QSignalSpy notices(&launcher, &Launcher::notice);
        QCOMPARE(launcher.settings().value("accentColor").toString(), QString("#8b5cf6"));

        for (const auto &invalid : QStringList{"", "red", "#fff", "#12345g", "123456", "#12345678"}) {
            launcher.setAccentColor(invalid);
            QCOMPARE(launcher.settings().value("accentColor").toString(), QString("#8b5cf6"));
        }
        QCOMPARE(notices.count(), 6);
        QVERIFY(!QFileInfo::exists(storage.path() + "/settings.json"));

        QSignalSpy changed(&launcher, &Launcher::changed);
        launcher.setAccentColor(" #3B82F6 ");
        QCOMPARE(launcher.settings().value("accentColor").toString(), QString("#3b82f6"));
        QCOMPARE(readObject(storage.path() + "/settings.json")["accentColor"].toString(), QString("#3b82f6"));
        QCOMPARE(changed.count(), 1);
        Launcher reloaded(storage.path());
        QCOMPARE(reloaded.settings().value("accentColor").toString(), QString("#3b82f6"));
    }

    void reducedMotionAppliesAndPersistsImmediately() {
        QTemporaryDir storage;
        QVERIFY(storage.isValid());
        Launcher launcher(storage.path());
        QSignalSpy changed(&launcher, &Launcher::changed);
        QVERIFY(!launcher.settings().value("reducedMotion").toBool());
        launcher.setReducedMotion(true);
        QVERIFY(launcher.settings().value("reducedMotion").toBool());
        QVERIFY(readObject(storage.path() + "/settings.json")["reducedMotion"].toBool());
        QCOMPARE(changed.count(), 1);
        Launcher reloaded(storage.path());
        QVERIFY(reloaded.settings().value("reducedMotion").toBool());
        launcher.setReducedMotion(true);
        QCOMPARE(changed.count(), 1);
        launcher.setReducedMotion(false);
        QVERIFY(!readObject(storage.path() + "/settings.json")["reducedMotion"].toBool());
        QCOMPARE(changed.count(), 2);
    }

    void invalidInstallTargetStaysInApplication() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        const QVariantMap project{
            {"provider", "Modrinth"}, {"projectId", "sodium"}, {"title", "Sodium"}, {"type", "mod"}};
        QSignalSpy failures(&launcher, &Launcher::installFailed);
        bool threw = false;
        try {
            launcher.beginInstall(project, "missing-instance");
        } catch (...) {
            threw = true;
        }
        QVERIFY(!threw);
        QVERIFY(!launcher.busy());
        QVERIFY(!launcher.installError().isEmpty());
        QCOMPARE(failures.count(), 1);
        QVERIFY(launcher.pendingPlan().isEmpty());
        QVERIFY(launcher.jobs().isEmpty());
    }

    void animatedArtworkAndBackgroundAreManaged() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto gif=QByteArray::fromHex("47494638396101000100800000000000ffffff21f904000a0000002c000000000100010000020244010021f904000a0000002c00000000010001000002024c01003b");
        const auto path=storage.path()+"/art.gif";writeBytes(path,gif);
        QImageReader reader(path);QCOMPARE(reader.imageCount(),2);
        Launcher launcher(storage.path());
        launcher.setInstanceIcon(QUrl::fromLocalFile(path).toString());
        launcher.setInstanceBackground(QUrl::fromLocalFile(path).toString());
        const auto selected=launcher.selected();
        QCOMPARE(selected["iconFile"].toString(),QString("icon.gif"));
        QCOMPARE(selected["backgroundFile"].toString(),QString("background.gif"));
        QVERIFY(selected["iconUrl"].toString().contains("icon.gif"));
        QVERIFY(selected["backgroundUrl"].toString().contains("background.gif"));
        Launcher restored(storage.path());QCOMPARE(restored.selected()["backgroundFile"].toString(),QString("background.gif"));
        launcher.resetInstanceIcon();launcher.resetInstanceBackground();
        QVERIFY(launcher.selected()["iconUrl"].toString().isEmpty());
        QVERIFY(launcher.selected()["backgroundUrl"].toString().isEmpty());
        QVERIFY(QFile::exists(path));
    }

    void personalizationPreferencesValidateAndPersist() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        launcher.setUiPreferences({{"pingColor","#22c55e"},{"animateArtwork",false},{"backgroundDim",0.72},{"unknown","ignored"}});
        QCOMPARE(launcher.settings()["pingColor"].toString(),QString("#22c55e"));
        QVERIFY(!launcher.settings()["animateArtwork"].toBool());
        QVERIFY(!launcher.settings().contains("unknown"));
        launcher.setUiPreferences({{"pingColor","red"}});
        QCOMPARE(launcher.settings()["pingColor"].toString(),QString("#22c55e"));
        Launcher restored(storage.path());QCOMPARE(restored.settings()["backgroundDim"].toDouble(),0.72);
    }

    void missingInstallTargetReportsInlineError() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        launcher.beginInstall({{"provider", "Modrinth"}, {"projectId", "sodium"}, {"type", "mod"}});
        QVERIFY(!launcher.busy());
        QVERIFY(launcher.installError().contains("instance", Qt::CaseInsensitive));
        QVERIFY(launcher.pendingPlan().isEmpty());
    }

    void dataPackCannotAccidentallyInstallAsMod() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        Launcher launcher(storage.path());
        launcher.beginInstall({{"provider", "Modrinth"}, {"projectId", "example"}, {"type", "datapack"}},
                              "test-instance");
        QVERIFY(launcher.installError().contains("world", Qt::CaseInsensitive));
        QVERIFY(!launcher.busy());
        QVERIFY(launcher.pendingPlan().isEmpty());
        QVERIFY(!QDir(storage.path() + "/instances/test-instance/game/mods").exists());
    }

    void unsafeRemovalIsCaughtBeforeReview() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto outside = storage.path() + "/outside.txt";
        writeBytes(outside, "keep me");
        Launcher launcher(storage.path());
        QSignalSpy notices(&launcher, &Launcher::notice);
        bool threw = false;
        try {
            launcher.removeFile("../../../outside.txt");
        } catch (...) {
            threw = true;
        }
        QVERIFY(!threw);
        QCOMPARE(notices.count(), 1);
        QVERIFY(notices[0][1].toBool());
        QVERIFY(launcher.pendingPlan().isEmpty());
        QFile preserved(outside);
        QVERIFY(preserved.open(QIODevice::ReadOnly));
        QCOMPARE(preserved.readAll(), QByteArray("keep me"));
    }

    void invalidSelectionCannotRetargetInstance() {
        QTemporaryDir storage;
        makeInstance(storage.path(), "first");
        makeInstance(storage.path(), "second");
        writeObject(storage.path() + "/settings.json",
                    {{"accentColor", "#8b5cf6"}, {"lastInstallInstance", "second"}});
        Launcher launcher(storage.path());
        launcher.selectInstance("first");
        launcher.selectInstance("not-an-instance");
        QCOMPARE(launcher.selectedId(), QString("first"));
        QCOMPARE(launcher.installTargetId(), QString("second"));
        launcher.beginInstall({{"provider", "Modrinth"}, {"projectId", "sodium"}, {"type", "mod"}},
                              "not-an-instance");
        QCOMPARE(launcher.selectedId(), QString("first"));
        QCOMPARE(launcher.installTargetId(), QString("second"));
        QVERIFY(!launcher.busy());
    }

    void contentExportOmitsSecretsAndLocalPaths() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        writeObject(
            storage.path() + "/instances/test-instance/content.json",
            {{"schemaVersion", 1},
             {"files", QJsonArray{QJsonObject{{"provider", "Modrinth"},
                                              {"projectId", "project"},
                                              {"versionId", "version"},
                                              {"path", "mods/example.jar"},
                                              {"type", "mod"},
                                              {"local", "C:/private/downloads/example.jar"},
                                              {"url", "https://example.test/private?token=secret-value"},
                                              {"access_token", "secret-value"}}}},
             {"pack", QJsonObject{{"private", "secret-value"}}}});
        Launcher launcher(storage.path());
        const auto destination = storage.path() + "/content list.json";
        launcher.exportContentList(QUrl::fromLocalFile(destination).toString());
        const auto exported = readObject(destination);
        QCOMPARE(exported["format"].toString(), QString("moly-content-list"));
        QCOMPARE(exported["name"].toString(), QString("Test instance"));
        QCOMPARE(exported["files"].toArray().size(), 1);
        QCOMPARE(exported["files"].toArray()[0].toObject()["path"].toString(), QString("mods/example.jar"));
        const auto serialized = QJsonDocument(exported).toJson();
        QVERIFY(!serialized.contains("C:/private"));
        QVERIFY(!serialized.contains("secret-value"));
        QVERIFY(!serialized.contains("access_token"));
        QVERIFY(!serialized.contains("https://"));
        QVERIFY(!exported.contains("pack"));
    }

    void worldListOnlyIncludesWorldDirectories() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto saves = storage.path() + "/instances/test-instance/game/saves";
        QDir().mkpath(saves + "/Survival");
        QDir().mkpath(saves + "/Creative");
        writeBytes(saves + "/Survival/level.dat", "test level");
        writeBytes(saves + "/readme.txt", "not a world directory");
        Launcher launcher(storage.path());
        const auto worlds = launcher.worldList();
        QCOMPARE(worlds.size(), 2);
        QCOMPARE(worlds[0].toMap().value("name").toString(), QString("Creative"));
        QCOMPARE(worlds[1].toMap().value("folder").toString(), QString("Survival"));
        QVERIFY(!worlds[0].toMap().value("lastModified").toString().isEmpty());
        QVERIFY(!worlds[0].toMap().contains("absolutePath"));
    }

    void createInstanceCanBePreparedWithoutNetwork() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        launcher.createInstance("Fresh instance", "1.20.1", "Vanilla", {}, false);
        QCOMPARE(launcher.instances().size(), 1);
        const auto selected = launcher.selected();
        QCOMPARE(selected["name"].toString(), QString("Fresh instance"));
        QVERIFY(!selected["installed"].toBool());
        QVERIFY(!launcher.busy());
        QVERIFY(launcher.taskInstanceId().isEmpty());
        QVERIFY(launcher.taskOperation().isEmpty());
        const auto id = selected["id"].toString();
        QVERIFY(QFileInfo::exists(storage.path() + "/instances/" + id + "/instance.json"));
        QVERIFY(!QFileInfo::exists(storage.path() + "/instances/" + id + "/installation.json"));
        // MOC exposes the four-argument convenience method, whose omitted fifth argument defaults true.
        QVERIFY(launcher.metaObject()->indexOfMethod("createInstance(QString,QString,QString,QString)") >= 0);
    }

    void instanceIconIsManagedResizedAndPersistent() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        QImage source(900, 600, QImage::Format_ARGB32);
        source.fill(QColor("#8b5cf6"));
        const auto sourcePath = storage.path() + "/original.png";
        QVERIFY(source.save(sourcePath));
        Launcher launcher(storage.path());
        launcher.setInstanceIcon(QUrl::fromLocalFile(sourcePath).toString());
        const auto iconPath = storage.path() + "/instances/test-instance/icon.png";
        const auto metadata = readObject(storage.path() + "/instances/test-instance/instance.json");
        QCOMPARE(metadata["iconFile"].toString(), QString("icon.png"));
        QVERIFY(!metadata["iconRevision"].toString().isEmpty());
        QCOMPARE(QImageReader(iconPath).size(), QSize(512, 341));
        const QUrl managedUrl(launcher.selected()["iconUrl"].toString());
        QCOMPARE(managedUrl.toLocalFile(), iconPath);
        QVERIFY(!managedUrl.query().isEmpty());
        QVERIFY(!QJsonDocument(metadata).toJson().contains(sourcePath.toUtf8()));
        Launcher reloaded(storage.path());
        QCOMPARE(reloaded.selected()["iconUrl"].toString(), managedUrl.toString());

        const auto firstUrl = launcher.selected()["iconUrl"].toString();
        source.fill(QColor("#3b82f6"));
        QVERIFY(source.save(sourcePath));
        launcher.setInstanceIcon(QUrl::fromLocalFile(sourcePath).toString());
        QVERIFY(launcher.selected()["iconUrl"].toString() != firstUrl);
        QVERIFY(QFileInfo::exists(sourcePath));
        launcher.resetInstanceIcon();
        QVERIFY(launcher.selected()["iconUrl"].toString().isEmpty());
        QVERIFY(!QFileInfo::exists(iconPath));
        QVERIFY(QFileInfo::exists(sourcePath));
    }

    void invalidInstanceIconsLeavePreviousIconUntouched() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        Launcher launcher(storage.path());
        QImage source(64, 64, QImage::Format_ARGB32);
        source.fill(Qt::red);
        const auto sourcePath = storage.path() + "/source.png";
        QVERIFY(source.save(sourcePath));
        launcher.setInstanceIcon(QUrl::fromLocalFile(sourcePath).toString());
        const auto originalUrl = launcher.selected()["iconUrl"].toString();
        const auto iconPath = storage.path() + "/instances/test-instance/icon.png";
        const auto originalHash = fileHash(iconPath);
        writeBytes(storage.path() + "/not-image.png", "not an image");
        QImage oversized(8193, 1, QImage::Format_ARGB32);
        oversized.fill(Qt::blue);
        QVERIFY(oversized.save(storage.path() + "/too-wide.png"));
        QSignalSpy notices(&launcher, &Launcher::notice);
        for (const auto &url :
             QStringList{"https://example.test/icon.png",
                         QUrl::fromLocalFile(storage.path() + "/missing.png").toString(),
                         QUrl::fromLocalFile(storage.path() + "/not-image.png").toString(),
                         QUrl::fromLocalFile(storage.path() + "/too-wide.png").toString()}) {
            launcher.setInstanceIcon(url);
            QCOMPARE(launcher.selected()["iconUrl"].toString(), originalUrl);
            QCOMPARE(fileHash(iconPath), originalHash);
        }
        QCOMPARE(notices.count(), 4);
        for (const auto &notice : notices)
            QVERIFY(notice[1].toBool());
    }

    void duplicateInstanceOwnsItsCopiedIcon() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        QImage source(64, 64, QImage::Format_ARGB32);
        source.fill(Qt::green);
        const auto sourcePath = storage.path() + "/source.png";
        QVERIFY(source.save(sourcePath));
        Launcher launcher(storage.path());
        launcher.setInstanceIcon(QUrl::fromLocalFile(sourcePath).toString());
        const auto originalHash = fileHash(storage.path() + "/instances/test-instance/icon.png");
        launcher.duplicateInstance();
        // Only the copy worker runs: deliberately avoid the GUI event loop and its startup catalog timer.
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        const auto directories =
            QDir(storage.path() + "/instances").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(directories.size(), 2);
        QString copiedId;
        for (const auto &id : directories)
            if (id != "test-instance")
                copiedId = id;
        QVERIFY(!copiedId.isEmpty());
        const auto duplicate = storage.path() + "/instances/" + copiedId;
        QCOMPARE(readObject(duplicate + "/instance.json")["iconFile"].toString(), QString("icon.png"));
        QCOMPARE(fileHash(duplicate + "/icon.png"), originalHash);
        QVERIFY(QFile::remove(storage.path() + "/instances/test-instance/icon.png"));
        QVERIFY(QFileInfo::exists(duplicate + "/icon.png"));
    }

    void paginationDeduplicatesWithoutSkippingOffsets() {
        QJsonArray first, next;
        for (int n = 0; n < 30; ++n) {
            first.append(QJsonObject{{"provider", "Modrinth"}, {"projectId", QString::number(n)}});
            next.append(QJsonObject{{"provider", "Modrinth"}, {"projectId", QString::number(n + 20)}});
        }
        const auto merged =
            mergeSearchPage(first, {{"items", next}, {"total", 90}, {"hasMore", true}}, 30, 30);
        QCOMPARE(merged["items"].toArray().size(), 50);
        QCOMPARE(merged["nextOffset"].toInt(),
                 60); // All 30 fetched positions were consumed, including overlaps.
        QVERIFY(merged["hasMore"].toBool());
        QCOMPARE(merged["items"].toArray()[0].toObject()["projectId"].toString(), QString("0"));
        QCOMPARE(merged["items"].toArray()[49].toObject()["projectId"].toString(), QString("49"));

        const auto last = mergeSearchPage(
            merged["items"].toArray(),
            {{"items", QJsonArray{QJsonObject{{"provider", "CurseForge"}, {"projectId", "20"}},
                                  QJsonObject{{"provider", "CurseForge"}, {"projectId", "20"}}}},
             {"total", 90},
             {"hasMore", true}},
            60, 30);
        QCOMPARE(last["items"].toArray().size(), 51); // Provider IDs have separate namespaces.
        QCOMPARE(last["nextOffset"].toInt(), 62);
        QVERIFY(!last["hasMore"].toBool()); // A short page always ends pagination.
    }

    void idlePaginationDoesNotStartRequests() {
        QTemporaryDir storage;
        Launcher launcher(storage.path());
        QSignalSpy results(&launcher, &Launcher::resultsChanged);
        launcher.loadMoreResults();
        launcher.loadMore();
        QVERIFY(!launcher.busy());
        QVERIFY(!launcher.searchLoading());
        QVERIFY(!launcher.canLoadMore());
        QVERIFY(launcher.results().isEmpty());
        QVERIFY(launcher.searchError().isEmpty());
        QVERIFY(launcher.jobs().isEmpty());
        QCOMPARE(results.count(), 0);
    }

    void searchQueuesNewIntentWithoutAddingDownloadJobs() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        Launcher launcher(storage.path());
        launcher.duplicateInstance();
        const auto jobsBefore = launcher.jobs().size();
        QVERIFY(launcher.busy());
        launcher.search("Modrinth", "mod", "first");
        launcher.search("Modrinth", "mod", "latest");
        QVERIFY(launcher.searchLoading());
        QVERIFY(!launcher.loadingMore());
        QVERIFY(!launcher.canLoadMore());
        QVERIFY(launcher.results().isEmpty());
        QCOMPARE(launcher.jobs().size(), jobsBefore);
        // Finish only the copy worker without delivering startup timers or starting the queued network query.
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
    }

    void managedModMetadataSurvivesInstallAndExternalDisable() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto source = storage.path() + "/source.jar";
        writeBytes(source, "verified local fixture");
        const QJsonObject file{{"provider", "Modrinth"},
                               {"projectId", "example"},
                               {"versionId", "version"},
                               {"title", "Example Mod"},
                               {"icon", "https://cdn.modrinth.com/example.png"},
                               {"author", "Example Author"},
                               {"path", "mods/example.jar"},
                               {"local", source},
                               {"hash", fileHash(source, "sha512")},
                               {"algorithm", "sha512"}};
        Content content(storage.path());
        content.apply({{"id", "test-instance"}}, {{"files", QJsonArray{file}}});
        auto files = content.files({{"id", "test-instance"}}, "mod");
        QCOMPARE(files.size(), 1);
        QCOMPARE(files[0].toObject()["title"].toString(), QString("Example Mod"));
        QCOMPARE(files[0].toObject()["icon"].toString(), QString("https://cdn.modrinth.com/example.png"));
        QCOMPARE(files[0].toObject()["author"].toString(), QString("Example Author"));
        const auto path = storage.path() + "/instances/test-instance/game/mods/example.jar";
        QVERIFY(QFile::rename(path, path + ".disabled"));
        files = content.files({{"id", "test-instance"}}, "mod");
        QCOMPARE(files[0].toObject()["title"].toString(), QString("Example Mod"));
        QCOMPARE(files[0].toObject()["icon"].toString(), QString("https://cdn.modrinth.com/example.png"));
        QVERIFY(!files[0].toObject()["enabled"].toBool());
    }

    void localFabricModHasCachedEmbeddedLogo() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto fixture = QByteArray::fromBase64(
            "UEsDBBQAAAAIADecWl2S1wLpYwAAAIAAAAAPAAAAZmFicmljLm1vZC5qc29uLcwxC4AgEIbhvyI3R9La1l5rS0QcKhWoF55N"
            "0X/Pk7aP54P3ATaHCzi7xCdF6FXXKDhtGeDJoN887QTFIgYnOoqqsaiayMqDdz4ocTmX/"
            "x2qwCopU6uAzC6zrk0tzfaKO7wfUEsDBBQAAAAIADecWl3ShdjbPwAAAEYAAAAVAAAAYXNzZXRzL2xvY2FsL2xvZ28ucG5n6"
            "wzwc+flkuJiYGDg9fRwCQLSjCDMwQYk5UWPdIIlXBxDKuYkv+j7+"
            "U2egc2A6W6YfF8LUILB09XPZZ1TQhMAUEsBAhQAFAAAAAgAN5xaXZLXAuljAAAAgAAAAA8AAAAAAAAAAAAAAIABAAAAAGZhY"
            "nJpYy5tb2QuanNvblBLAQIUABQAAAAIADecWl3ShdjbPwAAAEYAAAAVAAAAAAAAAAAAAACAAZAAAABhc3NldHMvbG9jYWwvb"
            "G9nby5wbmdQSwUGAAAAAAIAAgCAAAAAAgEAAAAA");
        writeBytes(storage.path() + "/instances/test-instance/game/mods/local.jar", fixture);
        Content content(storage.path());
        const auto files = content.files({{"id", "test-instance"}}, "mod");
        QCOMPARE(files.size(), 1);
        const auto mod = files[0].toObject();
        QCOMPARE(mod["title"].toString(), QString("Local Logo Mod"));
        QCOMPARE(mod["author"].toString(), QString("Local Author"));
        const auto logo = QUrl(mod["icon"].toString()).toLocalFile();
        QVERIFY(logo.startsWith(storage.path() + "/cache/mod-logos/"));
        QVERIFY(QFileInfo(logo).isFile());
        QVERIFY(!QImage(logo).isNull());
        const auto timestamp = QFileInfo(logo).lastModified();
        QCOMPARE(content.files({{"id", "test-instance"}}, "mod"), files);
        QCOMPARE(QFileInfo(logo).lastModified(), timestamp);
    }

    void unsafeEmbeddedLogoCannotEscapeCache() {
        QTemporaryDir storage;
        makeInstance(storage.path());
        const auto fixture = QByteArray::fromBase64(
            "UEsDBBQAAAAIAC+"
            "cWl0D4ODHYQAAAHkAAAAPAAAAZmFicmljLm1vZC5qc29uLcyxCoAgFIXhV5E7h9Ha1m5rS0RcVFRQb6hN0bun0nb4fjgPZGl"
            "1wE2n7CjCzKaBgVN1gCeJ/vRkCKpFDLqpaMpEVbaSagXvYinlGve/"
            "Ll3gaFeyvwLnI90lO6X5FQ28H1BLAQIUABQAAAAIAC+"
            "cWl0D4ODHYQAAAHkAAAAPAAAAAAAAAAAAAACAAQAAAABmYWJyaWMubW9kLmpzb25QSwUGAAAAAAEAAQA9AAAAjgAAAAAA");
        writeBytes(storage.path() + "/instances/test-instance/game/mods/unsafe.jar", fixture);
        Content content(storage.path());
        const auto files = content.files({{"id", "test-instance"}}, "mod");
        QCOMPARE(files.size(), 1);
        QVERIFY(files[0].toObject()["icon"].toString().isEmpty());
        QVERIFY(!QFileInfo::exists(storage.path() + "/outside.png"));
    }
};

QTEST_GUILESS_MAIN(LauncherTests)
#include "launcher_tests.moc"
