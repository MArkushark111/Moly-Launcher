// Opt-in live Minecraft creation/install regression. Never registered with CTest.
// Usage: creation_integration <isolated-data-dir> [minecraft-version]
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

#include "core.h"
#include "launcher.h"

using namespace moly;

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Moly");
    app.setApplicationName("Moly creation integration");
    const auto args = app.arguments();
    QTextStream output(stdout);
    if (args.size() < 2) {
        output << "Usage: creation_integration <isolated-data-dir> [minecraft-version]\n";
        return 2;
    }

    try {
        const auto root = QDir(args[1]).absolutePath();
        const auto version = args.value(2, "1.20.1");
        const auto name = "Creation regression " + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        Launcher launcher(root);
        QSet<QString> existingIds;
        for (const auto &instance : launcher.instances())
            existingIds.insert(instance.toMap()["id"].toString());

        enum class Phase { Catalog, Scheduled, Creating, Installing, Finished };
        Phase phase = Phase::Catalog;
        QString createdId;
        bool catalogWasBusy = false;
        bool installWasBusy = false;
        auto fail = [&](const QString &message) {
            if (phase == Phase::Finished)
                return;
            phase = Phase::Finished;
            launcher.cancel();
            output << redact(message) << Qt::endl;
            app.exit(1);
        };

        QObject::connect(&launcher, &Launcher::installFailed, &app, fail);
        QObject::connect(&launcher, &Launcher::busyChanged, &app, [&] {
            if (phase == Phase::Catalog) {
                if (launcher.busy()) {
                    catalogWasBusy = true;
                    return;
                }
                if (!catalogWasBusy)
                    return;
                phase = Phase::Scheduled;
                // Do not re-enter the catalog completion handler's future watcher.
                QTimer::singleShot(0, &app, [&] {
                    if (phase != Phase::Scheduled)
                        return;
                    phase = Phase::Creating;
                    const QString loader = "Vanilla";
                    const QString loaderVersion;
                    // Four arguments match the QML call; MOC supplies autoInstall=true.
                    const bool invoked = QMetaObject::invokeMethod(
                        &launcher, "createInstance", Qt::DirectConnection, Q_ARG(QString, name),
                        Q_ARG(QString, version), Q_ARG(QString, loader), Q_ARG(QString, loaderVersion));
                    if (!invoked) {
                        fail("The four-argument create-instance invocation was not available.");
                        return;
                    }
                    if (phase != Phase::Finished && (!launcher.busy() || !installWasBusy))
                        fail("Creating an instance did not automatically start game installation.");
                });
                return;
            }

            if (phase == Phase::Creating && launcher.busy()) {
                createdId = launcher.selectedId();
                if (createdId.isEmpty() || existingIds.contains(createdId)) {
                    fail("Creation did not select a new instance.");
                    return;
                }
                if (launcher.taskInstanceId() != createdId || launcher.taskOperation() != "install") {
                    fail("The automatically started task did not target the new instance.");
                    return;
                }
                if (launcher.selected()["installed"].toBool()) {
                    fail("A new instance was marked installed before its task completed.");
                    return;
                }
                installWasBusy = true;
                phase = Phase::Installing;
                return;
            }

            if (phase != Phase::Installing || launcher.busy())
                return;
            phase = Phase::Scheduled;
            QTimer::singleShot(0, &app, [&] {
                if (phase != Phase::Scheduled)
                    return;
                try {
                    if (!installWasBusy || launcher.selectedId() != createdId)
                        throw Error("The selected instance changed during automatic installation.");
                    if (!launcher.taskInstanceId().isEmpty() || !launcher.taskOperation().isEmpty())
                        throw Error("Completed installation left active task metadata behind.");
                    const auto dir = safePath(root + "/instances", createdId);
                    const auto instance = readObject(dir + "/instance.json");
                    const auto state = readObject(dir + "/installation.json");
                    if (instance["name"] != name || state["instanceVersion"] != version ||
                        !launcher.selected()["installed"].toBool())
                        throw Error("The newly created instance did not finish installing correctly.");
                    if (!QFileInfo(state["java"].toString()).isFile() ||
                        state["profile"].toObject()["mainClass"].toString().isEmpty())
                        throw Error("The installation is missing Java or its main class.");
                    const auto classpath = state["classpath"].toArray();
                    if (classpath.isEmpty())
                        throw Error("The installed classpath is empty.");
                    for (const auto &path : classpath)
                        if (!QFileInfo(path.toString()).isFile())
                            throw Error("An installed library is missing.");
                    phase = Phase::Finished;
                    output << "Created and installed Minecraft " << version << " automatically." << Qt::endl;
                    app.exit(0);
                } catch (const std::exception &error) {
                    fail(QString::fromUtf8(error.what()));
                }
            });
        });

        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(60000);
        QObject::connect(&timeout, &QTimer::timeout, &app,
                         [&] { fail("Automatic instance installation timed out after 60 seconds."); });
        timeout.start();
        return app.exec();
    } catch (const std::exception &error) {
        output << redact(QString::fromUtf8(error.what())) << Qt::endl;
        return 1;
    }
}
