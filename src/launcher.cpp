#include "launcher.h"
#include "accounts.h"
#include "content.h"
#include "minecraft.h"
#include "artwork.h"
#include "packexport.h"
#include "playtargets.h"
#include "skinstore.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <QtConcurrent>

using namespace moly;
static QString localPath(const QString &u) {
    auto url = QUrl(u);
    return url.isLocalFile() ? url.toLocalFile() : u;
}
Launcher::Launcher(QString location, QObject *parent) : QObject(parent), root(std::move(location)) {
    QDir().mkpath(root);
    load();
    logTimer.setInterval(150);
    connect(&logTimer, &QTimer::timeout, this, [this] {
        if (dirtyLogs) {
            dirtyLogs = false;
            emit logsChanged();
        }
    });
    logTimer.start();
    connect(&iconWatcher,&QFutureWatcher<QJsonArray>::finished,this,[this]{
        QJsonArray ready;try{ready=iconWatcher.result();}catch(...){return;}
        for(const auto &value:ready){
            const auto result=value.toObject();const auto id=result["id"].toString();
            if(processes.contains(id))continue;
            auto found=std::find_if(instanceList.begin(),instanceList.end(),[&](const auto &item){return item["id"]==id;});
            if(found==instanceList.end()||!artworkUrl(instanceDir(*found),(*found)["iconFile"].toString(),(*found)["iconRevision"].toString()).isEmpty())continue;
            try{
                auto instance=*found;QFile source(result["path"].toString());if(!source.open(QIODevice::ReadOnly)||source.size()>16*1024*1024)continue;
                const auto filename=result["file"].toString();if(!QStringList{"icon.png","icon.gif"}.contains(filename))continue;
                writeBytes(safePath(instanceDir(instance),filename),source.readAll());instance["iconFile"]=filename;
                instance["iconRevision"]=QUuid::createUuid().toString(QUuid::WithoutBraces);instance["iconOrigin"]="modpack";storeInstance(instance);
            }catch(const std::exception &){}
        }
        if(iconsAgain){iconsAgain=false;QTimer::singleShot(0,this,&Launcher::refreshPackIcons);}
    });
    QTimer::singleShot(0, this, [this] { refreshVersions(false); });
    QTimer::singleShot(250,this,&Launcher::refreshPackIcons);
}
Launcher::~Launcher() {
    choicesCancelled=true;choicesWatcher.waitForFinished();
    iconCancelled=true;iconWatcher.waitForFinished();
    cancelled = true;
    watcher.waitForFinished();
    for (auto *p : processes) {
        p->disconnect(this);
        p->kill();
        p->waitForFinished(3000);
    }
}
QString Launcher::instanceDir(const QJsonObject &i) const {
    return safePath(root + "/instances", i["id"].toString());
}
void Launcher::load() {
    config = readObject(root + "/settings.json", true);
    if (config.isEmpty())
        config = {{"schemaVersion", 1},
                  {"memoryMb", qMin(4096, qMax(1024, int(physicalMemoryMb() / 2 / 256 * 256)))},
                  {"width", 1280},
                  {"height", 720},
                  {"fullscreen", false},
                  {"reducedMotion", false},
                  {"keepVisible", true},
                  {"javaPath", ""},
                  {"jvmArgs", ""},
                  {"microsoftClientId", QString::fromUtf8(qgetenv("MOLY_MICROSOFT_CLIENT_ID"))}};
    if (!QRegularExpression("^#[0-9a-fA-F]{6}$").match(config["accentColor"].toString()).hasMatch())
        config["accentColor"] = "#8b5cf6";
    accountList = readObject(root + "/accounts.json", true)["accounts"].toArray();
    reloadInstances();
    auto cached = readObject(root + "/cache/version-manifest.json", true);
    for (auto v : cached["versions"].toArray())
        if (v.toObject()["type"] == "release")
            versionList.append(v);
}
void Launcher::persist() {
    writeObject(root + "/settings.json", config);
    writeObject(root + "/accounts.json", {{"schemaVersion", 1}, {"accounts", accountList}});
    emit changed();
}
void Launcher::reloadInstances() {
    instanceList.clear();
    for (const auto &d : QDir(root + "/instances").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        try {
            auto i = readObject(root + "/instances/" + d + "/instance.json");
            if (i["id"] != d)
                throw Error("Instance folder and ID do not match.");
            Content(root).recover(i);
            instanceList << i;
        } catch (const std::exception &e) {
            QTimer::singleShot(0, this,
                               [this, msg = QString::fromUtf8(e.what())] { emit notice(msg, true); });
        }
    }
    if (current().isEmpty())
        selection = instanceList.isEmpty() ? QString() : instanceList.first()["id"].toString();
    emit changed();
}
QJsonObject Launcher::current() const {
    for (auto i : instanceList)
        if (i["id"] == selection)
            return i;
    return {};
}
QVariantList Launcher::instances() const {
    QVariantList out;
    const auto now = QDateTime::currentDateTimeUtc();
    for (auto instance : instanceList) {
        const auto dir = instanceDir(instance);
        instance["running"] = processes.contains(instance["id"].toString());
        instance["installed"] = QFileInfo::exists(dir + "/installation.json");
        instance["status"] = instance["running"].toBool()     ? "Running"
                             : instance["installed"].toBool() ? "Installed"
                                                              : "Not installed";
        instance["modCount"] =
            QDir(dir + "/game/mods").entryList({"*.jar", "*.jar.disabled"}, QDir::Files).size();
        instance["iconUrl"] = QString();
        instance["iconUrl"] = artworkUrl(dir,instance["iconFile"].toString(),instance["iconRevision"].toString());
        instance["backgroundUrl"] = artworkUrl(dir,instance["backgroundFile"].toString(),instance["backgroundRevision"].toString());
        auto last = QDateTime::fromString(instance["lastPlayed"].toString(), Qt::ISODate);
        QString relative = "Never";
        if (last.isValid()) {
            const auto seconds = qMax<qint64>(0, last.secsTo(now));
            relative = seconds < 60       ? "Just now"
                       : seconds < 3600   ? QString("%1 min ago").arg(seconds / 60)
                       : seconds < 86400  ? QString("%1 hr ago").arg(seconds / 3600)
                       : seconds < 172800 ? "Yesterday"
                                          : QString("%1 days ago").arg(seconds / 86400);
        }
        instance["lastPlayedLabel"] = relative;
        qint64 seconds = qint64(instance["playtimeSeconds"].toDouble());
        if (startedAt.contains(instance["id"].toString()))
            seconds += startedAt[instance["id"].toString()].secsTo(now);
        instance["playtimeMinutes"] = double(qMax<qint64>(0, seconds) / 60);
        out << instance.toVariantMap();
    }
    return out;
}
QString Launcher::installTargetId() const {
    const auto saved = config["lastInstallInstance"].toString();
    for (const auto &instance : instanceList)
        if (instance["id"] == saved)
            return saved;
    return selection;
}
void Launcher::setInstallError(const QString &message) {
    if(!message.isEmpty()) automaticInstall=false;
    installErrorText = message;
    emit installStateChanged();
    if (!message.isEmpty())
        emit installFailed(message);
}
QVariantMap Launcher::selected() const {
    for (auto v : instances())
        if (v.toMap()["id"].toString() == selection)
            return v.toMap();
    return {};
}
QVariantMap Launcher::activeAccount() const {
    for (auto v : accountList)
        if (v.toObject()["id"] == config["activeAccount"])
            return v.toObject().toVariantMap();
    return {};
}
bool Launcher::curseConnected() const {
    return !Secrets::get("curseforge").isEmpty() || !qgetenv("MOLY_CURSEFORGE_API_KEY").isEmpty();
}
static QString curseKey() {
    auto k = Secrets::get("curseforge");
    return QString::fromUtf8(k.isEmpty() ? qgetenv("MOLY_CURSEFORGE_API_KEY") : k);
}
void Launcher::selectInstance(const QString &id) {
    if (selection == id)
        return;
    bool exists = false;
    for (const auto &instance : instanceList)
        if (instance["id"] == id) {
            exists = true;
            break;
        }
    if (!exists)
        return;
    selection = id;
    emit changed();
    emit logsChanged();
}
void Launcher::storeInstance(const QJsonObject &i) {
    writeObject(instanceDir(i) + "/instance.json", i);
    reloadInstances();
}
bool Launcher::editable() {
    if (working) {
        emit notice("Let the current task finish, or cancel it from the status bar.", true);
        return false;
    }
    if (current().isEmpty()) {
        emit notice("Select an instance first.", true);
        return false;
    }
    if (processes.contains(selection)) {
        emit notice("Stop this instance before changing its files or settings.", true);
        return false;
    }
    return true;
}
void Launcher::task(const QString &title, std::function<QJsonObject(Context)> work,
                    std::function<void(QJsonObject)> done, const QString &instanceId,
                    const QString &operation) {
    if (working) {
        emit notice("Another task is in progress. You can cancel it from the status bar.", true);
        return;
    }
    const bool searchOperation = operation == "search" || operation == "search-more";
    const bool recordJob = !searchOperation && operation!="catalog" && operation!="loaders";
    const auto searchTicket = searchGeneration;
    if (recordJob) {
        retryTitle = title;
        retryWork = work;
        retryDone = done;
        retryInstanceId = instanceId;
        retryOperation = operation;
    }
    working = true;
    lastTaskFailed = false;
    activeTaskInstanceId = instanceId;
    activeTaskOperation = operation;
    cancelled = false;
    const auto generation = ++taskGeneration;
    statusText = title;
    progressValue = -1;
    transferSpeed = 0;transferBytes = 0;transferTotal = -1;
    if (recordJob) {
        jobList.prepend(
            QVariantMap{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"instanceId",instanceId},{"operation",operation},{"startedAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"title", title}, {"detail", "Starting…"}, {"state", "Working"}, {"progress", -1.0}});
        while (jobList.size() > 30)
            jobList.removeLast();
    }
    emit busyChanged();
    emit statusChanged();
    emit jobsChanged();
    Context context{&cancelled, [this, generation, recordJob](QString text, qint64 doneCount, qint64 total) {
                        QMetaObject::invokeMethod(
                            this,
                            [this, generation, recordJob, text, doneCount, total] {
                                if (generation != taskGeneration || !working)
                                    return;
                                statusText = redact(text.left(300));
                                if(!text.startsWith("Downloading ")){transferSpeed=0;transferBytes=0;transferTotal=-1;}
                                if (total > 0) {
                                    const auto value = double(doneCount) / double(total);
                                    progressValue = qBound(0.0,value,1.0);
                                } else {
                                    progressValue = -1;
                                }
                                if (progressValue > 1)
                                    progressValue = 1;
                                if (recordJob && !jobList.isEmpty()) {
                                    auto job = jobList[0].toMap();
                                    job["detail"] = statusText;
                                    job["progress"] = progressValue;
                                    jobList[0] = job;
                                }
                                emit statusChanged();
                                emit jobsChanged();
                            },
                            Qt::QueuedConnection);
                    }};
    context.transfer = [this,generation,recordJob](qint64 bytes,qint64 total,double speed){
        QMetaObject::invokeMethod(this,[this,generation,recordJob,bytes,total,speed]{
            if(generation!=taskGeneration||!working)return;
            transferBytes=bytes;transferTotal=total;transferSpeed=speed;
            if(recordJob&&!jobList.isEmpty()){
                auto job=jobList[0].toMap();job["bytes"]=bytes;job["total"]=total;job["speed"]=speed;jobList[0]=job;
            }
            emit statusChanged();emit jobsChanged();
        },Qt::QueuedConnection);
    };
    disconnect(&watcher, nullptr, this, nullptr);
    connect(&watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, done, generation, operation, searchOperation, recordJob, searchTicket] {
                if (generation != taskGeneration)
                    return;
                QJsonObject result;
                try {
                    result = watcher.result();
                } catch (const std::exception &error) {
                    result = {{"_error", redact(QString::fromUtf8(error.what()))}};
                } catch (...) {
                    result = {
                        {"_error",
                         "The task stopped unexpectedly. Your existing files are preserved; please retry."}};
                }
                bool failed = result.contains("_error");
                // Set the finished state before notifying QML; a busyChanged handler can start another task.
                working = false;
                transferSpeed = 0;
                activeTaskInstanceId.clear();
                activeTaskOperation.clear();
                deviceCode.clear();
                statusText = failed ? result["_error"].toString() : "Task complete";
                progressValue = failed ? 0 : 1;
                if (recordJob && !jobList.isEmpty()) {
                    auto job = jobList[0].toMap();
                    job["state"] = failed ? (cancelled ? "Cancelled" : "Failed") : "Complete";
                    job["detail"] = statusText;
                    job["progress"] = progressValue;
                    job["finishedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                    jobList[0] = job;
                }
                if (!failed && done) {
                    try {
                        done(result);
                    } catch (const std::exception &error) {
                        failed = true;
                        statusText = redact(QString::fromUtf8(error.what()));
                    } catch (...) {
                        failed = true;
                        statusText =
                            "The task completed, but its result could not be displayed. Please retry.";
                    }
                }
                if (failed && (!searchOperation || searchTicket == searchGeneration)) {
                    automaticInstall = false;
                    emit installStateChanged();
                    if (recordJob && !jobList.isEmpty() && generation == taskGeneration) {
                        auto job = jobList[0].toMap();
                        job["state"] = "Failed";
                        job["detail"] = statusText;
                        jobList[0] = job;
                    }
                    if (operation == "search" || operation == "search-more") {
                        searchBusy = false;
                        searchMore = false;
                        searchErrorText = statusText;
                        emit searchStateChanged();
                    }
                    if (!searchOperation) {
                        setInstallError(statusText);
                        emit notice(statusText, true);
                    }
                }
                if (generation == taskGeneration) {
                    lastTaskFailed = recordJob && failed;
                    emit loginChanged();
                    emit busyChanged();
                    emit jobsChanged();
                    emit statusChanged();
                    emit changed();
                }
                if (pendingSearch)
                    QTimer::singleShot(0, this, [this] {
                        if (pendingSearch && !working)
                            startSearch(false);
                    });
            });
    watcher.setFuture(QtConcurrent::run([work, context] {
        try {
            return work(context);
        } catch (const std::exception &error) {
            return QJsonObject{{"_error", redact(QString::fromUtf8(error.what()))}};
        } catch (...) {
            return QJsonObject{{"_error", "The task stopped unexpectedly. Please retry."}};
        }
    }));
}
void Launcher::cancel() {
    cancelled = true;
}
void Launcher::retry() {
    if (!working && !searchErrorText.isEmpty())
        startSearch(lastSearchAppend);
    else if (!working && retryWork) {
        automaticInstall = retryOperation == "resolve-auto";
        task(retryTitle, retryWork, retryDone, retryInstanceId, retryOperation);
    }
}
void Launcher::refreshVersions(bool snapshots) {
    auto base = root;
    task(
        "Refreshing Minecraft versions",
        [base, snapshots](Context c) {
            return QJsonObject{{"versions", Minecraft(base, c).versions(snapshots)}};
        },
        [this](QJsonObject r) {
            versionList = r["versions"].toArray();
            emit catalogChanged();
        },{},"catalog");
}
void Launcher::refreshLoaders(const QString &version, const QString &loader) {
    loaderList.clear();
    emit catalogChanged();
    if (loader == "Vanilla")
        return;
    auto base = root;
    task(
        "Finding " + loader + " versions",
        [base, version, loader](Context c) {
            return QJsonObject{
                {"loaders", QJsonArray::fromStringList(Minecraft(base, c).loaders(version, loader))}};
        },
        [this](QJsonObject r) {
            for (auto v : r["loaders"].toArray())
                loaderList << v.toString();
            emit catalogChanged();
        },{},"loaders");
}
void Launcher::createInstance(const QString &name, const QString &version, const QString &loader,
                              const QString &loaderVersion, bool autoInstall) {
    if (working)
        return;
    try {
        if (name.trimmed().isEmpty() || version.isEmpty())
            throw Error("Choose a name and Minecraft version.");
        if (!QStringList{"Vanilla", "Fabric", "Forge", "NeoForge", "Quilt"}.contains(loader))
            throw Error("Choose a supported loader.");
        if (loader != "Vanilla" && loaderVersion.isEmpty())
            throw Error("Choose a loader version.");
        auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QJsonObject i{{"schemaVersion", 1},      {"id", id},         {"name", name.trimmed().left(80)},
                      {"version", version},      {"loader", loader}, {"loaderVersion", loaderVersion},
                      {"group", "My instances"}, {"icon", "◇"},      {"settings", QJsonObject{}}};
        QDir().mkpath(instanceDir(i) + "/game");
        storeInstance(i);
        selectInstance(id);
        emit navigate("instance");
        if (autoInstall)
            installSelected();
        else
            emit notice("Instance created.", false);
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::saveInstance(const QVariantMap &values) {
    if (!editable())
        return;
    try {
        auto i = current();
        auto v = QJsonObject::fromVariantMap(values);
        for (const auto &k : {"name", "group", "icon", "settings"})
            if (v.contains(k))
                i[k] = v[k];
        if (i["name"].toString().trimmed().isEmpty())
            throw Error("Instance name cannot be empty.");
        auto s = i["settings"].toObject();
        if (s.contains("memoryMb") &&
            (s["memoryMb"].toInt() < 512 || s["memoryMb"].toInt() > memoryTotal() - 512))
            throw Error("Choose a valid memory allocation.");
        if ((s.contains("width") && (s["width"].toInt() < 640 || s["width"].toInt() > 7680)) ||
            (s.contains("height") && (s["height"].toInt() < 360 || s["height"].toInt() > 4320)))
            throw Error("Choose a game resolution between 640 � 360 and 7680 � 4320.");
        storeInstance(i);
        emit notice("Instance settings saved.", false);
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::setInstanceIcon(const QString &url) {
    if(!editable())return;
    try {
        auto instance=current();
        instance["iconFile"]=prepareArtwork(url,instanceDir(instance),false);
        instance["iconRevision"]=QUuid::createUuid().toString(QUuid::WithoutBraces);
        storeInstance(instance);
        emit notice("Instance image updated.",false);
    }catch(const std::exception &e){emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::resetInstanceIcon() {
    if(!editable())return;
    try {
        auto instance=current();instance.remove("iconFile");instance.remove("iconRevision");storeInstance(instance);
        for(const auto &name:QStringList{"icon.png","icon.gif"})QFile::remove(safePath(instanceDir(instance),name));
        emit notice("Default instance image restored.",false);
        refreshPackIcons();
    }catch(const std::exception &e){emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::setInstanceBackground(const QString &url) {
    if(!editable())return;
    try {
        auto instance=current();instance["backgroundFile"]=prepareArtwork(url,instanceDir(instance),true);
        instance["backgroundRevision"]=QUuid::createUuid().toString(QUuid::WithoutBraces);storeInstance(instance);
        emit notice("Instance background updated.",false);
    }catch(const std::exception &e){emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::resetInstanceBackground() {
    if(!editable())return;
    try {
        auto instance=current();instance.remove("backgroundFile");instance.remove("backgroundRevision");storeInstance(instance);
        for(const auto &name:QStringList{"background.png","background.gif"})QFile::remove(safePath(instanceDir(instance),name));
    }catch(const std::exception &e){emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::refreshPackIcons() {
    if(iconWatcher.isRunning()){iconsAgain=true;return;}
    QJsonArray pending;
    for(const auto &instance:instanceList){
        if(processes.contains(instance["id"].toString())||!artworkUrl(instanceDir(instance),instance["iconFile"].toString(),instance["iconRevision"].toString()).isEmpty())continue;
        try{const auto pack=readObject(instanceDir(instance)+"/content.json",true)["pack"].toObject();if(!pack.isEmpty())pending.append(QJsonObject{{"id",instance["id"]},{"project",pack}});}catch(const std::exception &){}
    }
    if(pending.isEmpty())return;
    const auto base=root;QString key;try{key=curseKey();}catch(const std::exception &){}
    iconWatcher.setFuture(QtConcurrent::run([this,pending,base,key]{
        QJsonArray result;Context context;context.cancel=&iconCancelled;Network network(context);
        for(const auto &entry:pending){
            if(iconCancelled.load())break;
            const auto item=entry.toObject(),project=item["project"].toObject();
            try{
                auto address=project["icon"].toString();const auto provider=project["provider"].toString(),id=project["projectId"].toString();
                if(address.isEmpty()&&!id.isEmpty()){
                    if(provider=="Modrinth")address=network.json("https://api.modrinth.com/v2/project/"+QString::fromLatin1(QUrl::toPercentEncoding(id))).object()["icon_url"].toString();
                    else if(provider=="CurseForge"&&!key.isEmpty())address=network.json("https://api.curseforge.com/v1/mods/"+QString::fromLatin1(QUrl::toPercentEncoding(id)),{},{{"x-api-key",key.toUtf8()}}).object()["data"].toObject()["logo"].toObject()["thumbnailUrl"].toString();
                }
                const QUrl url(address);if(url.scheme()!="https"||!url.userInfo().isEmpty()||!QStringList{"cdn.modrinth.com","cdn-raw.modrinth.com","media.forgecdn.net","mediafilez.forgecdn.net"}.contains(url.host()))continue;
                const auto fingerprint=QString::fromLatin1(QCryptographicHash::hash(address.toUtf8(),QCryptographicHash::Sha256).toHex());
                const auto cache=safePath(base+"/cache/project-icons",fingerprint);const auto raw=cache+"/source-image";
                network.downloads({{address,raw,{},"sha1",-1,16*1024*1024}});const auto filename=prepareArtwork(raw,cache,false);
                result.append(QJsonObject{{"id",item["id"]},{"file",filename},{"path",safePath(cache,filename)}});
            }catch(const std::exception &){}
        }
        return result;
    }));
}
void Launcher::duplicateInstance() {
    if (!editable())
        return;
    auto i = current();
    auto base = root;
    task(
        "Duplicating " + i["name"].toString(),
        [i, base](Context c) mutable {
            auto old = safePath(base + "/instances", i["id"].toString());
            i["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
            i["name"] = i["name"].toString() + " copy";
            auto dest = safePath(base + "/instances", i["id"].toString());
            try {
                copyTree(old + "/game", dest + "/game", c);
                if (QFileInfo::exists(old + "/content.json"))
                    QFile::copy(old + "/content.json", dest + "/content.json");
                for(const auto &field:QStringList{"iconFile","backgroundFile"}) {
                    const auto name=i[field].toString();
                    if(QStringList{"icon.png","icon.gif","background.png","background.gif"}.contains(name))
                        if(!QFile::copy(safePath(old,name),safePath(dest,name)))throw Error("Could not copy instance artwork.");
                }
                writeObject(dest + "/instance.json", i);
            } catch (...) {
                QDir(dest).removeRecursively();
                throw;
            }
            return i;
        },
        [this](QJsonObject i) {
            reloadInstances();
            selectInstance(i["id"].toString());
            emit notice("Copied. Install this copy to prepare its own launch files.", false);
        });
}
void Launcher::deleteInstance() {
    if (!editable())
        return;
    try {
        auto dir = instanceDir(current());
        if (!QFile::moveToTrash(dir))
            throw Error("Windows could not move this instance to the Recycle Bin. Nothing was deleted.");
        reloadInstances();
        emit navigate("library");
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::installSelected() {
    if (!editable())
        return;
    auto i = current(), settings = config;
    auto base = root;
    task(
        "Installing " + i["name"].toString(),
        [i, settings, base](Context c) { return Minecraft(base, c).install(i, settings); },
        [this](QJsonObject) {
            reloadInstances();
            emit notice("Installation finished. Ready to play.", false);
        },
        i["id"].toString(), "install");
}
void Launcher::play(bool offline) {
    if (working || processes.contains(selection))
        return;
    auto i = current();
    if (i.isEmpty())
        return;
    if(!launchTarget.isEmpty())i["_quickPlay"]=launchTarget;
    auto account = QJsonObject::fromVariantMap(activeAccount());
    if (account.isEmpty()) {
        emit notice("Add an account or offline profile before playing.", true);
        emit navigate("accounts");
        return;
    }
    auto settings = config;
    auto base = root;
    auto dir = instanceDir(i);
    bool installed = QFileInfo::exists(dir + "/installation.json");
    if (offline && !installed) {
        emit notice("Offline play needs an installed instance. Install it while connected first.", true);
        return;
    }
    task(
        "Preparing " + i["name"].toString(),
        [=](Context c) {
            auto a = account;
            QString token;
            if (a["type"] == "microsoft" && !offline) {
                c.report("Refreshing Microsoft session…");
                auto login = Accounts(c).refresh(settings["microsoftClientId"].toString(), a);
                a = login.account;
                token = login.minecraftToken;
            }
            auto state =
                installed ? readObject(dir + "/installation.json") : Minecraft(base, c).install(i, settings);
            return QJsonObject{{"state", state}, {"account", a}, {"token", token}};
        },
        [this, i](QJsonObject r) {
            launchProcess(i, r["state"].toObject(), r["account"].toObject(), r["token"].toString());
        },
        i["id"].toString(), "launch");
}
void Launcher::playTarget(const QString &instanceId,const QString &type,const QString &value) {
    if(working){emit notice("Wait for the current task before starting Minecraft.",true);return;}
    if(processes.contains(instanceId)){emit notice("This instance is already running.",true);return;}
    try {
        if(!QStringList{"world","server"}.contains(type))throw Error("Choose a saved world or server.");
        const auto found=std::find_if(instanceList.begin(),instanceList.end(),[&](const auto &i){return i["id"]==instanceId;});
        if(found==instanceList.end())throw Error("This instance no longer exists.");
        if(type=="world") {
            if(value.contains('/')||value.contains('\\')||!QFileInfo::exists(safePath(instanceDir(*found)+"/game/saves",value)+"/level.dat"))throw Error("This saved world is no longer available.");
        }else if(parseMinecraftAddress(value).isEmpty())throw Error("This server address is not valid.");
        selectInstance(instanceId);launchTarget={{"type",type},{"value",value}};play();launchTarget={};
    }catch(const std::exception &e){launchTarget={};emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::appendLog(const QString &id, const QString &text) {
    auto cleaned = redact(text, processSecrets.value(id));
    auto &lines = logLines[id];
    for (const auto &line : cleaned.split('\n'))
        if (!line.isEmpty())
            lines << line.left(16000);
    while (lines.size() > 2500)
        lines.removeFirst();
    dirtyLogs = true;
}
QString Launcher::logText() const {
    return logLines.value(selection).join('\n');
}
void Launcher::launchProcess(QJsonObject instance, QJsonObject state, QJsonObject account, QString token) {
    auto id = instance["id"].toString();
    auto cmd = Minecraft::command(state, instance, config, account, token);
    auto p = new QProcess(this);
    p->setWorkingDirectory(cmd.workingDirectory);
    p->setProcessChannelMode(QProcess::MergedChannels);
    processSecrets[id] = {token};
    logLines[id].clear();
    partialLines[id].clear();
    auto logFile = new QFile(instanceDir(instance) + "/moly-latest.log", p);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw Error("Cannot create the game log.");
    connect(p, &QProcess::started, this, [this, id] {
        const auto now = QDateTime::currentDateTimeUtc();
        startedAt[id] = now;
        for (auto &item : instanceList)
            if (item["id"] == id) {
                item["lastPlayed"] = now.toString(Qt::ISODate);
                try {
                    writeObject(instanceDir(item) + "/instance.json", item);
                } catch (const std::exception &error) {
                    emit notice(QString::fromUtf8(error.what()), true);
                }
                break;
            }
        emit changed();
    });
    connect(p, &QProcess::readyReadStandardOutput, this, [this, id, p, logFile] {
        auto &buffer = partialLines[id];
        buffer += QString::fromUtf8(p->readAllStandardOutput());
        auto pos = buffer.lastIndexOf('\n');
        if (pos < 0 && buffer.size() > 65536)
            pos = buffer.size() - 4096;
        if (pos >= 0) {
            auto chunk = redact(buffer.left(pos + 1), processSecrets[id]);
            buffer.remove(0, pos + 1);
            appendLog(id, chunk);
            if (logFile->size() < 32 * 1024 * 1024)
                logFile->write(chunk.toUtf8());
        }
    });
    connect(p, &QProcess::errorOccurred, this, [this, id](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            appendLog(id, "[ERROR] Java could not start. Check your Java path or use Repair / Install.");
            auto proc = processes.take(id);
            if (proc)
                proc->deleteLater();
            emit changed();
            emit notice("Java could not start. Check Settings or repair the instance.", true);
            emit gameEnded();
        }
    });
    connect(p, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, id, p, logFile](int code, QProcess::ExitStatus exit) {
                auto tail = redact(partialLines.take(id), processSecrets[id]);
                appendLog(id, tail);
                logFile->write(tail.toUtf8());
                QString msg =
                    code == 0 && exit == QProcess::NormalExit
                        ? "Minecraft closed."
                        : QString("Minecraft stopped with exit code %1. Open the log for details.").arg(code);
                appendLog(id, "[Moly] " + msg);
                logFile->write(("\n[Moly] " + msg).toUtf8());
                logFile->close();
                processes.remove(id);
                processSecrets.remove(id);
                if (startedAt.contains(id)) {
                    const auto seconds =
                        qMax<qint64>(0, startedAt.take(id).secsTo(QDateTime::currentDateTimeUtc()));
                    for (auto &item : instanceList)
                        if (item["id"] == id) {
                            item["playtimeSeconds"] = item["playtimeSeconds"].toDouble() + double(seconds);
                            try {
                                writeObject(instanceDir(item) + "/instance.json", item);
                            } catch (const std::exception &error) {
                                emit notice(QString::fromUtf8(error.what()), true);
                            }
                            break;
                        }
                }
                p->deleteLater();
                emit changed();
                emit gameEnded();
                emit notice(msg, code != 0);
            });
    processes[id] = p;
    appendLog(id, "[Moly] Launching " + instance["name"].toString() + " with Java " +
                      QString::number(state["javaMajor"].toInt()) +
                      (token.isEmpty() ? " · offline profile" : " · Microsoft account"));
    p->start(cmd.program, cmd.arguments);
    selectInstance(id);
    emit changed();
    emit gameStarted();
    emit navigate("instance");
}
void Launcher::stopGame() {
    if (auto p = processes.value(selection)) {
        appendLog(selection, "[Moly] Stop requested by the user.");
        p->terminate();
        QTimer::singleShot(5000, p, [p] {
            if (p->state() != QProcess::NotRunning)
                p->kill();
        });
    }
}
void Launcher::openFolder(const QString &part) {
    try {
        auto i = current();
        QString path = i.isEmpty()      ? root
                       : part.isEmpty() ? instanceDir(i) + "/game"
                                        : safePath(instanceDir(i) + "/game", part);
        QDir().mkpath(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::openLink(const QString &url) {
    QUrl u(url);
    if (u.scheme() == "https")
        QDesktopServices::openUrl(u);
}
void Launcher::saveSettings(const QVariantMap &values) {
    if (working)
        return;
    try {
        auto c = config;
        auto v = QJsonObject::fromVariantMap(values);
        for (const auto &k :
             {"memoryMb", "width", "height", "fullscreen", "reducedMotion", "keepVisible", "javaPath",
              "jvmArgs", "microsoftClientId", "launcherFullscreen", "accentColor"})
            if (v.contains(k))
                c[k] = v[k];
        if (!QRegularExpression("^#[0-9a-fA-F]{6}$").match(c["accentColor"].toString()).hasMatch())
            throw Error("Use a six-digit accent color such as #8b5cf6.");
        if (c["memoryMb"].toInt() < 512 || c["memoryMb"].toInt() > memoryTotal() - 512)
            throw Error("Leave at least 512 MB of memory available for Windows.");
        if (c["width"].toInt() < 640 || c["height"].toInt() < 360)
            throw Error("Minimum game window size is 640 × 360.");
        config = c;
        persist();
        emit notice("Settings saved.", false);
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::setAccentColor(const QString &value) {
    const auto color = value.trimmed().toLower();
    if (!QRegularExpression("^#[0-9a-f]{6}$").match(color).hasMatch()) {
        emit notice("Use a six-digit color such as #8b5cf6.", true);
        return;
    }
    auto previous = config;
    try {
        config["accentColor"] = color;
        writeObject(root + "/settings.json", config);
        emit changed();
    } catch (const std::exception &error) {
        config = previous;
        emit notice(QString::fromUtf8(error.what()), true);
    }
}
void Launcher::setReducedMotion(bool enabled) {
    if (config["reducedMotion"].toBool() == enabled)
        return;
    const auto previous = config;
    try {
        config["reducedMotion"] = enabled;
        writeObject(root + "/settings.json", config);
        emit changed();
    } catch (const std::exception &error) {
        config = previous;
        emit changed();
        emit notice(QString::fromUtf8(error.what()), true);
    }
}
void Launcher::setUiPreferences(const QVariantMap &values) {
    const auto previous=config;
    try {
        for(const auto &key:QStringList{"animateArtwork","skinIdle","quickAccess","serverPing","showCompletedTasks","libraryGrid","showInstanceRail","showModCounts"})
            if(values.contains(key))config[key]=values[key].toBool();
        if(values.contains("pingColor")) {
            const auto color=values["pingColor"].toString().trimmed().toLower();
            if(!color.isEmpty()&&!QRegularExpression("^#[0-9a-f]{6}$").match(color).hasMatch())throw Error("Use a six-digit signal color.");
            config["pingColor"]=color;
        }
        if(values.contains("backgroundDim"))config["backgroundDim"]=qBound(0.2,values["backgroundDim"].toDouble(),0.95);
        writeObject(root+"/settings.json",config);emit changed();
    }catch(const std::exception &e){config=previous;emit changed();emit notice(QString::fromUtf8(e.what()),true);}
}
void Launcher::clearCompletedJobs() {
    for(qsizetype i=jobList.size()-1;i>=0;--i)if(jobList[i].toMap()["state"]!="Working")jobList.removeAt(i);
    emit jobsChanged();
}
void Launcher::dismissJob(const QString &id) {
    for(qsizetype i=jobList.size()-1;i>=0;--i)if(jobList[i].toMap()["id"].toString()==id&&jobList[i].toMap()["state"]!="Working")jobList.removeAt(i);
    emit jobsChanged();
}
void Launcher::exportInstance(const QString &url,const QString &format,bool includeWorlds) {
    if(!editable())return;
    const auto destination=localPath(url),base=root;
    const auto instance=current();
    task("Exporting "+instance["name"].toString(),[=](Context context){
        PackExport::write(base,instance,destination,format,includeWorlds,context);return QJsonObject{};
    },[this](QJsonObject){emit notice("Your instance export is ready.",false);},instance["id"].toString(),"export");
}
void Launcher::applySkin(const QString &path,const QString &variant) {
    if(working)return;
    const auto account=QJsonObject::fromVariantMap(activeAccount());
    if(account["type"]!="microsoft"){emit notice("Choose a signed-in Microsoft account to apply a Minecraft skin.",true);return;}
    const auto local=localPath(path);
    const QFileInfo source(local),folder(root+"/skins");
    if(folder.isSymLink()||source.isSymLink()||source.canonicalPath()!=folder.canonicalFilePath()||!QStringList{"classic","slim"}.contains(variant)){
        emit notice("Choose a skin from your Moly collection.",true);return;
    }
    const auto client=config["microsoftClientId"].toString();
    task("Applying Minecraft skin",[=](Context context){
        const auto image=SkinStore::readSkin(local);
        if(image.height()==32&&variant=="slim")throw Error("Legacy 64 × 32 skins use the Classic model.");
        QByteArray png;QBuffer pngBuffer(&png);pngBuffer.open(QIODevice::WriteOnly);
        if(!image.save(&pngBuffer,"PNG"))throw Error("Cannot prepare this skin PNG.");
        context.report("Refreshing Microsoft session…");
        const auto login=Accounts(context).refresh(client,account);
        const auto boundary="Moly"+QUuid::createUuid().toString(QUuid::Id128).toUtf8();
        QByteArray body="--"+boundary+"\r\nContent-Disposition: form-data; name=\"variant\"\r\n\r\n"+variant.toUtf8()+"\r\n--"+boundary+"\r\nContent-Disposition: form-data; name=\"file\"; filename=\"skin.png\"\r\nContent-Type: image/png\r\n\r\n"+png+"\r\n--"+boundary+"--\r\n";
        context.report("Updating your Minecraft skin…");
        Network(context).request("https://api.minecraftservices.com/minecraft/profile/skins",body,"multipart/form-data; boundary="+boundary,{{"Authorization","Bearer "+login.minecraftToken.toUtf8()}});
        return login.account;
    },[this](QJsonObject account){
        for(qsizetype i=0;i<accountList.size();++i)if(accountList[i].toObject()["id"]==account["id"]){accountList[i]=account;break;}
        persist();emit notice("Minecraft account skin updated. Relaunch Minecraft to see the change.",false);
    },{},"skin");
}
void Launcher::saveCurseKey(const QString &key) {
    try {
        if (key.trimmed().isEmpty())
            Secrets::remove("curseforge");
        else
            Secrets::put("curseforge", key.trimmed().toUtf8());
        emit changed();
        emit notice("Connection settings saved securely.", false);
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::addOfflineAccount(const QString &name) {
    if (!QRegularExpression("^[A-Za-z0-9_]{3,16}$").match(name).hasMatch()) {
        emit notice("Use 3–16 letters, numbers, or underscores for your nickname.", true);
        return;
    }
    try {
        auto uuid = offlineUuid(name);
        for (auto v : accountList)
            if (v.toObject()["id"] == uuid) {
                selectAccount(uuid);
                return;
            }
        accountList.append(QJsonObject{{"id", uuid}, {"uuid", uuid}, {"name", name}, {"type", "offline"}});
        config["activeAccount"] = uuid;
        persist();
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::signIn() {
    auto client = config["microsoftClientId"].toString();
    task(
        "Sign in with Microsoft",
        [this, client](Context c) {
            auto result = Accounts(c).signIn(client, [this](QString code, QString url) {
                QMetaObject::invokeMethod(
                    this,
                    [this, code, url] {
                        deviceCode = code;
                        emit loginChanged();
                        openLink(url);
                    },
                    Qt::QueuedConnection);
            });
            return result.account;
        },
        [this](QJsonObject a) {
            for (qsizetype i = accountList.size() - 1; i >= 0; --i)
                if (accountList[i].toObject()["id"] == a["id"])
                    accountList.removeAt(i);
            accountList.append(a);
            config["activeAccount"] = a["id"];
            persist();
            emit notice("Signed in as " + a["name"].toString(), false);
        });
}
void Launcher::selectAccount(const QString &id) {
    try {
        config["activeAccount"] = id;
        persist();
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::removeAccount(const QString &id) {
    if (working)
        return;
    try {
        for (qsizetype i = accountList.size() - 1; i >= 0; --i)
            if (accountList[i].toObject()["id"] == id)
                accountList.removeAt(i);
        Secrets::remove("account/" + id);
        if (config["activeAccount"] == id)
            config["activeAccount"] =
                accountList.isEmpty() ? QString() : accountList[0].toObject()["id"].toString();
        persist();
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::search(const QString &provider, const QString &type, const QString &query,
                      const QString &category) {
    const auto instance = current();
    if ((searchBusy || searchMore) && searchProvider == provider && searchType == type &&
        searchQuery == query && searchCategory == category && searchInstance == instance)
        return;
    ++searchGeneration;
    searchProvider = provider;
    searchType = type;
    searchQuery = query;
    searchCategory = category;
    searchInstance = instance;
    searchOffset = 0;
    searchTotal = 0;
    searchHasMore = false;
    searchErrorText.clear();
    searchResults = {};
    searchBusy = true;
    searchMore = false;
    pendingSearch = true;
    emit resultsChanged();
    emit searchStateChanged();
    if (working) {
        if (activeTaskOperation == "search" || activeTaskOperation == "search-more")
            cancelled = true;
        return;
    }
    startSearch(false);
}
void Launcher::loadMore() {
    loadMoreResults();
}
void Launcher::loadMoreResults() {
    if (working || searchBusy || searchMore || pendingSearch || !searchHasMore || searchProvider.isEmpty())
        return;
    startSearch(true);
}
void Launcher::startSearch(bool append) {
    if (working)
        return;
    if (searchProvider.isEmpty())
        return;
    pendingSearch = false;
    lastSearchAppend = append;
    searchBusy = !append;
    searchMore = append;
    searchErrorText.clear();
    emit searchStateChanged();
    const auto ticket = searchGeneration;
    const auto base = root, key = curseKey();
    const auto instance = searchInstance;
    const auto provider = searchProvider, type = searchType, query = searchQuery, category = searchCategory;
    const auto offset = append ? searchOffset : 0;
    task(
        append ? "Loading more " + provider : "Searching " + provider,
        [=](Context context) {
            return Content(base, key, context)
                .searchPage(provider, type, query, instance, category, offset, SearchPageSize);
        },
        [this, ticket, offset, append](QJsonObject page) {
            if (ticket != searchGeneration)
                return;
            const auto merged =
                mergeSearchPage(append ? searchResults : QJsonArray{}, page, offset, SearchPageSize);
            searchResults = merged["items"].toArray();
            searchOffset = merged["nextOffset"].toInt();
            searchTotal = merged["total"].toInt();
            searchHasMore = merged["hasMore"].toBool();
            searchBusy = false;
            searchMore = false;
            searchErrorText.clear();
            emit resultsChanged();
            emit searchStateChanged();
        },
        {}, append ? "search-more" : "search");
}
void Launcher::enrichReview(QJsonObject plan, const QString &requestedTarget) {
    const auto target = requestedTarget.isEmpty() ? selection : requestedTarget;
    QJsonObject instance;
    for (const auto &candidate : instanceList)
        if (candidate["id"] == target) {
            instance = candidate;
            break;
        }
    if (!plan.contains("instance") && instance.isEmpty())
        throw Error("The target instance no longer exists. Choose another instance.");
    auto files = plan["files"].toArray();
    auto known = instance.isEmpty()
                     ? QJsonArray()
                     : readObject(instanceDir(instance) + "/content.json", true)["files"].toArray();
    for (qsizetype n = 0; n < files.size(); ++n) {
        auto file = files[n].toObject();
        if (!instance.isEmpty() && !plan.contains("instance")) {
            const auto path = contentPath(instanceDir(instance) + "/game", file["path"].toString());
            if (QFileInfo::exists(path)) {
                file["replaces"] = true;
                bool tracked = false;
                for (auto value : known) {
                    auto old = value.toObject();
                    if (old["path"] == file["path"]) {
                        tracked = true;
                        file["modified"] =
                            fileHash(path, old["algorithm"].toString("sha1")) != old["hash"].toString();
                    }
                }
                if (!tracked)
                    file["modified"] = true;
            }
        }
        files[n] = file;
    }
    plan["files"] = files;
    plan["targetId"] = target;
    plan["targetName"] = instance["name"];
    review = plan;
    setInstallError({});
    bool needsReview = false;
    for (auto value : files) {
        const auto file = value.toObject();
        if ((file["url"].toString().isEmpty() && file["local"].toString().isEmpty()) || file["modified"].toBool())
            needsReview = true;
    }
    if (!plan["remove"].toArray().isEmpty()) needsReview = true;
    if (automaticInstall && !needsReview) {
        // Confirm has already been pressed. Resolve then commit without a second modal.
        emit planChanged();
        QTimer::singleShot(0, this, [this] { if (automaticInstall && !working && !review.isEmpty()) applyPlan(); });
    } else {
        automaticInstall = false;
        emit installStateChanged();
        emit planChanged();
    }
}
void Launcher::loadInstallChoices(const QVariantMap &project, const QString &requestedTarget) {
    ++choicesGeneration;
    choicesCancelled = true;
    choicesPending = false;
    installVersionList = {};
    installErrorText.clear();
    choicesProject = QJsonObject::fromVariantMap(project);
    choicesInstance = {};
    const auto target = requestedTarget.isEmpty() ? installTargetId() : requestedTarget;
    for (const auto &instance : instanceList)
        if (instance["id"] == target) choicesInstance = instance;
    if (choicesProject["type"] != "modpack" && choicesInstance.isEmpty()) {
        choicesLoading = false;
        setInstallError("Choose an instance before installing content.");
        return;
    }
    choicesLoading = true;
    emit installStateChanged();
    if (choicesWatcher.isRunning()) choicesPending = true;
    else startInstallChoices();
}
void Launcher::startInstallChoices() {
    choicesPending = false;
    choicesCancelled = false;
    const auto generation = choicesGeneration;
    const auto project = choicesProject, instance = choicesInstance;
    const auto cacheKey = project["provider"].toString()+"/"+project["projectId"].toString()+"/"+
                          project["type"].toString()+"/"+instance["version"].toString()+"/"+instance["loader"].toString();
    if (choicesCache.contains(cacheKey)) {
        installVersionList = choicesCache.value(cacheKey);
        choicesLoading = false;
        emit installStateChanged();
        return;
    }
    const auto base = root, key = curseKey();
    choicesWatcher.disconnect(this);
    connect(&choicesWatcher, &QFutureWatcher<QJsonObject>::finished, this, [this,generation,cacheKey] {
        if (generation != choicesGeneration) {
            if (choicesPending) startInstallChoices();
            return;
        }
        const auto result = choicesWatcher.result();
        choicesLoading = false;
        installVersionList = result["versions"].toArray();
        if (!result.contains("error")) choicesCache[cacheKey] = installVersionList;
        installErrorText = result["error"].toString();
        if (installVersionList.isEmpty() && installErrorText.isEmpty())
            installErrorText = "No compatible release was found for this setup.";
        emit installStateChanged();
    });
    choicesWatcher.setFuture(QtConcurrent::run([this,base,key,project,instance] {
        try {
            Context context{&choicesCancelled};
            // Version metadata only: no dependency resolution, archives, staging, or jobs.
            return QJsonObject{{"versions", Content(base,key,context).versions(project,instance)}};
        } catch (const std::exception &e) {
            return QJsonObject{{"error", redact(QString::fromUtf8(e.what()))}};
        } catch (...) { return QJsonObject{{"error", "Could not load available versions. Try again."}}; }
    }));
}
void Launcher::beginInstall(const QVariantMap &project, const QString &requestedTarget,
                            const QString &versionId, bool automatic) {
    if (working) {
        setInstallError("Another task is running. Wait for it to finish or cancel it from the status bar.");
        return;
    }
    automaticInstall = automatic;
    review = {};
    installVersionList = {};
    emit planChanged();
    setInstallError({});
    auto data = QJsonObject::fromVariantMap(project);
    if (!versionId.isEmpty())
        data["versionId"] = versionId;
    if (data["type"] == "datapack") {
        setInstallError("Data packs belong to a specific world. Open that world's datapacks folder from "
                        "Files and copy the pack there.");
        return;
    }
    try {
        if (data["type"] != "modpack") {
            const auto target = requestedTarget.isEmpty() ? installTargetId() : requestedTarget;
            bool found = false;
            for (const auto &instance : instanceList)
                if (instance["id"] == target) {
                    found = true;
                    break;
                }
            if (!found)
                throw Error("Choose an instance before installing this project.");
            if (processes.contains(target))
                throw Error("Stop this instance before installing content.");
            selectInstance(target);
            config["lastInstallInstance"] = target;
            writeObject(root + "/settings.json", config);
        }
        planInstall(data.toVariantMap());
    } catch (const std::exception &error) {
        setInstallError(QString::fromUtf8(error.what()));
    }
}
void Launcher::planInstall(const QVariantMap &project) {
    if (working)
        return;
    setInstallError({});
    auto data = QJsonObject::fromVariantMap(project), instance = current();
    if (data["type"] != "modpack" && !editable())
        return;
    const auto target = instance["id"].toString();
    auto base = root, key = curseKey();
    task(
        "Resolving files and dependencies",
        [=](Context context) {
            Content content(base, key, context);
            auto available = content.versions(data, instance);
            if (available.isEmpty())
                throw Error("No release supports this instance's Minecraft version and loader. Choose "
                            "another instance or version.");
            auto project = data;
            if (project["versionId"].toString().isEmpty())
                project["versionId"] = available[0].toObject()["id"];
            bool compatible = false;
            for (auto value : available)
                if (value.toObject()["id"] == project["versionId"])
                    compatible = true;
            if (!compatible)
                throw Error(
                    "The selected version is not compatible with this instance. Choose a supported version.");
            auto plan = content.plan(project, instance);
            plan["availableVersions"] = available;
            plan["selectedVersionId"] = project["versionId"];
            return plan;
        },
        [this, target](QJsonObject plan) {
            installVersionList = plan["availableVersions"].toArray();
            emit installStateChanged();
            enrichReview(plan, target);
        },data["type"]=="modpack"?QString():target,automaticInstall?"resolve-auto":"resolve");
}
void Launcher::checkUpdates() {
    if (!editable())
        return;
    auto instance = current();
    const auto target = instance["id"].toString();
    auto base = root, key = curseKey();
    task(
        "Checking for updates",
        [=](Context context) { return Content(base, key, context).updates(instance); },
        [this, target](QJsonObject plan) { enrichReview(plan, target); });
}
void Launcher::dismissPlan() {
    automaticInstall = false;
    emit installStateChanged();
    review = {};
    setInstallError({});
    emit planChanged();
}
void Launcher::resolveFile(int index, const QString &url) {
    try {
        auto files = review["files"].toArray();
        if (index < 0 || index >= files.size())
            return;
        auto f = files[index].toObject();
        auto path = localPath(url);
        if (fileHash(path, f["algorithm"].toString("sha1")) != f["hash"].toString())
            throw Error("That file does not match the expected download. Select the exact version shown.");
        f["local"] = path;
        files[index] = f;
        review["files"] = files;
        emit planChanged();
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::applyPlan() {
    if (working || review.isEmpty())
        return;
    try {
        auto plan = review;
        const auto target = plan["targetId"].toString();
        if (!plan.contains("instance") && (target != selection || !editable())) {
            setInstallError("Return to the instance selected for this installation.");
            return;
        }
        auto instance = current();
        const bool newInstance = plan.contains("instance");
        if (newInstance) {
            instance = plan["instance"].toObject();
            instance["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
            instance["schemaVersion"] = 1;
            instance["group"] = "Modpacks";
            instance["icon"] = "◇";
            instance["settings"] = QJsonObject{};
        }
        auto base = root, key = curseKey();
        setInstallError({});
        task(
            plan["title"].toString(),
            [=](Context context) {
                const auto dir = safePath(base + "/instances", instance["id"].toString());
                try {
                    Content(base, key, context).apply(instance, plan);
                    if (newInstance)
                        writeObject(dir + "/instance.json", instance);
                } catch (...) {
                    if (newInstance)
                        QDir(dir).removeRecursively();
                    throw;
                }
                return instance;
            },
            [this, newInstance, plan](QJsonObject instance) {
                dismissPlan();
                reloadInstances();
                selectInstance(instance["id"].toString());
                if (newInstance)
                    emit navigate("instance");
                auto title = plan["projectTitle"].toString();
                const bool removalOnly=plan["files"].toArray().isEmpty()&&!plan["remove"].toArray().isEmpty();
                const auto message = removalOnly
                    ? "Removed content from " + instance["name"].toString() + ". A backup was kept."
                    : title.isEmpty() ? "Content updated in " + instance["name"].toString()
                                      : "Installed " + title + " to " + instance["name"].toString();
                emit notice(message, false);
                emit installCompleted(message);
                if(newInstance)refreshPackIcons();
                if(newInstance){const auto id=instance["id"].toString();QTimer::singleShot(0,this,[this,id]{if(!working&&selection==id)installSelected();});}
            },instance["id"].toString(),"content");
    } catch (const std::exception &error) {
        setInstallError(QString::fromUtf8(error.what()));
    }
}
void Launcher::importPack(const QString &url,bool prepareGame) {
    const auto base=root,key=curseKey(),path=localPath(url);
    task("Reading modpack",[=](Context context){
        bool portable=QFileInfo(path).suffix().compare("mlypack",Qt::CaseInsensitive)==0;
        if(!portable&&QFileInfo(path).suffix().compare("zip",Qt::CaseInsensitive)==0){
            try{portable=!zipEntry(path,"moly-pack.json").isEmpty();}catch(const std::exception &){}
        }
        if(portable)return QJsonObject{{"portableInstance",PackExport::importMoly(base,path,context)}};
        return Content(base,key,context).pack(path);
    },[this,prepareGame](QJsonObject result){
        if(result.contains("portableInstance")){
            reloadInstances();selectInstance(result["portableInstance"].toObject()["id"].toString());emit navigate("instance");
            emit notice(prepareGame?"Imported your setup. Preparing Minecraft and Java…":"Imported your setup. Install game files to prepare Minecraft and Java.",false);
            if(prepareGame){const auto id=selection;QTimer::singleShot(0,this,[this,id]{if(!working&&selection==id)installSelected();});}
        }else enrichReview(result);
    },{},"import");
}
void Launcher::importPrism(const QString &url) {
    auto base = root, from = localPath(url);
    task(
        "Copying Prism instance",
        [=](Context c) {
            auto pack = readObject(from + "/mmc-pack.json");
            QJsonObject i{{"schemaVersion", 1},  {"id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
                          {"loader", "Vanilla"}, {"group", "Imported"},
                          {"icon", "◇"},         {"settings", QJsonObject{}}};
            QSettings cfg(from + "/instance.cfg", QSettings::IniFormat);
            i["name"] = cfg.value("name", QFileInfo(from).fileName()).toString();
            for (auto v : pack["components"].toArray()) {
                auto comp = v.toObject();
                auto uid = comp["uid"].toString();
                if (uid == "net.minecraft")
                    i["version"] = comp["version"];
                else if (uid == "net.fabricmc.fabric-loader") {
                    i["loader"] = "Fabric";
                    i["loaderVersion"] = comp["version"];
                } else if (uid == "org.quiltmc.quilt-loader") {
                    i["loader"] = "Quilt";
                    i["loaderVersion"] = comp["version"];
                } else if (uid == "net.minecraftforge") {
                    i["loader"] = "Forge";
                    i["loaderVersion"] = comp["version"];
                } else if (uid == "net.neoforged") {
                    i["loader"] = "NeoForge";
                    i["loaderVersion"] = comp["version"];
                } else if (!QStringList{"org.lwjgl", "org.lwjgl3", "net.fabricmc.intermediary"}.contains(uid))
                    throw Error("Unsupported Prism component: " + uid +
                                ". The original instance was not changed.");
            }
            if (i["version"].toString().isEmpty() || QDir(from + "/patches").exists())
                throw Error("This Prism instance has custom components or missing metadata. It cannot be "
                            "safely imported.");
            QString source = QDir(from + "/.minecraft").exists() ? from + "/.minecraft" : from + "/minecraft";
            if (!QDir(source).exists())
                throw Error("Select a Prism instance folder containing mmc-pack.json and .minecraft.");
            auto dest = safePath(base + "/instances", i["id"].toString());
            try {
                copyTree(source, dest + "/game", c);
                writeObject(dest + "/instance.json", i);
            } catch (...) {
                QDir(dest).removeRecursively();
                throw;
            }
            return i;
        },
        [this](QJsonObject i) {
            reloadInstances();
            selectInstance(i["id"].toString());
            emit navigate("instance");
            emit notice("Prism instance copied. Use Install to prepare its launch files.", false);
        });
}
QVariantList Launcher::contentFiles(const QString &type) const {
    try {
        if (current().isEmpty())
            return {};
        return Content(root).files(current(), type).toVariantList();
    } catch (...) {
        return {};
    }
}
void Launcher::importFile(const QString &url, const QString &type) {
    if (!editable())
        return;
    try {
        if (type == "datapack")
            throw Error("Data packs belong in a specific world folder. Open Files and choose the world\'s "
                        "datapacks folder.");
        auto path = localPath(url);
        auto filename = QFileInfo(path).fileName();
        if (type == "mod" && !filename.endsWith(".jar", Qt::CaseInsensitive))
            throw Error("Mods must be JAR files.");
        if (type != "mod" && !filename.endsWith(".zip", Qt::CaseInsensitive))
            throw Error("Import a ZIP resource or shader pack.");
        QJsonObject f{{"title", filename},
                      {"filename", filename},
                      {"path", contentFolder(type) + '/' + filename},
                      {"local", path},
                      {"hash", fileHash(path, "sha512")},
                      {"algorithm", "sha512"},
                      {"type", type},
                      {"provider", "Local file"}};
        enrichReview(
            {{"title", "Import local file — verify compatibility yourself"}, {"files", QJsonArray{f}}});
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::toggleFile(const QString &path) {
    if (!editable())
        return;
    try {
        auto i = current();
        auto dir = instanceDir(i);
        auto from = contentPath(dir + "/game", path);
        auto relative = path.endsWith(".disabled") ? path.left(path.size() - 9) : path + ".disabled";
        auto to = contentPath(dir + "/game", relative);
        if (QFileInfo::exists(to))
            throw Error("The destination file already exists.");
        if (!QFile::rename(from, to))
            throw Error("Cannot toggle this file.");
        auto manifest = readObject(dir + "/content.json", true);
        auto files = manifest["files"].toArray();
        for (qsizetype n = 0; n < files.size(); ++n) {
            auto f = files[n].toObject();
            if (f["path"] == path) {
                f["path"] = relative;
                files[n] = f;
            }
        }
        manifest["files"] = files;
        writeObject(dir + "/content.json", manifest);
        emit changed();
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::removeFile(const QString &path) {
    if (!editable())
        return;
    try {
        contentPath(instanceDir(current()) + "/game", path);
        enrichReview({{"title", "Remove " + QFileInfo(path).fileName() + " (a backup will be kept)"},
                      {"files", QJsonArray{}},
                      {"remove", QJsonArray{path}}});
    } catch (const std::exception &error) {
        emit notice(QString::fromUtf8(error.what()), true);
    }
}
void Launcher::backupWorlds() {
    if (!editable())
        return;
    auto i = current();
    auto base = root;
    task(
        "Backing up worlds",
        [=](Context c) {
            auto dir = safePath(base + "/instances", i["id"].toString());
            if (!QDir(dir + "/game/saves").exists())
                throw Error("This instance does not have any saved worlds yet.");
            auto dest = dir + "/world-backups/" + timestamp();
            try {
                copyTree(dir + "/game/saves", dest, c);
            } catch (...) {
                QDir(dest).removeRecursively();
                throw;
            }
            return QJsonObject{{"path", dest}};
        },
        [this](QJsonObject r) { emit notice("World backup saved to " + r["path"].toString(), false); });
}
QStringList Launcher::backups() const {
    if (current().isEmpty())
        return {};
    return QDir(instanceDir(current()) + "/backups")
        .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
}
void Launcher::restoreBackup(const QString &name) {
    if (!editable())
        return;
    auto i = current();
    auto base = root;
    task(
        "Restoring content backup",
        [=](Context) {
            Content(base).restore(i, name);
            return QJsonObject{};
        },
        [this](QJsonObject) {
            emit changed();
            emit notice("Content restored. Worlds were not changed.", false);
        });
}
void Launcher::saveLog(const QString &url) {
    try {
        writeBytes(localPath(url), redact(logText(), processSecrets.value(selection)).toUtf8());
        emit notice("Visible log saved.", false);
    } catch (const std::exception &e) {
        emit notice(e.what(), true);
    }
}
void Launcher::exportContentList(const QString &url) {
    try {
        const auto instance = current();
        if (instance.isEmpty())
            throw Error("Select an instance first.");
        const auto managed = readObject(instanceDir(instance) + "/content.json", true)["files"].toArray();
        QJsonArray files;
        for (auto value : managed) {
            auto source = value.toObject();
            QJsonObject file;
            for (const auto &field :
                 {"provider", "projectId", "versionId", "versionName", "path", "type", "hash", "algorithm"})
                if (source.contains(field))
                    file[field] = source[field];
            contentPath(instanceDir(instance) + "/game", file["path"].toString());
            files.append(file);
        }
        QJsonObject exported{{"format", "moly-content-list"},
                             {"formatVersion", 1},
                             {"name", instance["name"]},
                             {"minecraft", instance["version"]},
                             {"loader", instance["loader"]},
                             {"loaderVersion", instance["loaderVersion"]},
                             {"files", files}};
        writeObject(localPath(url), exported);
        emit notice("Content list exported. Worlds and account details are not included.", false);
    } catch (const std::exception &error) {
        emit notice(QString::fromUtf8(error.what()), true);
    }
}
QVariantList Launcher::worldList() const {
    QVariantList worlds;
    try {
        const auto instance = current();
        if (instance.isEmpty())
            return worlds;
        const auto saves = instanceDir(instance) + "/game/saves";
        for (const auto &entry : QDir(saves).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            if (entry.isSymLink())
                continue;
            const auto level = QFileInfo(entry.absoluteFilePath() + "/level.dat");
            worlds.append(QVariantMap{
                {"name", entry.fileName()},
                {"folder", entry.fileName()},
                {"lastModified",
                 (level.exists() ? level.lastModified() : entry.lastModified()).toString(Qt::ISODate)}});
        }
    } catch (...) {
        return {};
    }
    return worlds;
}
void Launcher::relocate(const QString &url) {
    if (working || !processes.isEmpty()) {
        emit notice("Stop running games and wait for downloads before moving storage.", true);
        return;
    }
    auto dest = QDir(localPath(url)).absoluteFilePath("MolyLauncher"), source = root;
    if (dest.startsWith(source, Qt::CaseInsensitive) || source.startsWith(dest, Qt::CaseInsensitive)) {
        emit notice("Choose a separate storage location.", true);
        return;
    }
    if (QDir(dest).exists()) {
        emit notice("The destination MolyLauncher folder already exists. Choose an empty location.", true);
        return;
    }
    task(
        "Copying launcher storage",
        [=](Context c) {
            try {
                copyTree(source, dest, c);
                for (auto d : QDir(dest + "/instances").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                    QFile::remove(dest + "/instances/" + d + "/installation.json");
            } catch (...) {
                QDir(dest).removeRecursively();
                throw;
            }
            return QJsonObject{{"path", dest}};
        },
        [this](QJsonObject r) {
            QSettings().setValue("dataRoot", r["path"].toString());
            emit notice("Storage copied. Restart Moly to use the new folder. Original files remain. Repair "
                        "each instance after restarting.",
                        false);
        });
}
