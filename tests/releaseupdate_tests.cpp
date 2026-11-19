#include "releaseupdates.h"
#include "core.h"
#include <QtTest>
#include <QJsonArray>
using namespace moly;

class ReleaseUpdateTests : public QObject {
    Q_OBJECT
    static QJsonObject fixture(QString tag = "v0.8.5") {
        return {{"tag_name", tag}, {"name", "Moly update"}, {"body", "New features\n\nFixes & improvements"},
            {"draft", false}, {"prerelease", false},
            {"html_url", "https://github.com/MArkushark111/Moly-Launcher/releases/tag/" + tag},
            {"assets", QJsonArray{QJsonObject{{"name", "MolyLauncher-0.8.5-Windows-x64-Setup.exe"},
                {"state", "uploaded"}, {"size", 123456},
                {"browser_download_url", "https://github.com/MArkushark111/Moly-Launcher/releases/download/v0.8.5/MolyLauncher-0.8.5-Windows-x64-Setup.exe"}}}}};
    }
private slots:
    void newerReleaseIncludesNotesAndInstaller() {
        const auto result = parseLauncherRelease(fixture(), "0.8.4");
        QVERIFY(result["available"].toBool());
        QCOMPARE(result["version"].toString(), "0.8.5");
        QCOMPARE(result["notes"].toString(), "New features\n\nFixes & improvements");
        QVERIFY(result["installerUrl"].toString().endsWith("Setup.exe"));
    }
    void numericComparisonPreventsDowngrades() {
        QVERIFY(!parseLauncherRelease(fixture(), "0.8.5")["available"].toBool());
        QVERIFY(!parseLauncherRelease(fixture(), "0.9.0")["available"].toBool());
        QVERIFY(parseLauncherRelease(fixture("v0.10.0"), "0.9.9")["available"].toBool());
    }
    void existingRepositoryReleaseNamingIsSupported() {
        auto release = fixture("Release"); release["name"] = "MolyLauncher 0.8.5";
        auto asset = release["assets"].toArray().first().toObject();
        asset["name"] = "MolyInstall.exe";
        asset["browser_download_url"] = "https://github.com/MArkushark111/Moly-Launcher/releases/download/Release/MolyInstall.exe";
        release["assets"] = QJsonArray{asset};
        const auto result = parseLauncherRelease(release, "0.8.4");
        QVERIFY(result["available"].toBool()); QCOMPARE(result["version"].toString(), "0.8.5");
        QVERIFY(result["installerUrl"].toString().endsWith("MolyInstall.exe"));
        QVERIFY(!parseLauncherRelease(release, "0.8.5")["available"].toBool());
    }
    void unfinishedAndInvalidVersionsAreRejected() {
        for (const auto &flag : QStringList{"draft", "prerelease"}) {
            auto release = fixture(); release[flag] = true;
            QVERIFY_EXCEPTION_THROWN(parseLauncherRelease(release, "0.8.4"), Error);
        }
        for (const auto &tag : QStringList{"latest", "v0.8.5-beta.1", "v0.8.5evil", "", "999999999999999999.0.0"})
            QVERIFY_EXCEPTION_THROWN(parseLauncherRelease(fixture(tag), "0.8.4"), Error);
    }
    void unrelatedAssetsCannotBeOfferedAsUpdates() {
        for (const auto &url : QStringList{
            "https://github.com.evil.test/MArkushark111/Moly-Launcher/releases/download/v0.8.5/MolyLauncher-0.8.5-Windows-x64-Setup.exe",
            "http://github.com/MArkushark111/Moly-Launcher/releases/download/v0.8.5/MolyLauncher-0.8.5-Windows-x64-Setup.exe",
            "https://github.com/other/repository/releases/download/v0.8.5/MolyLauncher-0.8.5-Windows-x64-Setup.exe",
            "https://evil@github.com/MArkushark111/Moly-Launcher/releases/download/v0.8.5/MolyLauncher-0.8.5-Windows-x64-Setup.exe"}) {
            auto release = fixture(); auto asset = release["assets"].toArray().first().toObject();
            asset["browser_download_url"] = url; release["assets"] = QJsonArray{asset};
            const auto result = parseLauncherRelease(release, "0.8.4");
            QVERIFY(result["available"].toBool()); QVERIFY(result["installerUrl"].toString().isEmpty());
        }
        auto release = fixture(); release["assets"] = QJsonArray{};
        QVERIFY(parseLauncherRelease(release, "0.8.4")["installerUrl"].toString().isEmpty());
    }
    void unexpectedReleasePageIsRejected() {
        auto release = fixture(); release["html_url"] = "https://example.com/download";
        QVERIFY_EXCEPTION_THROWN(parseLauncherRelease(release, "0.8.4"), Error);
    }
    void openingBeforeDiscoveryDoesNothing() {
        ReleaseUpdates updater;
        QVERIFY(!updater.available()); QVERIFY(!updater.checking()); QVERIFY(!updater.openUpdate());
    }
};
QTEST_GUILESS_MAIN(ReleaseUpdateTests)
#include "releaseupdate_tests.moc"
