// Opt-in live-service regression helper. This is deliberately not registered with CTest.
// Usage: moly_install_integration <isolated-data-dir> [Modrinth|CurseForge] [project-id] [loader]
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

#include "core.h"
#include "launcher.h"

using namespace moly;

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Moly");
    app.setApplicationName("Moly install integration");
    const auto args = app.arguments();
    QTextStream output(stdout);
    if (args.size() < 2) {
        output << "Usage: moly_install_integration <isolated-data-dir> [Modrinth|CurseForge] "
                  "[project-id] [loader]\n";
        return 2;
    }

    try {
        const auto root = QDir(args[1]).absolutePath();
        const auto provider = args.value(2, "Modrinth");
        if (provider != "Modrinth" && provider != "CurseForge")
            throw Error("Provider must be Modrinth or CurseForge.");
        const auto projectId = args.value(3, provider == "CurseForge" ? "238222" : "iris");
        const auto loader = args.value(4, "Fabric");
        const auto id = "install-check-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto instanceDir = safePath(root + "/instances", id);
        writeObject(instanceDir + "/instance.json", {{"schemaVersion", 1},
                                                     {"id", id},
                                                     {"name", "Install regression " + provider},
                                                     {"version", "1.20.1"},
                                                     {"loader", loader},
                                                     {"settings", QJsonObject{}}});
        if (!QDir().mkpath(instanceDir + "/game"))
            throw Error("Cannot create the isolated game folder.");

        Launcher launcher(root);
        enum class Phase { Catalog, Scheduled, Resolving, Applying, Finished };
        Phase phase = Phase::Catalog;
        bool catalogWasBusy = false;
        bool resolvingWasBusy = false;
        bool applyingWasBusy = false;
        QJsonArray expectedFiles;
        quint64 heartbeatCount = 0;
        quint64 resolutionHeartbeat = 0;
        quint64 applicationHeartbeat = 0;

        auto fail = [&](const QString &message) {
            if (phase == Phase::Finished)
                return;
            phase = Phase::Finished;
            launcher.cancel();
            output << redact(message) << Qt::endl;
            app.exit(1);
        };

        QTimer heartbeat;
        heartbeat.setInterval(5);
        QObject::connect(&heartbeat, &QTimer::timeout, &app, [&] { ++heartbeatCount; });
        heartbeat.start();

        QObject::connect(&launcher, &Launcher::installFailed, &app, fail);
        QObject::connect(&launcher, &Launcher::busyChanged, &app, [&] {
            if (phase == Phase::Catalog && launcher.busy())
                catalogWasBusy = true;
            else if (phase == Phase::Resolving && launcher.busy())
                resolvingWasBusy = true;
            else if (phase == Phase::Applying && launcher.busy())
                applyingWasBusy = true;

            if (phase != Phase::Catalog || launcher.busy() || !catalogWasBusy)
                return;
            phase = Phase::Scheduled;
            // Let the catalog's completion callback unwind before reusing its future watcher.
            QTimer::singleShot(0, &app, [&] {
                if (phase != Phase::Scheduled)
                    return;
                phase = Phase::Resolving;
                resolutionHeartbeat = heartbeatCount;
                const auto title = projectId == "iris" ? "Iris" : projectId == "238222" ? "JEI" : projectId;
                launcher.beginInstall(
                    {{"provider", provider}, {"projectId", projectId}, {"title", title}, {"type", "mod"}},
                    id);
            });
        });

        QObject::connect(&launcher, &Launcher::planChanged, &app, [&] {
            if (phase != Phase::Resolving)
                return;
            const auto plan = QJsonObject::fromVariantMap(launcher.pendingPlan());
            const auto files = plan["files"].toArray();
            if (files.isEmpty())
                return;
            if (plan["targetId"].toString() != id) {
                fail("Install review targeted the wrong isolated instance.");
                return;
            }
            for (auto value : files) {
                const auto file = value.toObject();
                if (file["url"].toString().isEmpty() && file["local"].toString().isEmpty()) {
                    fail("This project requires a browser download; the headless install cannot continue.");
                    return;
                }
            }
            if (!resolvingWasBusy || heartbeatCount <= resolutionHeartbeat) {
                fail("The application's event loop did not remain responsive during dependency resolution.");
                return;
            }
            expectedFiles = files;
            phase = Phase::Scheduled;
            // Applying from planChanged synchronously would re-enter the resolving task's callback.
            QTimer::singleShot(0, &app, [&] {
                if (phase != Phase::Scheduled)
                    return;
                phase = Phase::Applying;
                applicationHeartbeat = heartbeatCount;
                launcher.applyPlan();
            });
        });

        QObject::connect(&launcher, &Launcher::installCompleted, &app, [&](const QString &message) {
            if (phase != Phase::Applying) {
                fail("Install completed outside the expected application phase.");
                return;
            }
            try {
                if (!applyingWasBusy || heartbeatCount <= applicationHeartbeat)
                    throw Error(
                        "The application's event loop did not remain responsive during installation.");
                const auto manifest = readObject(instanceDir + "/content.json")["files"].toArray();
                if (manifest.size() != expectedFiles.size())
                    throw Error("Installed content manifest does not match the reviewed file list.");
                for (auto value : expectedFiles) {
                    const auto file = value.toObject();
                    const auto path = safePath(instanceDir + "/game", file["path"].toString());
                    if (!QFileInfo(path).isFile() || QFileInfo(path).size() <= 0)
                        throw Error("An installed content file is missing or empty.");
                    if (file["hash"].toString().isEmpty() ||
                        fileHash(path, file["algorithm"].toString("sha1")) != file["hash"].toString())
                        throw Error("An installed content file failed checksum verification.");
                }
                if (QFileInfo::exists(instanceDir + "/transaction.json"))
                    throw Error("The content transaction did not finish cleanly.");
                phase = Phase::Finished;
                output << redact(message) << Qt::endl;
                app.exit(0);
            } catch (const std::exception &error) {
                fail(QString::fromUtf8(error.what()));
            }
        });

        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(60000);
        QObject::connect(&timeout, &QTimer::timeout, &app,
                         [&] { fail("The live install regression timed out after 60 seconds."); });
        timeout.start();
        return app.exec();
    } catch (const std::exception &error) {
        output << redact(QString::fromUtf8(error.what())) << Qt::endl;
        return 1;
    }
}
