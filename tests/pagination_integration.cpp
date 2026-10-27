// Opt-in live provider pagination check. Does not launch or control a GUI.
// Usage: pagination_integration <isolated-data-dir> [Modrinth|CurseForge]
#include <QCoreApplication>
#include <QDir>
#include <QSet>
#include <QTextStream>
#include <QTimer>

#include "core.h"
#include "launcher.h"

using namespace moly;

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Moly");
    app.setApplicationName("Moly pagination integration");
    QTextStream output(stdout);
    const auto args = app.arguments();
    if (args.size() < 2) {
        output << "Usage: pagination_integration <isolated-data-dir> [Modrinth|CurseForge]\n";
        return 2;
    }
    try {
        const auto provider = args.value(2, "Modrinth");
        if (provider != "Modrinth" && provider != "CurseForge")
            throw Error("Choose Modrinth or CurseForge.");
        Launcher launcher(QDir(args[1]).absolutePath());
        enum class Phase { Catalog, Scheduled, FirstPage, More, Finished };
        Phase phase = Phase::Catalog;
        bool catalogWasBusy = false;
        QVariantList firstPage;
        qsizetype jobsBeforeSearch = 0;
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
            if (phase != Phase::Catalog)
                return;
            if (launcher.busy()) {
                catalogWasBusy = true;
                return;
            }
            if (!catalogWasBusy)
                return;
            phase = Phase::Scheduled;
            jobsBeforeSearch = launcher.jobs().size();
            QTimer::singleShot(0, &app, [&] {
                if (phase != Phase::Scheduled)
                    return;
                phase = Phase::FirstPage;
                launcher.search(provider, "mod", {}, "Popular");
            });
        });
        QObject::connect(&launcher, &Launcher::searchStateChanged, &app, [&] {
            if ((phase == Phase::FirstPage || phase == Phase::More) && !launcher.searchError().isEmpty())
                fail(launcher.searchError());
        });
        QObject::connect(&launcher, &Launcher::resultsChanged, &app, [&] {
            const auto rows = launcher.results();
            if (phase == Phase::FirstPage && !rows.isEmpty()) {
                if (rows.size() != 30 || !launcher.canLoadMore()) {
                    fail("The first popular-mod page did not provide 30 results and another page.");
                    return;
                }
                firstPage = rows;
                phase = Phase::Scheduled;
                QTimer::singleShot(0, &app, [&] {
                    if (phase != Phase::Scheduled)
                        return;
                    phase = Phase::More;
                    launcher.loadMoreResults();
                    if (!launcher.loadingMore() || !launcher.searchLoading())
                        fail("Loading another page did not expose its loading state.");
                });
                return;
            }
            if (phase != Phase::More)
                return;
            if (rows.size() <= firstPage.size()) {
                fail("The second provider page did not append new projects.");
                return;
            }
            for (qsizetype index = 0; index < firstPage.size(); ++index) {
                if (rows[index].toMap()["provider"] != firstPage[index].toMap()["provider"] ||
                    rows[index].toMap()["projectId"] != firstPage[index].toMap()["projectId"]) {
                    fail("Appending a page changed the existing result order.");
                    return;
                }
            }
            QSet<QString> identifiers;
            for (const auto &value : rows) {
                const auto item = value.toMap();
                const auto key = item["provider"].toString() + '/' + item["projectId"].toString();
                if (item["projectId"].toString().isEmpty() || identifiers.contains(key)) {
                    fail("Paginated provider results contain an empty or duplicate project ID.");
                    return;
                }
                identifiers.insert(key);
            }
            if (launcher.jobs().size() != jobsBeforeSearch) {
                fail("Browsing projects added entries to download history.");
                return;
            }
            if (launcher.searchLoading()) {
                fail("The completed provider page left its loading indicator active.");
                return;
            }
            phase = Phase::Finished;
            output << provider << ": loaded 30 then " << rows.size()
                   << " unique projects; first-page order preserved." << Qt::endl;
            app.exit(0);
        });

        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(60000);
        QObject::connect(&timeout, &QTimer::timeout, &app,
                         [&] { fail("Provider pagination timed out after 60 seconds."); });
        timeout.start();
        return app.exec();
    } catch (const std::exception &error) {
        output << redact(QString::fromUtf8(error.what())) << Qt::endl;
        return 1;
    }
}
