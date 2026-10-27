#include <QtTest>
#include <QDir>
#include <QJsonArray>
#include <QTemporaryDir>

#include "core.h"
#include "minecraft.h"

using namespace moly;

namespace {
QJsonObject conditional(const QString &feature, const QJsonArray &arguments) {
    return {{"rules", QJsonArray{QJsonObject{{"action", "allow"},
                                              {"features", QJsonObject{{feature, true}}}}}},
            {"value", arguments}};
}

// Only command construction is exercised. The fake executable and JAR are
// existing files so this fixture can never start Java or contact a service.
struct Fixture {
    QTemporaryDir storage;
    QJsonObject state, instance, settings, account;

    Fixture() {
        const auto root = storage.path();
        const auto java = root + "/runtime/java.exe";
        const auto jar = root + "/libraries/client.jar";
        writeBytes(java, "command construction fixture; not executable");
        writeBytes(jar, "command construction fixture; not a real JAR");
        QDir().mkpath(root + "/game");
        const QJsonArray game{
            "--username", "${auth_player_name}", "--version", "${version_name}",
            conditional("has_custom_resolution", {"--width", "${resolution_width}",
                                                   "--height", "${resolution_height}"}),
            conditional("has_quick_plays_support", {"--quickPlayPath", "${quickPlayPath}"}),
            conditional("is_quick_play_singleplayer", {"--quickPlaySingleplayer", "${quickPlaySingleplayer}"}),
            conditional("is_quick_play_multiplayer", {"--quickPlayMultiplayer", "${quickPlayMultiplayer}"})};
        const QJsonObject profile{{"id", "1.20.1"}, {"mainClass", "test.minecraft.Main"},
                                  {"arguments", QJsonObject{{"game", game},
                                                             {"jvm", QJsonArray{"-cp", "${classpath}"}}}}};
        state = {{"profile", profile}, {"classpath", QJsonArray{jar}}, {"java", java},
                 {"javaMajor", 17}, {"gameDirectory", root + "/game"},
                 {"assets", root + "/assets"}, {"natives", root + "/natives"}};
        instance = {{"id", "test-instance"}, {"settings", QJsonObject{}}};
        settings = {{"memoryMb", 2048}, {"width", 1440}, {"height", 900}, {"fullscreen", false}};
        account = {{"name", "MolyTest"}, {"uuid", offlineUuid("MolyTest")}};
    }

    void target(const QString &type, const QString &value) {
        instance["_quickPlay"] = QJsonObject{{"type", type}, {"value", value}};
    }

    void removeProfileFlag(const QString &flag) {
        auto profile = state["profile"].toObject();
        auto arguments = profile["arguments"].toObject();
        const auto original = arguments["game"].toArray();
        QJsonArray game;
        for (const auto &argument : original) {
            const auto values = argument.toObject()["value"].toArray();
            if (!values.contains(flag))
                game.append(argument);
        }
        arguments["game"] = game;
        profile["arguments"] = arguments;
        state["profile"] = profile;
    }

    LaunchCommand command() const {
        return Minecraft::command(state, instance, settings, account, {});
    }
};

QStringList withoutQuickPlayArguments(const QStringList &arguments) {
    QStringList result;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--quickPlayPath" || arguments[i] == "--quickPlaySingleplayer" ||
            arguments[i] == "--quickPlayMultiplayer") {
            ++i;
        } else {
            result.append(arguments[i]);
        }
    }
    return result;
}
}

class QuickPlayTests : public QObject {
    Q_OBJECT
private slots:
    void ordinaryPlayHasNoQuickPlayArguments() {
        Fixture fixture;
        QVERIFY(fixture.storage.isValid());
        const auto command = fixture.command();
        QVERIFY(!command.arguments.contains("--quickPlayPath"));
        QVERIFY(!command.arguments.contains("--quickPlaySingleplayer"));
        QVERIFY(!command.arguments.contains("--quickPlayMultiplayer"));
        for (const auto &argument : command.arguments)
            QVERIFY2(!argument.contains("${"), qPrintable(argument));
        QCOMPARE(command.program, fixture.state["java"].toString());
        QCOMPARE(command.workingDirectory, fixture.state["gameDirectory"].toString());
    }

    void exactTargetAndMutuallyExclusiveFlags_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("value");
        QTest::addColumn<QString>("expectedFlag");
        QTest::addColumn<QString>("excludedFlag");
        QTest::newRow("world with spaces") << "world" << "My Survival World"
                                           << "--quickPlaySingleplayer" << "--quickPlayMultiplayer";
        QTest::newRow("server with port") << "server" << "play.example.test:25570"
                                          << "--quickPlayMultiplayer" << "--quickPlaySingleplayer";
        QTest::newRow("IPv6 server") << "server" << "[2001:db8::1]:25570"
                                     << "--quickPlayMultiplayer" << "--quickPlaySingleplayer";
    }

    void exactTargetAndMutuallyExclusiveFlags() {
        QFETCH(QString, type);
        QFETCH(QString, value);
        QFETCH(QString, expectedFlag);
        QFETCH(QString, excludedFlag);
        Fixture fixture;
        fixture.target(type, value);
        const auto args = fixture.command().arguments;
        QCOMPARE(args.count(expectedFlag), 1);
        const auto targetIndex = args.indexOf(expectedFlag);
        QVERIFY(targetIndex + 1 < args.size());
        QCOMPARE(args[targetIndex + 1], value);
        QVERIFY(!args.contains(excludedFlag));
        QCOMPARE(args.count("--quickPlayPath"), 1);
        QCOMPARE(args[args.indexOf("--quickPlayPath") + 1], QString("moly-quickplay.json"));
        if (type == "world") {
            QVERIFY(!args.contains("My"));
            QVERIFY(!args.contains("Survival"));
            QVERIFY(!args.contains("World"));
        }
    }

    void unsupportedProfileReportsUsefulError_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("missingFlag");
        QTest::newRow("missing singleplayer support") << "world" << "--quickPlaySingleplayer";
        QTest::newRow("missing multiplayer support") << "server" << "--quickPlayMultiplayer";
    }

    void unsupportedProfileReportsUsefulError() {
        QFETCH(QString, type);
        QFETCH(QString, missingFlag);
        Fixture fixture;
        fixture.removeProfileFlag(missingFlag);
        fixture.target(type, type == "world" ? "Saved World" : "localhost:25565");
        bool failed = false;
        try {
            fixture.command();
        } catch (const Error &error) {
            failed = true;
            const auto message = QString::fromUtf8(error.what());
            QVERIFY(message.contains("does not support", Qt::CaseInsensitive));
            QVERIFY(message.contains("Play"));
        }
        QVERIFY(failed);
        // Unsupported direct entry must not prevent an ordinary launch.
        fixture.instance.remove("_quickPlay");
        QVERIFY(!fixture.command().arguments.contains(missingFlag));
    }

    void quickPlayPreservesMemoryResolutionAndOtherArguments() {
        Fixture fixture;
        fixture.settings["fullscreen"] = true;
        fixture.settings["jvmArgs"] = "-Dmoly.test=true";
        fixture.instance["settings"] = QJsonObject{{"memoryMb", 1024}};
        const auto normal = fixture.command();
        fixture.target("world", "My Survival World");
        const auto quick = fixture.command();
        QCOMPARE(quick.program, normal.program);
        QCOMPARE(quick.workingDirectory, normal.workingDirectory);
        QCOMPARE(withoutQuickPlayArguments(quick.arguments), normal.arguments);
        QVERIFY(quick.arguments.contains("-Xms512M"));
        QVERIFY(quick.arguments.contains("-Xmx1024M"));
        QVERIFY(!quick.arguments.contains("-Xmx2048M"));
        QCOMPARE(quick.arguments[quick.arguments.indexOf("--width") + 1], QString("1440"));
        QCOMPARE(quick.arguments[quick.arguments.indexOf("--height") + 1], QString("900"));
        QVERIFY(quick.arguments.contains("--fullscreen"));
        QVERIFY(quick.arguments.contains("-Dmoly.test=true"));
    }
};

QTEST_GUILESS_MAIN(QuickPlayTests)
#include "quickplay_tests.moc"
