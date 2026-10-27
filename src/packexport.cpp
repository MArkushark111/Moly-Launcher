#include "packexport.h"
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTemporaryDir>
#include <QUrl>
#include <QUuid>
#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

namespace moly {
namespace {
constexpr qint64 MaxFile = 512ll * 1024 * 1024;
constexpr qint64 MaxPack = 8ll * 1024 * 1024 * 1024;
const QStringList Artwork{"icon.png", "icon.gif", "background.png", "background.gif"};

bool isSensitive(const QString &relative) {
    const auto name = QFileInfo(relative).fileName().toLower();
    static const QRegularExpression privateName(
        "(?:^|[._-])(?:accounts?|sessions?|tokens?|credentials?|authlib|usercache|usernamecache)(?:[._-]|$)");
    return privateName.match(name).hasMatch() || name.startsWith("launcher_") ||
           name == "session.lock" || name.endsWith(".log") || name.endsWith(".lck");
}

bool gameFileAllowed(const QString &relative, bool includeWorlds) {
    safePath(QDir::tempPath() + "/moly-export-check", relative);
    const auto first = relative.section('/', 0, 0).toLower();
    if (QStringList{"assets", "libraries", "versions", "runtime", "runtimes", "java", "natives",
                    "logs", "crash-reports", "screenshots", "backups", ".fabric", ".quilt", ".cache"}
            .contains(first))
        return false;
    if (first == "saves" && !includeWorlds)
        return false;
    if (first.startsWith('.') || isSensitive(relative))
        return false;
    const auto name = QFileInfo(relative).fileName().toLower();
    return name != "minecraft.jar" && name != "client.jar" && name != "server.jar" &&
           name != "installation.json" && name != "launcher_profiles.json";
}

QJsonObject portableSettings(const QJsonObject &source) {
    QJsonObject result;
    for (const auto &key : {"memoryMb", "width", "height"}) {
        if (!source.contains(key))
            continue;
        const int value = source[key].toInt(-1);
        const int minimum = QString(key) == "memoryMb" ? 512 : QString(key) == "width" ? 640 : 360;
        const int maximum = QString(key) == "memoryMb" ? 1048576 : 16384;
        if (value >= minimum && value <= maximum)
            result[key] = value;
    }
    for (const auto &key : {"fullscreen", "reducedMotion"})
        if (source[key].isBool())
            result[key] = source[key];
    for (const auto &key : {"imageDim", "backgroundDim", "backgroundOpacity"})
        if (source[key].isDouble() && source[key].toDouble() >= 0 && source[key].toDouble() <= 1)
            result[key] = source[key];
    return result;
}

QJsonObject portableInstance(const QJsonObject &source) {
    const auto version = source["version"].toString();
    const auto loader = source["loader"].toString("Vanilla");
    static const QRegularExpression versionPattern("^[a-zA-Z0-9][a-zA-Z0-9._+\\-]{0,95}$");
    if (!versionPattern.match(version).hasMatch())
        throw Error("The instance has an unsupported Minecraft version identifier.");
    if (!QStringList{"Vanilla", "Fabric", "Forge", "NeoForge", "Quilt"}.contains(loader))
        throw Error("The pack requests an unsupported mod loader.");
    const auto loaderVersion = source["loaderVersion"].toString();
    if (loader != "Vanilla" && !versionPattern.match(loaderVersion).hasMatch())
        throw Error("The instance needs a valid mod-loader version before it can be exported.");
    const auto name = source["name"].toString().trimmed().left(80);
    if (name.isEmpty())
        throw Error("The pack needs an instance name.");
    QJsonObject result{{"schemaVersion", 1}, {"name", name}, {"version", version}, {"loader", loader},
                       {"group", source["group"].toString().left(64)},
                       {"settings", portableSettings(source["settings"].toObject())}};
    if (!loaderVersion.isEmpty())
        result["loaderVersion"] = loaderVersion;
    for (const auto &field : {"iconFile", "backgroundFile"}) {
        const auto value = source[field].toString();
        if (Artwork.contains(value) && value.startsWith(QString(field) == "iconFile" ? "icon." : "background."))
            result[field] = value;
    }
    return result;
}

bool publicUrl(const QString &text, bool modrinthOnly = false) {
    const QUrl url(text, QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != "https" || !url.userInfo().isEmpty() || url.hasQuery() ||
        url.hasFragment() || url.host().isEmpty())
        return false;
    return !modrinthOnly || QStringList{"cdn.modrinth.com", "github.com", "raw.githubusercontent.com", "gitlab.com"}.contains(url.host());
}

QJsonObject safeContent(const QJsonObject &source) {
    QJsonArray files;
    for (const auto &entry : source["files"].toArray()) {
        const auto original = entry.toObject();
        const auto path = original["path"].toString();
        if (!gameFileAllowed(path, true))
            continue;
        QJsonObject clean;
        for (const auto &key : {"path", "provider", "projectId", "versionId", "type", "title", "author",
                                "versionName", "filename", "algorithm", "hash", "size"})
            if (original.contains(key))
                clean[key] = original[key];
        for (const auto &key : {"url", "page", "icon"})
            if (publicUrl(original[key].toString()))
                clean[key] = original[key];
        files.append(clean);
    }
    return {{"schemaVersion", 1}, {"files", files}};
}

struct Payload { QString path, from; qint64 size; };

// QZipWriter closes even a borrowed QIODevice when finishing. QSaveFile must be
// closed only by commit()/cancelWriting(), so keep its transaction behind a
// non-owning seekable adapter rather than passing QSaveFile directly.
class ZipDevice final : public QIODevice {
  public:
    explicit ZipDevice(QSaveFile &destination) : file(destination) { open(QIODevice::WriteOnly); }
    bool isSequential() const override { return false; }
    qint64 size() const override { return file.size(); }
    bool seek(qint64 offset) override { return file.seek(offset) && QIODevice::seek(offset); }
    void close() override { /* QSaveFile owns the commit boundary. */ }
  protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *bytes, qint64 size) override { return file.write(bytes, size); }
  private:
    QSaveFile &file;
};

QList<Payload> collectFiles(const QString &directory, const QJsonObject &instance, bool worlds) {
    QList<Payload> files;
    qint64 total = 0;
    const auto game = directory + "/game";
    QDirIterator it(game, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        if (info.isSymLink())
            throw Error("Linked files cannot be exported: " + info.fileName());
        if (!info.isFile())
            continue;
        const auto relative = QDir(game).relativeFilePath(info.absoluteFilePath());
        if (!gameFileAllowed(relative, worlds))
            continue;
        safePath(game, relative);
        if (info.size() > MaxFile || (total += info.size()) > MaxPack || files.size() >= 99990)
            throw Error("This setup exceeds the portable pack limits (512 MiB per file, 8 GiB total).");
        files.append({"game/" + relative, info.absoluteFilePath(), info.size()});
    }
    for (const auto &field : {"iconFile", "backgroundFile"}) {
        const auto filename = instance[field].toString();
        if (!Artwork.contains(filename))
            continue;
        const auto path = safePath(directory, filename);
        const QFileInfo info(path);
        if (info.isFile()) {
            if (info.size() > 32 * 1024 * 1024)
                throw Error("Instance artwork is too large to export.");
            files.append({filename, path, info.size()});
        }
    }
    return files;
}

QByteArray jsonBytes(const QJsonObject &object) { return QJsonDocument(object).toJson(QJsonDocument::Indented); }

void addFile(QZipWriter &zip, const QString &name, const QString &from) {
    QFile input(from);
    if (!input.open(QIODevice::ReadOnly))
        throw Error("Cannot read export file: " + name);
    zip.addFile(name, &input);
    if (zip.status() != QZipWriter::NoError)
        throw Error("Cannot write export file: " + name);
}

void checkRestricted(const QMap<QString, QJsonObject> &managed) {
    for (auto it = managed.begin(); it != managed.end(); ++it)
        if (it->value("provider").toString().contains("CurseForge", Qt::CaseInsensitive) &&
            !publicUrl(it->value("url").toString()))
            throw Error("A restricted CurseForge file cannot be embedded in an exported pack: " + it.key() +
                        ". Its author requires a separate browser download.");
}

QMap<QString, QJsonObject> managedFiles(const QJsonObject &content) {
    QMap<QString, QJsonObject> result;
    for (const auto &entry : content["files"].toArray()) {
        auto file = entry.toObject();
        result[file["path"].toString().toLower()] = file;
    }
    return result;
}
} // namespace

void PackExport::write(const QString &root, const QJsonObject &instance, const QString &destination,
                       const QString &requestedFormat, bool includeWorlds, Context context) {
    context.check();
    const auto format = requestedFormat.toLower().remove('.');
    if (!QStringList{"mlypack", "zip", "mrpack"}.contains(format))
        throw Error("Choose a .mrpack, .zip, or .mlypack export.");
    const auto source = safePath(root + "/instances", instance["id"].toString());
    if (!QDir(source).exists())
        throw Error("The instance folder no longer exists.");
    const auto target = QFileInfo(destination).absoluteFilePath();
    const auto rootPath = QDir(root).absolutePath();
    if (target.startsWith(rootPath + "/instances/", Qt::CaseInsensitive) ||
        target.compare(rootPath + "/settings.json",Qt::CaseInsensitive)==0 ||
        target.compare(rootPath + "/accounts.json",Qt::CaseInsensitive)==0)
        throw Error("Save the export outside the instance folders and launcher settings.");
    auto descriptor = portableInstance(instance);
    const auto content = safeContent(readObject(source + "/content.json", true));
    auto managed = managedFiles(content);
    auto payload = collectFiles(source, descriptor, format == "mrpack" ? false : includeWorlds);
    QMap<QString, QJsonObject> presentManaged;
    for (const auto &file : payload) {
        if (!file.path.startsWith("game/")) continue;
        auto path=file.path.mid(5).toLower();
        if(path.endsWith(".disabled"))path.chop(9);
        if(managed.contains(path))presentManaged.insert(path,managed[path]);
    }
    checkRestricted(presentManaged);
    QJsonArray indexFiles, integrity;
    QMap<QString, QString> archiveFiles;
    QMap<QString, QString> archiveHashes;
    for (qsizetype i = 0; i < payload.size(); ++i) {
        context.report("Preparing export files", i, payload.size());
        const auto &file = payload[i];
        const auto sha512 = fileHash(file.from, "sha512");
        if (sha512.isEmpty())
            throw Error("Cannot hash export file: " + file.path);
        if (format != "mrpack") {
            archiveFiles[file.path] = file.from;
            archiveHashes[file.path] = sha512;
            integrity.append(QJsonObject{{"path", file.path}, {"size", double(file.size)}, {"sha512", sha512}});
            continue;
        }
        if (!file.path.startsWith("game/"))
            continue;
        const auto relative = file.path.mid(5);
        auto key = relative.toLower();
        if (key.endsWith(".disabled"))
            key.chop(9);
        const auto tracked = managed.value(key);
        const auto provider = tracked["provider"].toString();
        const bool local = tracked.isEmpty() || provider.contains("Local", Qt::CaseInsensitive) ||
                           provider == "Pack override";
        if (!local) {
            if (!publicUrl(tracked["url"].toString(), true))
                throw Error("This managed file has no portable Modrinth-compatible download: " + relative +
                            ". Use .mlypack or .zip instead.");
            const auto originalHash = tracked["hash"].toString();
            if (originalHash.isEmpty() || fileHash(file.from, tracked["algorithm"].toString("sha1")) != originalHash)
                throw Error("This managed file has local changes: " + relative +
                            ". Use .mlypack or .zip to preserve the changed file.");
            indexFiles.append(QJsonObject{{"path", relative},
                {"hashes", QJsonObject{{"sha1", fileHash(file.from, "sha1")}, {"sha512", sha512}}},
                {"downloads", QJsonArray{tracked["url"]}}, {"fileSize", double(file.size)}});
        } else {
            archiveFiles["overrides/" + relative] = file.from;
            archiveHashes["overrides/" + relative] = sha512;
        }
    }
    if (!QDir().mkpath(QFileInfo(target).absolutePath()))
        throw Error("Cannot create the export folder.");
    QSaveFile output(target);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly))
        throw Error("Cannot open the export file. Choose a writable location.");
    ZipDevice zipDevice(output);
    {
        QZipWriter zip(&zipDevice);
        zip.setCompressionPolicy(QZipWriter::AutoCompress);
        if (format == "mrpack") {
            QJsonObject dependencies{{"minecraft", descriptor["version"]}};
            const QMap<QString, QString> loaderKeys{{"Fabric", "fabric-loader"}, {"Quilt", "quilt-loader"}, {"Forge", "forge"}, {"NeoForge", "neoforge"}};
            const auto loader = descriptor["loader"].toString();
            if (loaderKeys.contains(loader))
                dependencies[loaderKeys[loader]] = descriptor["loaderVersion"];
            zip.addFile("modrinth.index.json", jsonBytes({{"formatVersion", 1}, {"game", "minecraft"},
                {"versionId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"name", descriptor["name"]},
                {"files", indexFiles}, {"dependencies", dependencies}}));
        } else {
            zip.addFile("instance.json", jsonBytes(descriptor));
            zip.addFile("content.json", jsonBytes(content));
            zip.addFile("moly-pack.json", jsonBytes({{"format", "moly-instance-pack"}, {"schemaVersion", 1},
                {"createdAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                {"includesWorlds", includeWorlds}, {"instance", descriptor}, {"content", content}, {"files", integrity}}));
        }
        qsizetype index = 0;
        for (auto it = archiveFiles.begin(); it != archiveFiles.end(); ++it) {
            context.report("Writing " + it.key(), index++, archiveFiles.size());
            addFile(zip, it.key(), it.value());
            if(fileHash(it.value(),"sha512")!=archiveHashes[it.key()])
                throw Error("A file changed while exporting: "+it.key()+". Close Minecraft and retry.");
        }
        context.check();
        zip.close();
        if (zip.status() != QZipWriter::NoError)
            throw Error("The export could not be completed. Check available disk space.");
    }
    context.check();
    if (!output.commit())
        throw Error("The export could not be saved. The previous file has been preserved.");
    // Cancellation is deferred after the atomic commit. Reporting completion
    // must not turn a successfully committed export into a failed operation.
    if(context.progress)context.progress("Export complete",1,1);
}

QJsonObject PackExport::importMoly(const QString &root, const QString &archive, Context context) {
    context.check();
    const auto cache=safePath(root,"cache");
    if (!QDir().mkpath(cache))
        throw Error("Cannot create an import staging folder.");
    QTemporaryDir unpacked(cache + "/moly-import-XXXXXX");
    if (!unpacked.isValid())
        throw Error("Cannot create an import staging folder.");
    context.report("Reading portable pack");
    extractZip(archive, unpacked.path());
    context.check();
    const auto manifest = readObject(unpacked.path() + "/moly-pack.json");
    if (manifest["format"] != "moly-instance-pack" || manifest["schemaVersion"].toInt() != 1)
        throw Error("This portable pack version is not supported by Moly.");
    auto instance = portableInstance(manifest["instance"].toObject());
    const auto files = manifest["files"].toArray();
    const bool includesWorlds = manifest["includesWorlds"].toBool();
    QSet<QString> listed;
    qint64 total = 0;
    static const QRegularExpression shaPattern("^[a-fA-F0-9]{128}$");
    for (qsizetype i = 0; i < files.size(); ++i) {
        context.report("Checking portable pack files", i, files.size());
        const auto file = files[i].toObject();
        const auto relative = file["path"].toString();
        const auto path = safePath(unpacked.path(), relative);
        if ((!Artwork.contains(relative) && (!relative.startsWith("game/") || !gameFileAllowed(relative.mid(5), includesWorlds))) ||
            listed.contains(relative.toLower()))
            throw Error("The portable pack contains an unsupported or duplicate file: " + relative);
        listed.insert(relative.toLower());
        const QFileInfo info(path);
        const qint64 expected = qint64(file["size"].toDouble(-1));
        if (!info.isFile() || info.size() != expected || expected < 0 || expected > MaxFile ||
            (total += expected) > MaxPack || !shaPattern.match(file["sha512"].toString()).hasMatch() ||
            fileHash(path, "sha512").compare(file["sha512"].toString(), Qt::CaseInsensitive) != 0)
            throw Error("A portable pack file failed verification: " + relative);
    }
    QDirIterator found(unpacked.path(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (found.hasNext()) {
        const auto rel = QDir(unpacked.path()).relativeFilePath(found.next());
        if (!listed.contains(rel.toLower()) && !QStringList{"moly-pack.json", "instance.json", "content.json"}.contains(rel))
            throw Error("The portable pack contains an unlisted file: " + rel);
    }
    const auto instances = safePath(root,"instances");
    if (!QDir().mkpath(instances))
        throw Error("Cannot create the instances folder.");
    QTemporaryDir staged(instances + "/.import-XXXXXX");
    if (!staged.isValid())
        throw Error("Cannot stage the imported instance.");
    instance["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    instance["group"] = "Imported";
    instance["iconRevision"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    instance["backgroundRevision"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    for (const auto &field : {"iconFile", "backgroundFile"})
        if (!listed.contains(instance[field].toString().toLower()))
            instance.remove(field);
    for (qsizetype i = 0; i < files.size(); ++i) {
        context.report("Importing portable pack", i, files.size());
        const auto relative = files[i].toObject()["path"].toString();
        const auto target = safePath(staged.path(), relative);
        QDir().mkpath(QFileInfo(target).absolutePath());
        if (!QFile::copy(safePath(unpacked.path(), relative), target))
            throw Error("Cannot copy imported file: " + relative);
    }
    QDir().mkpath(staged.path() + "/game");
    writeObject(staged.path() + "/instance.json", instance);
    writeObject(staged.path() + "/content.json", safeContent(manifest["content"].toObject()));
    context.check();
    const auto destination = safePath(instances, instance["id"].toString());
    if (!QDir().rename(staged.path(), destination))
        throw Error("Cannot commit the imported instance. No partial instance was added.");
    staged.setAutoRemove(false);
    // The instance is complete once renamed; a late cancellation cannot undo it.
    if(context.progress)context.progress("Portable instance imported",1,1);
    return instance;
}
} // namespace moly
