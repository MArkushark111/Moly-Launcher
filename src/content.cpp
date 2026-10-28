#include "content.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QUrlQuery>
#include <QUuid>
#include <private/qzipreader_p.h>

namespace moly {
static QString encoded(const QString &s) {
    return QString::fromLatin1(QUrl::toPercentEncoding(s));
}
static QString jsonArray(const QString &v) {
    return QString::fromUtf8(QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact));
}
QString contentFolder(const QString &type) {
    if (type == "datapack")
        throw Error("Choose a specific world folder for data packs.");
    if (type == "shader")
        return "shaderpacks";
    if (type == "resourcepack")
        return "resourcepacks";
    return "mods";
}
QString contentPath(const QString &base, const QString &relative) {
    auto path = safePath(base, relative);
    auto first = relative.split('/').first().toLower();
    if (QStringList{"saves", "logs", "screenshots", "crash-reports", "session.lock", "installation.json",
                    "instance.json"}
            .contains(first))
        throw Error("Packs cannot overwrite worlds, logs, or launcher metadata: " + relative);
    return path;
}
QJsonArray Content::search(const QString &provider, const QString &type, const QString &query,
                           const QJsonObject &instance, const QString &category) {
    return searchPage(provider, type, query, instance, category, 0, 30)["items"].toArray();
}
QJsonObject Content::searchPage(const QString &provider, const QString &type, const QString &query,
                                const QJsonObject &instance, const QString &category, int offset, int limit) {
    if (!QStringList{"mod", "modpack", "resourcepack", "shader", "datapack"}.contains(type))
        throw Error("Choose a supported content type.");
    offset = qMax(0, offset);
    limit = qBound(1, limit, provider == "CurseForge" ? 50 : 100);
    if (provider == "CurseForge") {
        if (offset >= 10000)
            return {{"items", QJsonArray{}}, {"total", 10000}, {"hasMore", false}};
        limit = qMin(limit, 10000 - offset);
    }
    QJsonArray result;
    int total = 0;
    const auto selectedCategory = category.trimmed().toLower();
    if (provider == "Modrinth") {
        QJsonArray facets{QJsonArray{(type == "datapack" ? "all_project_types:" : "project_type:") + type}};
        if (!instance.isEmpty() && type != "modpack") {
            facets.append(QJsonArray{"versions:" + instance["version"].toString()});
            if (type == "mod" && instance["loader"] != "Vanilla")
                facets.append(QJsonArray{"categories:" + instance["loader"].toString().toLower()});
        }
        const QMap<QString, QString> categories{{"performance", "optimization"},
                                                {"adventure", "adventure"},
                                                {"tech", "technology"},
                                                {"magic", "magic"},
                                                {"utility", "utility"}};
        if (categories.contains(selectedCategory))
            facets.append(QJsonArray{"categories:" + categories[selectedCategory]});
        const auto filter = QString::fromUtf8(QJsonDocument(facets).toJson(QJsonDocument::Compact));
        const auto sort = query.trimmed().isEmpty() ? "downloads" : "relevance";
        auto response = net.json("https://api.modrinth.com/v2/search?limit=" + QString::number(limit) +
                                 "&offset=" + QString::number(offset) + "&index=" + QString(sort) +
                                 "&query=" + encoded(query) + "&facets=" + encoded(filter))
                            .object();
        total = response["total_hits"].toInt();
        for (auto value : response["hits"].toArray()) {
            auto project = value.toObject();
            auto thumbnail = project["featured_gallery"].toString();
            if (thumbnail.isEmpty() && !project["gallery"].toArray().isEmpty())
                thumbnail = project["gallery"].toArray()[0].toString();
            result.append(
                QJsonObject{{"provider", provider},
                            {"projectId", project["project_id"]},
                            {"title", project["title"]},
                            {"description", project["description"]},
                            {"author", project["author"]},
                            {"icon", project["icon_url"]},
                            {"thumbnail", thumbnail},
                            {"downloads", project["downloads"]},
                            {"type", type},
                            {"page", "https://modrinth.com/" + type + '/' + project["slug"].toString()}});
        }
    } else if (provider == "CurseForge") {
        if (key.isEmpty())
            throw Error("Add your CurseForge API key in Settings → Connections. Modrinth is available "
                        "without a key.");
        int classId = type == "modpack" ? 4471 : type == "resourcepack" ? 12 : type == "shader" ? 6552 : 6;
        if (type == "datapack") {
            classId = 0;
            auto classes = net.json("https://api.curseforge.com/v1/categories?gameId=432&classesOnly=true",
                                    {}, {{"x-api-key", key.toUtf8()}})
                               .object()["data"]
                               .toArray();
            for (auto value : classes) {
                auto item = value.toObject();
                if (item["name"].toString().contains("data pack", Qt::CaseInsensitive) ||
                    item["slug"].toString() == "data-packs")
                    classId = item["id"].toInt();
            }
            if (!classId)
                throw Error("CurseForge does not currently expose a data-pack category. Choose Modrinth for "
                            "data packs.");
        }
        QString url =
            QString(
                "https://api.curseforge.com/v1/mods/"
                "search?gameId=432&classId=%1&pageSize=%2&index=%3&sortField=2&sortOrder=desc&searchFilter=")
                .arg(classId)
                .arg(limit)
                .arg(offset) +
            encoded(query);
        if (!instance.isEmpty() && type != "modpack") {
            url += "&gameVersion=" + encoded(instance["version"].toString());
            const int loader = instance["loader"] == "Forge"      ? 1
                               : instance["loader"] == "Fabric"   ? 4
                               : instance["loader"] == "Quilt"    ? 5
                               : instance["loader"] == "NeoForge" ? 6
                                                                  : 0;
            if (type == "mod" && loader)
                url += "&modLoaderType=" + QString::number(loader);
        }
        if (!selectedCategory.isEmpty() && selectedCategory != "popular") {
            auto categories =
                net.json(
                       QString("https://api.curseforge.com/v1/categories?gameId=432&classId=%1").arg(classId),
                       {}, {{"x-api-key", key.toUtf8()}})
                    .object()["data"]
                    .toArray();
            const QMap<QString, QStringList> aliases{{"performance", {"performance", "optimization"}},
                                                     {"adventure", {"adventure", "adventure-rpg"}},
                                                     {"tech", {"technology", "tech"}},
                                                     {"magic", {"magic"}},
                                                     {"utility", {"utility", "utility-qol"}}};
            for (auto value : categories) {
                auto item = value.toObject();
                if (aliases.value(selectedCategory).contains(item["slug"].toString())) {
                    url += "&categoryId=" + QString::number(item["id"].toInt());
                    break;
                }
            }
        }
        auto response = net.json(url, {}, {{"x-api-key", key.toUtf8()}}).object();
        total = qMin(10000, response["pagination"].toObject()["totalCount"].toInt());
        for (auto value : response["data"].toArray()) {
            auto project = value.toObject();
            QStringList authors;
            for (auto author : project["authors"].toArray())
                authors << author.toObject()["name"].toString();
            QString thumbnail;
            for (auto image : project["screenshots"].toArray()) {
                auto shot = image.toObject();
                thumbnail = shot["url"].toString();
                if (!thumbnail.isEmpty())
                    break;
            }
            result.append(QJsonObject{{"provider", provider},
                                      {"projectId", QString::number(qint64(project["id"].toDouble()))},
                                      {"title", project["name"]},
                                      {"description", project["summary"]},
                                      {"author", authors.join(", ")},
                                      {"icon", project["logo"].toObject()["thumbnailUrl"]},
                                      {"thumbnail", thumbnail},
                                      {"downloads", project["downloadCount"]},
                                      {"type", type},
                                      {"page", project["links"].toObject()["websiteUrl"]}});
        }
    } else
        throw Error("Choose Modrinth or CurseForge.");
    if (total < offset + result.size())
        total = offset + result.size();
    return {{"items", result},
            {"total", total},
            {"hasMore", offset + result.size() < total && result.size() >= limit}};
}
QJsonObject mergeSearchPage(const QJsonArray &previous, const QJsonObject &page, int offset, int pageSize) {
    QJsonArray merged;
    QSet<QString> seen;
    const auto appendUnique = [&](const QJsonArray &items) {
        for (const auto &value : items) {
            const auto item = value.toObject();
            const auto id = item["projectId"].toString();
            const auto provider = item["provider"].toString();
            if (id.isEmpty() || provider.isEmpty())
                continue;
            const auto key = provider + '|' + id;
            if (seen.contains(key))
                continue;
            seen.insert(key);
            merged.append(item);
        }
    };
    appendUnique(previous);
    const auto incoming = page["items"].toArray();
    appendUnique(incoming);
    const auto nextOffset = qMax(0, offset) + incoming.size();
    const auto total = qMax(qsizetype(page["total"].toInt()), nextOffset);
    return {{"items", merged},
            {"nextOffset", nextOffset},
            {"total", total},
            {"hasMore", incoming.size() >= qMax(1, pageSize) && page["hasMore"].toBool()}};
}
QJsonArray Content::versions(const QJsonObject &project, const QJsonObject &instance) {
    QJsonArray result;
    const auto provider = project["provider"].toString(), type = project["type"].toString();
    auto filter = type == "modpack" ? QJsonObject() : instance;
    if (provider == "Modrinth") {
        QString url =
            "https://api.modrinth.com/v2/project/" + encoded(project["projectId"].toString()) + "/version";
        if (!filter.isEmpty()) {
            url += "?game_versions=" + encoded(jsonArray(filter["version"].toString()));
            if (type == "mod" && filter["loader"] != "Vanilla")
                url += "&loaders=" + encoded(jsonArray(filter["loader"].toString().toLower()));
        }
        for (auto value : net.json(url).array()) {
            auto version = value.toObject();
            if (type == "mod" && !filter.isEmpty() &&
                !version["loaders"].toArray().contains(filter["loader"].toString().toLower()))
                continue;
            result.append(QJsonObject{{"id", version["id"]},
                                      {"name", version["version_number"]},
                                      {"label", version["version_number"].toString()+" · "+version["version_type"].toString()},
                                      {"channel", version["version_type"]},
                                      {"published", version["date_published"]}});
            if (result.size() >= 60)
                break;
        }
    } else if (provider == "CurseForge") {
        if (key.isEmpty())
            throw Error("Add your CurseForge API key in Settings → Connections.");
        QString url = "https://api.curseforge.com/v1/mods/" + encoded(project["projectId"].toString()) +
                      "/files?pageSize=50";
        if (!filter.isEmpty()) {
            url += "&gameVersion=" + encoded(filter["version"].toString());
            const int loader = filter["loader"] == "Forge"      ? 1
                               : filter["loader"] == "Fabric"   ? 4
                               : filter["loader"] == "Quilt"    ? 5
                               : filter["loader"] == "NeoForge" ? 6
                                                                : 0;
            if (type == "mod" && loader)
                url += "&modLoaderType=" + QString::number(loader);
        }
        for (auto value : net.json(url, {}, {{"x-api-key", key.toUtf8()}}).object()["data"].toArray()) {
            auto version = value.toObject();
            result.append(QJsonObject{{"id", QString::number(qint64(version["id"].toDouble()))},
                                      {"name", version["displayName"]},
                                      {"label", version["displayName"].toString()+" · "+QStringList{"release","beta","alpha"}.value(version["releaseType"].toInt()-1,"release")},
                                      {"channel", QStringList{"release","beta","alpha"}.value(version["releaseType"].toInt()-1,"release")},
                                      {"published", version["fileDate"]}});
        }
    }
    const auto currentId=project["versionId"].toString();bool includesCurrent=false;
    for(auto value:result)if(value.toObject()["id"]==currentId)includesCurrent=true;
    if(!currentId.isEmpty()&&!includesCurrent)try {
        if(provider=="Modrinth") {
            const auto version=net.json("https://api.modrinth.com/v2/version/"+encoded(currentId)).object();
            if(version["project_id"]==project["projectId"]&&(filter.isEmpty()||version["game_versions"].toArray().contains(filter["version"]))&&(type!="mod"||filter.isEmpty()||version["loaders"].toArray().contains(filter["loader"].toString().toLower())))
                result.append(QJsonObject{{"id",version["id"]},{"name",version["version_number"]},{"label",version["version_number"].toString()+" · "+version["version_type"].toString()},{"channel",version["version_type"]},{"published",version["date_published"]}});
        }else if(provider=="CurseForge"&&!key.isEmpty()) {
            const auto version=net.json("https://api.curseforge.com/v1/mods/"+encoded(project["projectId"].toString())+"/files/"+encoded(currentId),{},{{"x-api-key",key.toUtf8()}}).object()["data"].toObject();
            if(filter.isEmpty()||version["gameVersions"].toArray().contains(filter["version"])) {
                const auto channel=QStringList{"release","beta","alpha"}.value(version["releaseType"].toInt()-1,"release");
                result.append(QJsonObject{{"id",currentId},{"name",version["displayName"]},{"label",version["displayName"].toString()+" · "+channel},{"channel",channel},{"published",version["fileDate"]}});
            }
        }
    }catch(const std::exception &){net.ctx.check();} // Delisted current versions must not hide other choices.
    QList<QJsonObject> sorted;for(auto value:result)sorted.append(value.toObject());
    std::stable_sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a["published"].toString()>b["published"].toString();});
    result={};for(const auto &entry:sorted)result.append(entry);
    return result;
}
QJsonObject Content::modrinthVersion(const QString &project, const QJsonObject &instance,
                                     const QString &version) {
    QJsonArray versions;
    if (!version.isEmpty())
        versions.append(net.json("https://api.modrinth.com/v2/version/" + encoded(version)).object());
    else {
        QString url = "https://api.modrinth.com/v2/project/" + encoded(project) + "/version";
        if (!instance.isEmpty()) {
            url += "?game_versions=" + encoded(jsonArray(instance["version"].toString()));
            if (instance["loader"] != "Vanilla")
                url += "&loaders=" + encoded(jsonArray(instance["loader"].toString().toLower()));
        }
        versions = net.json(url).array();
    }
    for (auto v : versions) {
        auto o = v.toObject();
        if (!version.isEmpty() && !project.isEmpty() && o["project_id"].toString() != project) {
            const auto canonical =
                net.json("https://api.modrinth.com/v2/project/" + encoded(project)).object()["id"].toString();
            if (canonical.isEmpty() || o["project_id"].toString() != canonical)
                throw Error("The selected version belongs to a different project.");
        }
        if (!instance.isEmpty() && !o["game_versions"].toArray().contains(instance["version"]))
            continue;
        if (version.isEmpty() && o["version_type"] != "release")
            continue;
        auto candidates = o["files"].toArray();
        if (candidates.isEmpty())
            continue;
        QJsonObject f = candidates[0].toObject();
        for (auto c : candidates)
            if (c.toObject()["primary"].toBool())
                f = c.toObject();
        return {{"provider", "Modrinth"},
                {"projectId", o["project_id"]},
                {"versionId", o["id"]},
                {"title", o["name"]},
                {"versionName", o["version_number"]},
                {"url", f["url"]},
                {"filename", f["filename"]},
                {"hash", f["hashes"].toObject()["sha512"]},
                {"algorithm", "sha512"},
                {"size", f["size"]},
                {"dependencies", o["dependencies"]},
                {"gameVersions", o["game_versions"]},
                {"loaders", o["loaders"]}};
    }
    throw Error("No compatible release found for this project and instance.");
}
QJsonObject Content::curseFile(qint64 project, const QJsonObject &instance, qint64 file) {
    if (key.isEmpty())
        throw Error("A CurseForge API key is required. Add it in Settings → Connections.");
    QString url = QString("https://api.curseforge.com/v1/mods/%1/files").arg(project);
    if (file)
        url += '/' + QString::number(file);
    else if (!instance.isEmpty()) {
        url += "?gameVersion=" + encoded(instance["version"].toString());
        int l = instance["loader"] == "Forge"      ? 1
                : instance["loader"] == "Fabric"   ? 4
                : instance["loader"] == "Quilt"    ? 5
                : instance["loader"] == "NeoForge" ? 6
                                                   : 0;
        if (l)
            url += "&modLoaderType=" + QString::number(l);
    }
    auto response = net.json(url, {}, {{"x-api-key", key.toUtf8()}}).object();
    QJsonArray all = file ? QJsonArray{response["data"]} : response["data"].toArray();
    for (auto v : all) {
        auto o = v.toObject();
        if (!file && o["releaseType"].toInt() != 1)
            continue;
        if (!instance.isEmpty() && !o["gameVersions"].toArray().contains(instance["version"]))
            continue;
        QString hash, alg;
        for (auto h : o["hashes"].toArray()) {
            auto hh = h.toObject();
            if (hh["algo"].toInt() == 1) {
                hash = hh["value"].toString();
                alg = "sha1";
                break;
            }
        }
        if (hash.isEmpty())
            throw Error("CurseForge did not provide a verifiable SHA-1 for this file.");
        return {{"provider", "CurseForge"},
                {"projectId", QString::number(project)},
                {"versionId", QString::number(qint64(o["id"].toDouble()))},
                {"title", o["displayName"]},
                {"versionName", o["displayName"]},
                {"url", o["downloadUrl"]},
                {"filename", o["fileName"]},
                {"hash", hash},
                {"algorithm", alg},
                {"size", o["fileLength"]},
                {"dependencies", o["dependencies"]},
                {"page", QString("https://www.curseforge.com/minecraft/search?search=%1").arg(project)}};
    }
    throw Error("No compatible CurseForge file was found.");
}
QJsonObject Content::projectMetadata(const QString &provider, const QString &project,
                                     const QJsonObject &known) {
    const auto cacheKey = provider + '/' + project;
    QJsonObject metadata = metadataCache.value(cacheKey);
    for (const auto &field : {"title", "icon", "author"})
        if (!known[field].toString().isEmpty())
            metadata[field] = known[field];
    if (metadata["title"].toString().isEmpty() || metadata["icon"].toString().isEmpty() ||
        metadata["author"].toString().isEmpty()) {
        try {
            if (provider == "Modrinth") {
                const auto info =
                    net.json("https://api.modrinth.com/v2/project/" + encoded(project)).object();
                if (metadata["title"].toString().isEmpty())
                    metadata["title"] = info["title"];
                if (metadata["icon"].toString().isEmpty())
                    metadata["icon"] = info["icon_url"];
                if (metadata["author"].toString().isEmpty()) {
                    const auto members =
                        net.json("https://api.modrinth.com/v2/project/" + encoded(project) + "/members")
                            .array();
                    QStringList authors;
                    for (const auto &value : members) {
                        const auto member = value.toObject();
                        const auto name = member["user"].toObject()["username"].toString();
                        if (!name.isEmpty() &&
                            member["role"].toString().compare("Owner", Qt::CaseInsensitive) == 0)
                            authors << name;
                    }
                    if (authors.isEmpty() && !members.isEmpty())
                        authors << members[0].toObject()["user"].toObject()["username"].toString();
                    metadata["author"] = authors.join(", ");
                }
            } else if (provider == "CurseForge") {
                const auto info = net.json("https://api.curseforge.com/v1/mods/" + encoded(project), {},
                                           {{"x-api-key", key.toUtf8()}})
                                      .object()["data"]
                                      .toObject();
                if (metadata["title"].toString().isEmpty())
                    metadata["title"] = info["name"];
                if (metadata["icon"].toString().isEmpty())
                    metadata["icon"] = info["logo"].toObject()["thumbnailUrl"];
                if (metadata["author"].toString().isEmpty()) {
                    QStringList authors;
                    for (const auto &value : info["authors"].toArray())
                        authors << value.toObject()["name"].toString();
                    metadata["author"] = authors.join(", ");
                }
            }
        } catch (const std::exception &) {
            // Decorative project metadata must not prevent a verified mod file from installing.
            net.ctx.check();
        }
    }
    if (!metadata["icon"].toString().isEmpty()) {
        const QUrl icon(metadata["icon"].toString());
        if (icon.scheme() != "https" || icon.host().isEmpty())
            metadata.remove("icon");
    }
    metadataCache[cacheKey] = metadata;
    return metadata;
}
void Content::resolve(const QString &provider, const QString &project, const QString &type,
                      const QJsonObject &instance, QJsonArray &out, QSet<QString> &seen,
                      const QString &version, const QJsonObject &metadata) {
    net.ctx.check();
    QString keyId = provider + '/' + project;
    if (seen.contains(keyId)) {
        if (!version.isEmpty())
            for (auto v : out) {
                auto f = v.toObject();
                if (f["provider"] == provider && f["projectId"] == project && f["versionId"] != version)
                    throw Error("Dependencies require conflicting versions of the same project.");
            }
        return;
    }
    if (seen.size() > 300)
        throw Error("Dependency tree exceeds 300 projects.");
    seen.insert(keyId);
    auto filter = instance;
    if (type != "mod")
        filter["loader"] = "Vanilla";
    auto f = provider == "Modrinth" ? modrinthVersion(project, filter, version)
                                    : curseFile(project.toLongLong(), filter, version.toLongLong());
    seen.insert(provider + '/' + f["projectId"].toString());
    const auto info = projectMetadata(provider, f["projectId"].toString(), metadata);
    for (const auto &field : {"title", "icon", "author"})
        if (!info[field].toString().isEmpty())
            f[field] = info[field];
    if (type == "mod" && provider == "Modrinth" &&
        !f["loaders"].toArray().contains(instance["loader"].toString().toLower()))
        throw Error("This dependency does not support the instance's mod loader.");
    f["dependency"] = !out.isEmpty();
    f["required"] = true;
    f["type"] = type;
    f["path"] = contentFolder(type) + '/' + f["filename"].toString();
    contentPath(root + "/validation", f["path"].toString());
    out.append(f);
    for (auto value : f["dependencies"].toArray()) {
        auto dep = value.toObject();
        if (provider == "Modrinth" && dep["dependency_type"] == "required") {
            auto p = dep["project_id"].toString(), ver = dep["version_id"].toString();
            if (p.isEmpty() && !ver.isEmpty())
                p = net.json("https://api.modrinth.com/v2/version/" + encoded(ver))
                        .object()["project_id"]
                        .toString();
            if (p.isEmpty())
                throw Error("A required dependency needs manual installation: " +
                            dep["file_name"].toString());
            resolve(provider, p, "mod", instance, out, seen, ver);
        } else if (provider == "CurseForge" && dep["relationType"].toInt() == 3)
            resolve(provider, QString::number(qint64(dep["modId"].toDouble())), "mod", instance, out, seen);
    }
}
QJsonObject Content::plan(const QJsonObject &project, const QJsonObject &instance) {
    if (project["type"] == "datapack")
        throw Error("Data packs are installed per world. Open the world folder and copy the pack into its "
                    "datapacks folder.");
    if (!QStringList{"mod", "modpack", "resourcepack", "shader"}.contains(project["type"].toString()))
        throw Error("Unsupported content type.");
    if (project["type"] == "modpack") {
        auto f = project["provider"] == "Modrinth"
                     ? modrinthVersion(project["projectId"].toString(), {}, project["versionId"].toString())
                     : curseFile(project["projectId"].toString().toLongLong(), {},
                                 project["versionId"].toString().toLongLong());
        if (f["url"].toString().isEmpty())
            throw Error("This pack requires a browser download. Download its ZIP from CurseForge, then use "
                        "Import pack.");
        QString archive = safePath(root + "/cache/packs", f["filename"].toString());
        net.downloads({{f["url"].toString(), archive, f["hash"].toString(), f["algorithm"].toString(),
                        qint64(f["size"].toDouble(-1))}});
        auto source = project;
        source["versionId"] = f["versionId"];
        auto prepared = pack(archive, source);
        prepared["projectTitle"] = project["title"];
        return prepared;
    }
    if (instance.isEmpty())
        throw Error("Create or select an instance before installing content.");
    if (project["type"] == "mod" && instance["loader"] == "Vanilla")
        throw Error("Mods need a mod loader. Create a Fabric, Forge, NeoForge, or Quilt instance first.");
    QJsonArray files;
    QSet<QString> seen;
    resolve(project["provider"].toString(), project["projectId"].toString(), project["type"].toString(),
            instance, files, seen, project["versionId"].toString(), project);
    return {{"title", "Install " + project["title"].toString()},
            {"projectTitle", project["title"]},
            {"files", files}};
}
QJsonObject Content::updates(const QJsonObject &instance) {
    auto dir = safePath(root + "/instances", instance["id"].toString());
    auto manifest = readObject(dir + "/content.json", true);
    if (manifest.contains("pack")) {
        auto source = manifest["pack"].toObject();
        if (source["projectId"].toString().isEmpty())
            throw Error("This pack was imported locally. Import its newer pack file to a new instance, then "
                        "copy worlds when ready.");
        auto next = plan(source, {});
        if (next["source"].toObject()["versionId"] == source["versionId"])
            return {{"title", "Your pack is up to date"}, {"files", QJsonArray{}}};
        auto desc = next["instance"].toObject();
        if (desc["version"] != instance["version"] || desc["loader"] != instance["loader"] ||
            desc["loaderVersion"] != instance["loaderVersion"])
            throw Error("This pack update changes Minecraft or the loader. Install it as a new instance to "
                        "preserve this setup and its worlds.");
        next.remove("instance");
        next["title"] = "Review complete modpack update";
        next["packUpdate"] = true;
        QSet<QString> keep;
        for (auto v : next["files"].toArray())
            keep.insert(v.toObject()["path"].toString());
        QJsonArray remove;
        for (auto v : manifest["files"].toArray())
            if (!keep.contains(v.toObject()["path"].toString()))
                remove.append(v.toObject()["path"]);
        next["remove"] = remove;
        return next;
    }
    QJsonArray out;
    QSet<QString> seen;
    for (auto v : manifest["files"].toArray()) {
        auto old = v.toObject();
        if (old["projectId"].toString().isEmpty())
            continue;
        QJsonArray resolved;
        resolve(old["provider"].toString(), old["projectId"].toString(), old["type"].toString("mod"),
                instance, resolved, seen);
        for (auto f : resolved) {
            auto item = f.toObject();
            bool same = false;
            for (auto x : manifest["files"].toArray()) {
                auto known = x.toObject();
                if (known["provider"] == item["provider"] && known["projectId"] == item["projectId"] &&
                    known["versionId"] == item["versionId"])
                    same = true;
            }
            if (!same)
                out.append(item);
        }
    }
    return {{"title", out.isEmpty() ? "Everything is up to date" : "Review available updates"},
            {"files", out}};
}
QJsonObject Content::pack(const QString &archive, const QJsonObject &source) {
    net.ctx.report("Reading pack manifest…");
    QByteArray manifest;bool modrinth=true;
    try{manifest=zipEntry(archive,"modrinth.index.json");}
    catch(const std::exception &error){if(!QString::fromUtf8(error.what()).startsWith("Archive is missing "))throw;modrinth=false;manifest=zipEntry(archive,"manifest.json");}
    QJsonParseError parseError;const auto parsed=QJsonDocument::fromJson(manifest,&parseError);
    if(parseError.error!=QJsonParseError::NoError||!parsed.isObject())throw Error("This pack has an invalid manifest.");
    QJsonObject descriptor;
    QJsonArray files;
    QString prefix;
    if (modrinth) {
        auto index = parsed.object();
        if (index["formatVersion"].toInt() != 1 || index["game"] != "minecraft")
            throw Error("Unsupported Modrinth pack format.");
        auto deps = index["dependencies"].toObject();
        descriptor = {{"name", index["name"]}, {"version", deps["minecraft"]}, {"loader", "Vanilla"}};
        for (const auto &pair : QList<QPair<QString, QString>>{{"fabric-loader", "Fabric"},
                                                               {"quilt-loader", "Quilt"},
                                                               {"forge", "Forge"},
                                                               {"neoforge", "NeoForge"}})
            if (deps.contains(pair.first)) {
                if (descriptor["loader"] != "Vanilla")
                    throw Error("Pack requests multiple loaders.");
                descriptor["loader"] = pair.second;
                descriptor["loaderVersion"] = deps[pair.first];
            }
        for (auto i = deps.begin(); i != deps.end(); ++i)
            if (!QStringList{"minecraft", "fabric-loader", "quilt-loader", "forge", "neoforge"}.contains(
                    i.key()))
                throw Error("Unknown pack requirement: " + i.key());
        for (auto v : index["files"].toArray()) {
            auto o = v.toObject();
            if (o["env"].toObject()["client"] == "unsupported")
                continue;
            auto urls = o["downloads"].toArray();
            QString url;
            for (auto u : urls) {
                QUrl candidate(u.toString());
                if (QStringList{"cdn.modrinth.com", "github.com", "raw.githubusercontent.com", "gitlab.com"}
                        .contains(candidate.host()) &&
                    candidate.scheme() == "https") {
                    url = u.toString();
                    break;
                }
            }
            if (url.isEmpty())
                throw Error("Pack file has no supported HTTPS download: " + o["path"].toString());
            auto hash = o["hashes"].toObject()["sha512"].toString();
            if (hash.isEmpty())
                throw Error("Pack file is missing its SHA-512 hash.");
            contentPath(root+"/cache/imports", o["path"].toString());
            files.append(QJsonObject{{"path", o["path"]},
                                     {"filename", QFileInfo(o["path"].toString()).fileName()},
                                     {"title", QFileInfo(o["path"].toString()).fileName()},
                                     {"url", url},
                                     {"hash", hash},
                                     {"algorithm", "sha512"},
                                     {"size", o["fileSize"]},
                                     {"provider", "Modrinth pack"}});
        }
        prefix = "overrides";
    } else {
        auto index = parsed.object();
        if (index["manifestType"] != "minecraftModpack" || index["manifestVersion"].toInt() != 1)
            throw Error("Unsupported CurseForge pack manifest.");
        auto mc = index["minecraft"].toObject();
        descriptor = {{"name", index["name"]}, {"version", mc["version"]}, {"loader", "Vanilla"}};
        for (auto v : mc["modLoaders"].toArray()) {
            auto loader = v.toObject()["id"].toString();
            auto dash = loader.indexOf('-');
            QString kind = loader.left(dash), version = loader.mid(dash + 1);
            if (kind == "forge")
                descriptor["loader"] = "Forge";
            else if (kind == "neoforge")
                descriptor["loader"] = "NeoForge";
            else if (kind == "fabric")
                descriptor["loader"] = "Fabric";
            else if (kind == "quilt")
                descriptor["loader"] = "Quilt";
            else
                throw Error("Unsupported pack loader: " + kind);
            descriptor["loaderVersion"] = version;
        }
        for (auto v : index["files"].toArray()) {
            auto o = v.toObject();
            if (!o["required"].toBool(true))
                continue;
            auto f = curseFile(qint64(o["projectID"].toDouble()), {}, qint64(o["fileID"].toDouble()));
            auto project = net.json(QString("https://api.curseforge.com/v1/mods/%1")
                                        .arg(qint64(o["projectID"].toDouble())),
                                    {}, {{"x-api-key", key.toUtf8()}})
                               .object()["data"]
                               .toObject();
            int cls = project["classId"].toInt();
            auto type = cls == 12 ? "resourcepack" : cls == 6552 ? "shader" : "mod";
            f["type"] = type;
            f["path"] = contentFolder(type) + '/' + f["filename"].toString();
            f["page"] =
                project["links"].toObject()["websiteUrl"].toString() + "/files/" + f["versionId"].toString();
            files.append(f);
        }
        prefix = index["overrides"].toString("overrides");
        safePath(root+"/cache", prefix);
    }
    if (descriptor["version"].toString().isEmpty())
        throw Error("Pack does not specify a Minecraft version.");
    QDir().mkpath(root+"/cache/imports");
    QTemporaryDir stored(root+"/cache/imports/pack-XXXXXX");
    if(!stored.isValid())throw Error("Cannot create pack staging folder.");
    // Staging is private and discarded on failure. Write each override once;
    // only the later instance transaction exposes files to the working game.
    for(const auto &folder:QStringList{prefix,"client-overrides"}){
        net.ctx.report("Preparing client overrides…");
        extractZip(archive,stored.path(),folder+"/",false,net.ctx,false);
    }
    QHash<QString,qsizetype> positions;
    for(qsizetype i=0;i<files.size();++i){
        const auto path=files[i].toObject()["path"].toString().toLower();
        if(positions.contains(path))throw Error("Pack contains duplicate client paths.");
        positions[path]=i;
    }
    QStringList localFiles;QDirIterator it(stored.path(),QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())localFiles<<it.next();
    QElapsedTimer progress;progress.start();qint64 checked=0;
    for(const auto &from:localFiles){
        net.ctx.check();const auto rel=QDir(stored.path()).relativeFilePath(from);
        const auto dest=contentPath(stored.path(),rel);
        QJsonObject file{{"path",rel},{"filename",QFileInfo(rel).fileName()},{"title",rel},{"local",dest},{"hash",fileHash(dest,"sha512")},{"algorithm","sha512"},{"provider","Pack override"}};
        const auto key=rel.toLower();
        if(positions.contains(key))files[positions[key]]=file;
        else{positions[key]=files.size();files.append(file);}
        ++checked;if(checked==localFiles.size()||progress.elapsed()>=100){net.ctx.report("Checking prepared pack files",checked,localFiles.size());progress.restart();}
    }
    net.ctx.check();stored.setAutoRemove(false);
    auto origin = source;
    origin["type"] = "modpack";
    return {{"title", "Import " + descriptor["name"].toString()},
            {"instance", descriptor},
            {"files", files},
            {"source", origin}};
}
void Content::apply(const QJsonObject &instance, const QJsonObject &plan) {
    auto dir = safePath(root + "/instances", instance["id"].toString()), game = dir + "/game";
    QDir().mkpath(dir);
    QTemporaryDir staging(dir + "/content-stage-XXXXXX");
    if (!staging.isValid())
        throw Error("Cannot create staging folder.");
    auto manifest = readObject(dir + "/content.json", true);
    auto oldFiles = manifest["files"].toArray(), next = oldFiles;
    QJsonArray files = plan["files"].toArray();
    QList<Download> downloads;
    QSet<QString> writes;
    for (auto v : files) {
        auto f = v.toObject();
        auto rel = f["path"].toString(), dest = contentPath(staging.path(), rel);
        if (writes.contains(rel.toLower()))
            throw Error("Two projects write the same file: " + rel);
        writes.insert(rel.toLower());
        if (f["local"].toString().isEmpty()) {
            if (f["url"].toString().isEmpty())
                throw Error("Resolve browser downloads before applying this plan.");
            downloads << Download{f["url"].toString(), dest, f["hash"].toString(),
                                  f["algorithm"].toString("sha1"), qint64(f["size"].toDouble(-1))};
        } else {
            QDir().mkpath(QFileInfo(dest).absolutePath());
            if (!QFile::copy(f["local"].toString(), dest))
                throw Error("Cannot copy imported file.");
            if (fileHash(dest, f["algorithm"].toString()) != f["hash"].toString())
                throw Error("Imported file does not match the expected hash.");
        }
    }
    net.downloads(downloads);
    net.ctx.check();
    QSet<QString> affected;
    for (auto v : plan["remove"].toArray())
        affected.insert(v.toString());
    for (auto v : files) {
        auto f = v.toObject();
        affected.insert(f["path"].toString());
        for (qsizetype i = next.size() - 1; i >= 0; --i) {
            auto old = next[i].toObject();
            if (old["path"] == f["path"] ||
                (!f["projectId"].toString().isEmpty() && old["provider"] == f["provider"] &&
                 old["projectId"] == f["projectId"])) {
                affected.insert(old["path"].toString());
                next.removeAt(i);
            }
        }
        f.remove("local");
        f.remove("dependencies");
        f["hash"] =
            fileHash(contentPath(staging.path(), f["path"].toString()), f["algorithm"].toString("sha1"));
        next.append(f);
    }
    for (qsizetype i = next.size() - 1; i >= 0; --i)
        if (plan["remove"].toArray().contains(next[i].toObject()["path"]))
            next.removeAt(i);
    QString backup = dir + "/backups/" + timestamp();
    QJsonArray entries;
    for (const auto &rel : affected) {
        auto from = contentPath(game, rel);
        bool exists = QFileInfo::exists(from);
        entries.append(QJsonObject{{"path", rel}, {"existed", exists}});
        if (exists) {
            auto to = contentPath(backup + "/files", rel);
            QDir().mkpath(QFileInfo(to).absolutePath());
            if (!QFile::copy(from, to))
                throw Error("Cannot back up " + rel);
        }
    }
    QJsonObject journal{{"entries", entries}, {"manifest", manifest}, {"backup", backup}};
    writeObject(backup + "/backup.json", journal);
    writeObject(dir + "/transaction.json", journal);
    try {
        // Cancellation is intentionally deferred during this short commit; a complete rollback is always
        // available.
        for (const auto &rel : affected) {
            auto target = contentPath(game, rel);
            if (QFileInfo::exists(target) && !QFile::remove(target))
                throw Error("Cannot replace " + rel + ". Close Minecraft and try again.");
        }
        for (auto v : files) {
            auto f = v.toObject();
            auto rel = f["path"].toString(), to = contentPath(game, rel);
            QDir().mkpath(QFileInfo(to).absolutePath());
            if (!QFile::copy(contentPath(staging.path(), rel), to))
                throw Error("Cannot commit " + rel);
        }
        manifest["schemaVersion"] = 1;
        manifest["files"] = next;
        if (plan.contains("source"))
            manifest["pack"] = plan["source"];
        writeObject(dir + "/content.json", manifest);
        if (!QFile::remove(dir + "/transaction.json"))
            throw Error("Cannot finish the transaction journal.");
    } catch (...) {
        recover(instance);
        throw;
    }
}
void Content::restore(const QJsonObject &instance, const QString &backup) {
    auto dir = safePath(root + "/instances", instance["id"].toString());
    auto journal = readObject(safePath(dir + "/backups", backup) + "/backup.json");
    journal["backup"] = safePath(dir + "/backups", backup);
    writeObject(dir + "/transaction.json", journal);
    recover(instance);
}
void Content::recover(const QJsonObject &instance) {
    auto dir = safePath(root + "/instances", instance["id"].toString());
    if (!QFileInfo::exists(dir + "/transaction.json"))
        return;
    auto journal = readObject(dir + "/transaction.json");
    auto backup = journal["backup"].toString();
    if (!QDir::cleanPath(backup).startsWith(QDir::cleanPath(dir + "/backups") + '/'))
        throw Error("Invalid recovery location.");
    for (auto v : journal["entries"].toArray()) {
        auto e = v.toObject();
        auto to = contentPath(dir + "/game", e["path"].toString());
        if (QFileInfo::exists(to) && !QFile::remove(to))
            throw Error("Cannot restore " + to);
        if (e["existed"].toBool()) {
            QDir().mkpath(QFileInfo(to).absolutePath());
            if (!QFile::copy(contentPath(backup + "/files", e["path"].toString()), to))
                throw Error("Recovery failed. Backup is preserved at " + backup);
        }
    }
    writeObject(dir + "/content.json", journal["manifest"].toObject());
    QFile::remove(dir + "/transaction.json");
}
static QJsonObject embeddedModMetadata(const QString &archive, const QString &cacheRoot) {
    const QFileInfo file(archive);
    if (file.isSymLink() || file.size() <= 0 || file.size() > 128 * 1024 * 1024)
        return {};
    const auto identity = QByteArray("artwork-v2") + file.absoluteFilePath().toUtf8() + '\0' + QByteArray::number(file.size()) + '\0' +
                          QByteArray::number(file.lastModified().toMSecsSinceEpoch());
    const auto cacheKey =
        QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
    const auto metadataPath = cacheRoot + '/' + cacheKey + ".json";
    try {
        if (QFileInfo::exists(metadataPath))
            return readObject(metadataPath);
        QJsonObject metadata;
        QZipReader zip(archive);
        if (!zip.isReadable() || zip.status() != QZipReader::NoError) {
            writeObject(metadataPath, metadata);
            return metadata;
        }
        const auto entries = zip.fileInfoList();
        if (entries.size() > 100000)
            return {};
        const auto readSmall = [&](const QString &name, qint64 maximum) -> QByteArray {
            try {
                safePath(cacheRoot + "/entry-validation", name);
            } catch (const std::exception &) {
                return {};
            }
            for (const auto &entry : entries) {
                if (entry.filePath != name)
                    continue;
                if (!entry.isFile || entry.isSymLink || entry.size <= 0 || entry.size > maximum)
                    return {};
                const auto bytes = zip.fileData(name);
                return bytes.size() == entry.size && zip.status() == QZipReader::NoError ? bytes
                                                                                         : QByteArray();
            }
            return {};
        };
        QString icon;
        if (!readSmall("pack.png",4*1024*1024).isEmpty()) icon="pack.png";
        const auto fabric = QJsonDocument::fromJson(readSmall("fabric.mod.json", 1024 * 1024)).object();
        if (!fabric.isEmpty()) {
            metadata["title"] = fabric["name"];
            QStringList authors;
            for (const auto &author : fabric["authors"].toArray()) {
                const auto name =
                    author.isString() ? author.toString() : author.toObject()["name"].toString();
                if (!name.isEmpty())
                    authors << name;
            }
            metadata["author"] = authors.join(", ");
            if (fabric["icon"].isString())
                icon = fabric["icon"].toString();
            else {
                const auto icons = fabric["icon"].toObject();
                int best = -1;
                for (auto entry = icons.begin(); entry != icons.end(); ++entry) {
                    bool numeric = false;
                    const auto size = entry.key().toInt(&numeric);
                    if (numeric && size > best) {
                        best = size;
                        icon = entry.value().toString();
                    }
                }
            }
        }
        if (metadata["title"].toString().isEmpty()) {
            const auto quilt = QJsonDocument::fromJson(readSmall("quilt.mod.json", 1024 * 1024))
                                   .object()["quilt_loader"]
                                   .toObject()["metadata"]
                                   .toObject();
            if (!quilt.isEmpty()) {
                metadata["title"] = quilt["name"];
                icon = quilt["icon"].toString();
                metadata["author"] = quilt["contributors"].toObject().keys().join(", ");
            }
        }
        if (metadata["title"].toString().isEmpty()) {
            const auto legacy = QJsonDocument::fromJson(readSmall("mcmod.info", 1024 * 1024));
            auto mods = legacy.isArray() ? legacy.array() : legacy.object()["modList"].toArray();
            if (!mods.isEmpty()) {
                const auto mod = mods[0].toObject();
                metadata["title"] = mod["name"];
                icon = mod["logoFile"].toString();
                QStringList authors;
                for (const auto &author : mod["authorList"].toArray())
                    authors << author.toString();
                metadata["author"] = authors.join(", ");
            }
        }
        if (!icon.isEmpty()) {
            const auto bytes = readSmall(icon, 4 * 1024 * 1024);
            QString extension;
            if (bytes.startsWith(QByteArray::fromHex("89504e470d0a1a0a")))
                extension = ".png";
            else if (bytes.startsWith(QByteArray::fromHex("ffd8ff")))
                extension = ".jpg";
            else if (bytes.startsWith("GIF87a") || bytes.startsWith("GIF89a"))
                extension = ".gif";
            else if (bytes.startsWith("RIFF") && bytes.mid(8, 4) == "WEBP")
                extension = ".webp";
            if (!extension.isEmpty()) {
                const auto iconPath = cacheRoot + '/' + cacheKey + extension;
                writeBytes(iconPath, bytes);
                metadata["icon"] = QUrl::fromLocalFile(iconPath).toString();
            }
        }
        writeObject(metadataPath, metadata);
        return metadata;
    } catch (const std::exception &) {
        // A bad optional logo must never make the installed mod list disappear.
        return {};
    }
}
static QString enabledContentPath(QString path) {
    if (path.endsWith(".disabled", Qt::CaseInsensitive))
        path.chop(9);
    return path;
}
QJsonArray Content::files(const QJsonObject &instance, const QString &type) {
    auto dir = safePath(root + "/instances", instance["id"].toString());
    auto manifest = readObject(dir + "/content.json", true)["files"].toArray();
    auto folder = contentFolder(type);
    QJsonArray out;
    for (const auto &f : QDir(dir + "/game/" + folder).entryInfoList(QDir::Files, QDir::Name)) {
        QJsonObject item{{"name", f.fileName()},
                         {"path", folder + '/' + f.fileName()},
                         {"enabled", !f.fileName().endsWith(".disabled")},
                         {"provider", "Local file"},
                         {"size", double(f.size())}};
        for (auto v : manifest) {
            auto o = v.toObject();
            if (enabledContentPath(o["path"].toString())
                    .compare(enabledContentPath(item["path"].toString()), Qt::CaseInsensitive) == 0) {
                for (const auto &field :
                     {"provider", "versionName", "title", "icon", "author", "projectId", "versionId"})
                    if (!o[field].toString().isEmpty())
                        item[field] = o[field];
                break;
            }
        }
        item["type"] = type;
        if (item["icon"].toString().isEmpty() || (type == "mod" && (item["title"].toString().isEmpty() ||
                              item["author"].toString().isEmpty()))) {
            const auto embedded = embeddedModMetadata(f.absoluteFilePath(), root + "/cache/mod-logos");
            for (const auto &field : {"title", "icon", "author"})
                if (item[field].toString().isEmpty() && !embedded[field].toString().isEmpty())
                    item[field] = embedded[field];
        }
        out.append(item);
    }
    return out;
}
} // namespace moly
