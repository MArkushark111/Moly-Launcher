#include "minecraft.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QVersionNumber>
#include <QXmlStreamReader>

namespace moly {
static QString enc(const QString &s) {
    return QString::fromLatin1(QUrl::toPercentEncoding(s));
}
QJsonArray Minecraft::versions(bool snapshots) {
    auto manifest = net.json("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json").object();
    writeObject(root + "/cache/version-manifest.json", manifest);
    QJsonArray out;
    for (auto v : manifest["versions"].toArray()) {
        auto o = v.toObject();
        if (o["type"] == "release" || (snapshots && o["type"] == "snapshot"))
            out.append(o);
    }
    return out;
}
QJsonObject Minecraft::gameVersion(const QString &v) {
    const auto path = safePath(root + "/cache/game/versions", v + '/' + v + ".json");
    auto manifest = net.json("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json").object();
    for (auto entry : manifest["versions"].toArray()) {
        auto o = entry.toObject();
        if (o["id"] != v)
            continue;
        net.downloads({{o["url"].toString(), path, o["sha1"].toString()}});
        return readObject(path);
    }
    throw Error("Minecraft version not found: " + v);
}
QStringList Minecraft::loaders(const QString &game, const QString &loader) {
    QStringList result;
    if (loader == "Vanilla")
        return result;
    if (loader == "Fabric" || loader == "Quilt") {
        const QString base =
            loader == "Fabric" ? "https://meta.fabricmc.net/v2" : "https://meta.quiltmc.org/v3";
        for (auto v : net.json(base + "/versions/loader/" + enc(game)).array()) {
            auto o = v.toObject()["loader"].toObject();
            if (o.contains("version"))
                result << o["version"].toString();
        }
    } else {
        const QString url =
            loader == "Forge"
                ? "https://maven.minecraftforge.net/net/minecraftforge/forge/maven-metadata.xml"
                : "https://maven.neoforged.net/releases/net/neoforged/neoforge/maven-metadata.xml";
        QXmlStreamReader xml(net.request(url));
        auto parts = game.split('.');
        QString prefix = loader == "Forge"
                             ? game + '-'
                             : (parts.size() >= 2 ? parts[1] + '.' + (parts.size() > 2 ? parts[2] : "0") + '.'
                                                  : "unsupported");
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == u"version") {
                auto ver = xml.readElementText();
                if (ver.startsWith(prefix) && !ver.contains("beta"))
                    result << ver;
            }
        }
        std::reverse(result.begin(), result.end());
    }
    std::stable_sort(result.begin(), result.end(), [](const QString &a, const QString &b) {
        bool preA = a.contains("beta") || a.contains("alpha") || a.contains("rc"),
             preB = b.contains("beta") || b.contains("alpha") || b.contains("rc");
        if (preA != preB)
            return !preA;
        return QVersionNumber::fromString(a) > QVersionNumber::fromString(b);
    });
    return result;
}
static int javaMajor(const QString &exe) {
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, {"-version"});
    if (!p.waitForStarted(5000) || !p.waitForFinished(10000) || p.exitCode() != 0)
        return 0;
    auto m = QRegularExpression("(?:version\\s+\\\"|openjdk\\s+)([0-9]+)(?:\\.([0-9]+))?")
                 .match(QString::fromUtf8(p.readAll()));
    if (!m.hasMatch())
        return 0;
    int n = m.captured(1).toInt();
    return n == 1 ? m.captured(2).toInt() : n;
}
QString Minecraft::javaFor(int major, const QString &custom) {
    if (!custom.trimmed().isEmpty()) {
        int found = javaMajor(custom);
        if (found != major)
            throw Error(QString("This instance needs Java %1. The selected Java reports %2. Clear the custom "
                                "path to download the correct runtime.")
                            .arg(major)
                            .arg(found));
        return custom;
    }
    QString runtime = root + "/runtimes/java-" + QString::number(major);
    auto locate = [&]() -> QString {
        QDirIterator it(runtime, {"java.exe"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            auto p = it.next();
            if (javaMajor(p) == major)
                return p;
        }
        return {};
    };
    QString cached = locate();
    if (!cached.isEmpty())
        return cached;
    QString found = QStandardPaths::findExecutable("java");
    if (!found.isEmpty() && javaMajor(found) == major)
        return found;
    net.ctx.report(QString("Getting Java %1 from Eclipse Adoptium…").arg(major));
    auto versions = net.json(QString("https://api.adoptium.net/v3/assets/latest/%1/"
                                     "hotspot?architecture=x64&image_type=jre&os=windows&vendor=eclipse")
                                 .arg(major))
                        .array();
    if (versions.isEmpty())
        throw Error("No automatic runtime found. Select an appropriate Java installation in Settings.");
    auto pkg = versions[0].toObject()["binary"].toObject()["package"].toObject();
    QString zip = root + "/cache/java-" + QString::number(major) + ".zip";
    net.downloads({{pkg["link"].toString(), zip, pkg["checksum"].toString(), "sha256",
                    qint64(pkg["size"].toDouble(-1))}});
    const auto stage = runtime + ".staging-" + timestamp();
    net.ctx.report(QString("Extracting Java %1…").arg(major));
    try {
        extractZip(zip, stage);
        net.ctx.check();
        replaceTree(stage, runtime);
    } catch (...) {
        QDir(stage).removeRecursively();
        throw;
    }
    cached = locate();
    if (cached.isEmpty())
        throw Error("Downloaded Java did not pass its version check.");
    return cached;
}
void Minecraft::libraries(const QJsonObject &profile, const QString &natives, QStringList &classpath) {
    QList<Download> downloads;
    QStringList nativeZips;
    QSet<QString> seen;
    for (auto value : profile["libraries"].toArray()) {
        auto lib = value.toObject();
        if (!rulesAllow(lib["rules"].toArray()))
            continue;
        auto coords = lib["name"].toString().split(':');
        QString key = coords.value(0) + ':' + coords.value(1) + ':' + coords.value(3);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        auto dl = lib["downloads"].toObject();
        auto artifact = dl["artifact"].toObject();
        if (!artifact.isEmpty() || !lib.contains("natives")) {
            QString rel = artifact["path"].toString();
            if (rel.isEmpty())
                rel = mavenPath(lib["name"].toString());
            QString dest = safePath(root + "/cache/game/libraries", rel), url = artifact["url"].toString();
            if (url.isEmpty())
                url = lib["url"].toString("https://libraries.minecraft.net/") + rel;
            if (!QFileInfo::exists(dest) || !artifact["sha1"].toString().isEmpty())
                downloads << Download{url, dest, artifact["sha1"].toString(), "sha1",
                                      qint64(artifact["size"].toDouble(-1))};
            classpath << dest;
        }
        QString native = lib["natives"].toObject()["windows"].toString();
        native.replace("${arch}", "64");
        if (!native.isEmpty()) {
            auto n = dl["classifiers"].toObject()[native].toObject();
            if (n.isEmpty())
                continue;
            auto dest = safePath(root + "/cache/game/libraries", n["path"].toString());
            downloads << Download{n["url"].toString(), dest, n["sha1"].toString(), "sha1",
                                  qint64(n["size"].toDouble(-1))};
            nativeZips << dest;
        }
    }
    net.downloads(downloads);
    QDir().mkpath(natives);
    qsizetype extracted = 0;
    for (const auto &zip : nativeZips) {
        net.ctx.report("Preparing native libraries…", extracted, nativeZips.size());
        extractZip(zip, natives, {}, true);
        net.ctx.report("Preparing native libraries…", ++extracted, nativeZips.size());
    }
}
QJsonObject Minecraft::loaderProfile(const QJsonObject &instance, const QJsonObject &vanilla,
                                     const QString &java) {
    auto loader = instance["loader"].toString();
    if (loader == "Vanilla")
        return vanilla;
    auto game = instance["version"].toString(), version = instance["loaderVersion"].toString();
    if (version.isEmpty()) {
        auto all = loaders(game, loader);
        if (all.isEmpty())
            throw Error(loader + " has no supported loader for " + game);
        version = all.first();
    }
    QJsonObject custom;
    if (loader == "Fabric" || loader == "Quilt") {
        QString base = loader == "Fabric" ? "https://meta.fabricmc.net/v2" : "https://meta.quiltmc.org/v3";
        custom =
            net.json(base + "/versions/loader/" + enc(game) + '/' + enc(version) + "/profile/json").object();
    } else if (loader == "Forge" || loader == "NeoForge") {
        QString full = version;
        if (loader == "Forge" && !full.startsWith(game + '-'))
            full = game + '-' + full;
        QString base = loader == "Forge" ? "https://maven.minecraftforge.net/net/minecraftforge/forge/"
                                         : "https://maven.neoforged.net/releases/net/neoforged/neoforge/";
        QString file = (loader == "Forge" ? "forge-" : "neoforge-") + full + "-installer.jar";
        QString dest = safePath(root + "/cache/installers", file);
        QString url = base + full + '/' + file;
        QString sha = QString::fromUtf8(net.request(url + ".sha1")).trimmed().split(' ').first();
        if (!QRegularExpression("^[a-fA-F0-9]{40}$").match(sha).hasMatch())
            throw Error("Loader installer checksum is unavailable.");
        net.downloads({{url, dest, sha}});
        auto embedded = QJsonDocument::fromJson(zipEntry(dest, "version.json")).object();
        if (embedded.isEmpty())
            throw Error("This older loader installer is unsupported. Use a modern version.");
        QString gameRoot = root + "/cache/game";
        if (!QFileInfo::exists(gameRoot + "/launcher_profiles.json"))
            writeObject(gameRoot + "/launcher_profiles.json",
                        {{"profiles", QJsonObject{}}, {"settings", QJsonObject{}}});
        net.ctx.report("Running the official " + loader + " installer…");
        QProcess p;
        p.setWorkingDirectory(gameRoot);
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start(java, {"-jar", dest, "--installClient", gameRoot});
        if (!p.waitForStarted(10000))
            throw Error("Could not start the loader installer.");
        while (!p.waitForFinished(200)) {
            if (net.ctx.cancel && net.ctx.cancel->load()) {
                p.kill();
                p.waitForFinished();
                net.ctx.check();
            }
            auto b = p.readAll();
            if (!b.isEmpty() && net.ctx.progress)
                net.ctx.progress(redact(QString::fromUtf8(b).right(1500)), 0, 0);
        }
        auto output = p.readAll();
        if (p.exitCode() != 0)
            throw Error(loader + " installation failed: " + redact(QString::fromUtf8(output).right(2000)));
        custom = readObject(safePath(gameRoot + "/versions",
                                     embedded["id"].toString() + '/' + embedded["id"].toString() + ".json"));
    } else
        throw Error("Unknown mod loader: " + loader);
    auto merged = vanilla;
    for (auto i = custom.begin(); i != custom.end(); ++i)
        if (i.key() != "libraries" && i.key() != "arguments")
            merged[i.key()] = i.value();
    QJsonArray libs = custom["libraries"].toArray();
    for (auto l : vanilla["libraries"].toArray())
        libs.append(l);
    merged["libraries"] = libs;
    auto args = vanilla["arguments"].toObject();
    auto extra = custom["arguments"].toObject();
    for (const auto &key : {"game", "jvm"}) {
        auto a = args[key].toArray();
        for (auto v : extra[key].toArray())
            a.append(v);
        args[key] = a;
    }
    merged["arguments"] = args;
    merged["molyLoaderVersion"] = version;
    return merged;
}
QJsonObject Minecraft::install(const QJsonObject &instance, const QJsonObject &settings) {
    const QString id = instance["id"].toString(), game = instance["version"].toString();
    QString dir = safePath(root + "/instances", id);
    QDir().mkpath(dir + "/game");
    // Each phase reports measured work in its own units. Metadata, extraction and official
    // installer execution have no reliable total, so they deliberately remain unspecified.
    net.ctx.report("Reading Minecraft " + game + " metadata…");
    auto vanilla = gameVersion(game);
    if (!vanilla.contains("arguments"))
        throw Error(
            "This preview supports Minecraft 1.13 and newer. Older argument formats are not enabled.");
    int major = vanilla["javaVersion"].toObject()["majorVersion"].toInt(8);
    auto overrides = instance["settings"].toObject();
    auto custom = overrides["javaPath"].toString(settings["javaPath"].toString());
    QString java = javaFor(major, custom),
            client = safePath(root + "/cache/game/versions", game + '/' + game + ".jar");
    auto c = vanilla["downloads"].toObject()["client"].toObject();
    auto ai = vanilla["assetIndex"].toObject();
    QString index = safePath(root + "/cache/game/assets/indexes", ai["id"].toString() + ".json");
    net.ctx.report("Downloading Minecraft " + game + "…");
    net.downloads(
        {{c["url"].toString(), client, c["sha1"].toString(), "sha1", qint64(c["size"].toDouble(-1))},
         {ai["url"].toString(), index, ai["sha1"].toString(), "sha1", qint64(ai["size"].toDouble(-1))}});
    net.ctx.report("Installing core libraries…");
    QStringList ignored;
    libraries(vanilla, dir + "/natives", ignored);
    auto profile = loaderProfile(instance, vanilla, java);
    net.ctx.report("Installing loader libraries…");
    QStringList classpath;
    libraries(profile, dir + "/natives", classpath);
    classpath << client;
    auto objects = readObject(index)["objects"].toObject();
    QList<Download> assets;
    for (auto i = objects.begin(); i != objects.end(); ++i) {
        auto o = i.value().toObject();
        auto h = o["hash"].toString();
        if (!QRegularExpression("^[a-f0-9]{40}$").match(h).hasMatch())
            throw Error("Invalid asset hash.");
        auto rel = h.left(2) + '/' + h;
        assets << Download{"https://resources.download.minecraft.net/" + rel,
                           root + "/cache/game/assets/objects/" + rel, h, "sha1",
                           qint64(o["size"].toDouble(-1))};
    }
    net.ctx.report("Installing game assets…");
    net.downloads(assets);
    net.ctx.report("Finishing up…");
    QString logging;
    auto log = profile["logging"].toObject()["client"].toObject();
    if (!log.isEmpty()) {
        auto f = log["file"].toObject();
        auto path = safePath(root + "/cache/game/assets/log_configs", f["id"].toString());
        net.downloads({{f["url"].toString(), path, f["sha1"].toString(), "sha1", qint64(f["size"].toDouble(-1))}});
        logging = log["argument"].toString();
        logging.replace("${path}", QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded));
    }
    net.ctx.check();
    QJsonObject state{{"schemaVersion", 1},
                      {"java", java},
                      {"javaMajor", major},
                      {"profile", profile},
                      {"classpath", QJsonArray::fromStringList(classpath)},
                      {"natives", dir + "/natives"},
                      {"assets", root + "/cache/game/assets"},
                      {"libraryDirectory", root + "/cache/game/libraries"},
                      {"gameDirectory", dir + "/game"},
                      {"logging", logging},
                      {"instanceVersion", game},
                      {"loader", instance["loader"]},
                      {"loaderVersion", profile["molyLoaderVersion"]}};
    writeObject(dir + "/installation.json", state);
    net.ctx.report("Installation complete", 1, 1);
    return state;
}
LaunchCommand Minecraft::command(const QJsonObject &state, const QJsonObject &instance,
                                 const QJsonObject &global, const QJsonObject &account,
                                 const QString &token) {
    auto settings = global;
    auto overrides = instance["settings"].toObject();
    for (auto i = overrides.begin(); i != overrides.end(); ++i)
        if (!i.value().isNull())
            settings[i.key()] = i.value();
    auto profile = state["profile"].toObject();
    QStringList cp;
    for (auto p : state["classpath"].toArray()) {
        auto path = p.toString();
        if (!QFileInfo::exists(path))
            throw Error("An installed library is missing. Use Repair / Install before playing.");
        cp << QDir::toNativeSeparators(path);
    }
    if (cp.isEmpty() || profile["mainClass"].toString().isEmpty())
        throw Error("This instance has not finished installing.");
    int ram = settings["memoryMb"].toInt(4096);
    if (ram < 512 || ram > physicalMemoryMb() - 512)
        throw Error("Memory allocation must leave at least 512 MB for Windows.");
    QMap<QString, QString> vars{{"auth_player_name", account["name"].toString()},
                                {"auth_uuid", account["uuid"].toString()},
                                {"auth_access_token", token.isEmpty() ? "0" : token},
                                {"user_type", token.isEmpty() ? "legacy" : "msa"},
                                {"version_name", profile["id"].toString()},
                                {"version_type", "release"},
                                {"game_directory", state["gameDirectory"].toString()},
                                {"assets_root", state["assets"].toString()},
                                {"assets_index_name", profile["assetIndex"].toObject()["id"].toString()},
                                {"natives_directory", state["natives"].toString()},
                                {"launcher_name", "Moly"},
                                {"launcher_version", "0.3.1"},
                                {"classpath", cp.join(';')},
                                {"classpath_separator", ";"},
                                {"library_directory", state["libraryDirectory"].toString()},
                                {"resolution_width", QString::number(settings["width"].toInt(1280))},
                                {"resolution_height", QString::number(settings["height"].toInt(720))},
                                {"clientid", global["microsoftClientId"].toString()},
                                {"auth_xuid", account["xuid"].toString()},
                                {"user_properties", "{}"}};
    QJsonObject features{
        {"has_custom_resolution", true}, {"is_demo_user", false}, {"has_quick_plays_support", false}};
    const auto quick=instance["_quickPlay"].toObject();
    if(!quick.isEmpty()) {
        const auto type=quick["type"].toString();
        const auto option=type=="world"?QString("--quickPlaySingleplayer"):QString("--quickPlayMultiplayer");
        const auto gameJson=QJsonDocument(profile["arguments"].toObject()["game"].toArray()).toJson();
        if(!gameJson.contains(option.toUtf8()))throw Error("This Minecraft version does not support direct world/server launch. Open the instance and use Play to choose it inside Minecraft.");
        features["has_quick_plays_support"]=true;
        features["is_quick_play_singleplayer"]=type=="world";
        features["is_quick_play_multiplayer"]=type=="server";
        vars["quickPlaySingleplayer"]=type=="world"?quick["value"].toString():QString();
        vars["quickPlayMultiplayer"]=type=="server"?quick["value"].toString():QString();
        vars["quickPlayPath"]="moly-quickplay.json";
    }
    QStringList args = argumentValues(profile["arguments"].toObject()["jvm"].toArray(), features);
    if (args.isEmpty())
        args << "-Djava.library.path=${natives_directory}" << "-cp" << "${classpath}";
    args << QString("-Xms%1M").arg(qMin(512, ram)) << QString("-Xmx%1M").arg(ram);
    if (!state["logging"].toString().isEmpty()) {
        auto logging = state["logging"].toString();
        const QString prefix = "-Dlog4j.configurationFile=";
        if (logging.startsWith(prefix) &&
            QRegularExpression("^[A-Za-z]:/").match(logging.mid(prefix.size())).hasMatch())
            logging = prefix + QUrl::fromLocalFile(logging.mid(prefix.size())).toString(QUrl::FullyEncoded);
        args << logging;
    }
    auto custom = QProcess::splitCommand(settings["jvmArgs"].toString());
    for (const auto &a : custom)
        if (a.startsWith("-Xmx") || a.startsWith("-Xms") || a == "-cp" || a == "-classpath" || a == "-jar")
            throw Error("Use the memory controls for RAM. Classpath and -jar arguments are managed by Moly.");
    args << custom;
    args << profile["mainClass"].toString();
    args << argumentValues(profile["arguments"].toObject()["game"].toArray(), features);
    if (settings["fullscreen"].toBool())
        args << "--fullscreen";
    for (auto &a : args) {
        // Forge's official launcher renames the vanilla JAR to its profile ID. Our shared cache
        // keeps the vanilla filename, so its module exclusion must include that filename too.
        if (a.startsWith("-DignoreList="))
            a += ',' + state["instanceVersion"].toString() + ".jar";
        for (auto i = vars.begin(); i != vars.end(); ++i)
            a.replace("${" + i.key() + '}', i.value());
        if (a.contains("${"))
            throw Error("Unsupported launch argument: " + a);
    }
    auto java = settings["javaPath"].toString();
    if (java.isEmpty())
        java = state["java"].toString();
    if (!QFileInfo::exists(java))
        throw Error("Java is missing. Use Repair / Install to restore it.");
    if (java != state["java"].toString() && javaMajor(java) != state["javaMajor"].toInt())
        throw Error("The selected Java override does not match this instance's required Java version.");
    return {java, state["gameDirectory"].toString(), args};
}
} // namespace moly
