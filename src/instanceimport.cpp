#include "instanceimport.h"
#include "artwork.h"
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUrl>
#include <QUuid>

namespace moly {
namespace {
bool linked(const QFileInfo &info) { return info.isSymLink() || info.isJunction(); }
QJsonObject metadata(const QString &path) {
    QFileInfo info(path);
    if (!info.exists()) return {};
    if (linked(info) || info.size()>8*1024*1024) throw Error("This instance metadata is not a regular, reasonably sized file.");
    return readObject(path);
}
QString first(const QJsonObject &data, const QStringList &keys) {
    for (const auto &key:keys) if (!data[key].toString().isEmpty()) return data[key].toString();
    return {};
}
QString normalizeLoader(QString value) {
    value=value.toLower();
    if (value.contains("neoforge")) return "NeoForge";
    if (value.contains("fabric")) return "Fabric";
    if (value.contains("quilt")) return "Quilt";
    if (value.contains("forge")) return "Forge";
    if (value.isEmpty() || value=="vanilla" || value=="none") return "Vanilla";
    return value;
}
void finish(QJsonObject &row) {
    row["id"]=QString::fromLatin1(QCryptographicHash::hash((row["provider"].toString()+row["path"].toString()+row["sourceProfileId"].toString()).toUtf8(),QCryptographicHash::Sha256).toHex());
    if (row["name"].toString().isEmpty()) row["name"]=QFileInfo(row["path"].toString()).fileName();
    const bool known=QStringList{"Vanilla","Fabric","Forge","NeoForge","Quilt"}.contains(row["loader"].toString());
    if (!known) row["reason"]="This client uses an unsupported loader. Choose an ordinary Minecraft loader below.";
    else if (row["version"].toString().isEmpty()) row["reason"]="Choose the Minecraft and loader versions below before importing.";
    else if (row["loader"]!="Vanilla" && row["loaderVersion"].toString().isEmpty()) row["reason"]="Choose the loader version below before importing.";
    row["supported"]=known&&!row["version"].toString().isEmpty()&&(row["loader"]=="Vanilla"||!row["loaderVersion"].toString().isEmpty());
    const auto icon=row["iconPath"].toString();
    if (!icon.isEmpty() && QFileInfo(icon).isFile() && !linked(QFileInfo(icon))) row["iconUrl"]=QUrl::fromLocalFile(icon).toString();
}
QJsonArray modrinthDatabase(const QString &database,const Context &context) {
    QJsonArray out;
    if (!QFileInfo::exists(database)) return out;
    const auto connection="moly-import-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString error;
    {
        auto db=QSqlDatabase::addDatabase("QSQLITE",connection);
        db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=1000");
        db.setDatabaseName(database);
        if (!db.open()) error="Could not read Modrinth's local database. Close Modrinth or choose its data folder again.";
        else {
            QSqlQuery query(db);
            // Explicit allowlist: never query account/token/settings tables.
            const bool current=db.tables().contains("instance_content_sets");
            QString sql=current?
                "SELECT i.path,i.name,i.icon_path,c.game_version,c.loader,c.loader_version FROM instances i LEFT JOIN instance_content_sets c ON c.id=i.applied_content_set_id LIMIT 500":
                "SELECT path,name,icon_path,game_version,mod_loader,mod_loader_version FROM profiles LIMIT 500";
            if (!query.exec(sql)) error="This Modrinth database version is not supported yet. Choose a profile folder and enter its game version below.";
            else while (query.next()) {
                context.check();
                auto path=query.value(0).toString();
                if (!QDir::isAbsolutePath(path)) path=QDir(QFileInfo(database).absolutePath()).filePath("profiles/"+path);
                if (!QFileInfo(path).isDir() || linked(QFileInfo(path))) continue;
                QJsonObject row{{"provider","Modrinth"},{"path",QDir(path).canonicalPath()},{"gameDir",QDir(path).canonicalPath()},
                    {"name",query.value(1).toString()},{"iconPath",query.value(2).toString()},
                    {"version",query.value(3).toString()},{"loader",normalizeLoader(query.value(4).toString())},{"loaderVersion",query.value(5).toString()}};
                // Earlier schemas serialized loader versions as a JSON object.
                auto loader=QJsonDocument::fromJson(row["loaderVersion"].toString().toUtf8()).object();
                if(!loader.isEmpty())row["loaderVersion"]=first(loader,{"id","version"});
                finish(row);out.append(row);
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connection);
    if (!error.isEmpty()) throw Error(error);
    return out;
}
}
QStringList InstanceImport::defaultRoots(const QString &provider) {
    const auto home=QDir::homePath(),roaming=QString::fromLocal8Bit(qgetenv("APPDATA")),local=QString::fromLocal8Bit(qgetenv("LOCALAPPDATA"));
    if(provider=="Modrinth")return {roaming+"/ModrinthApp",roaming+"/com.modrinth.theseus"};
    if(provider=="Prism")return {roaming+"/PrismLauncher/instances",home+"/PrismLauncher/instances"};
    if(provider=="MultiMC")return {roaming+"/MultiMC/instances",home+"/MultiMC/instances"};
    if(provider=="CurseForge")return {home+"/curseforge/minecraft/Instances",home+"/Documents/Curse/Minecraft/Instances",home+"/Documents/CurseForge/Minecraft/Instances"};
    if(provider=="ATLauncher")return {roaming+"/ATLauncher/instances",local+"/ATLauncher/instances",home+"/ATLauncher/instances"};
    if(provider=="Lunar Client")return {home+"/.lunarclient/profiles",home+"/.lunarclient/profiles/modpacks"};
    if(provider=="Minecraft")return {roaming+"/.minecraft"};
    return {};
}
QJsonObject InstanceImport::describe(const QString &provider,const QString &folder) {
    const QFileInfo info(folder);
    if(!info.isDir()||linked(info))throw Error("Choose a regular instance folder.");
    const auto path=info.canonicalFilePath();
    QJsonObject row{{"provider",provider},{"path",path},{"gameDir",path},{"loader","Vanilla"}};
    if(provider=="Prism"||provider=="MultiMC") {
        const auto pack=metadata(path+"/mmc-pack.json");
        if(pack.isEmpty())throw Error("No Prism/MultiMC instance metadata was found here.");
        QSettings config(path+"/instance.cfg",QSettings::IniFormat);
        row["name"]=config.value("name",info.fileName()).toString();
        for(auto value:pack["components"].toArray()) {
            const auto component=value.toObject();const auto uid=component["uid"].toString();
            if(uid=="net.minecraft")row["version"]=component["version"];
            else if(uid=="net.fabricmc.fabric-loader"||uid=="org.quiltmc.quilt-loader"||uid=="net.minecraftforge"||uid=="net.neoforged") {
                row["loader"]=normalizeLoader(uid);row["loaderVersion"]=component["version"];
            }else if(!component["cachedVolatile"].toBool() && !QStringList{"org.lwjgl3","org.lwjgl","net.fabricmc.intermediary"}.contains(uid) && !uid.isEmpty())
                row["warning"]="Custom launch components are not copied. Review this setup after import.";
        }
        row["gameDir"]=QFileInfo(path+"/.minecraft").isDir()?path+"/.minecraft":path+"/minecraft";
        auto key=config.value("iconKey").toString();
        if(QRegularExpression("^[a-zA-Z0-9_. -]+$").match(key).hasMatch()) {
            const auto icon=QDir(path+"/../../icons").absoluteFilePath(key+".png");
            if(QFileInfo::exists(icon))row["iconPath"]=icon;
        }
    } else if(provider=="CurseForge") {
        const auto data=metadata(path+"/minecraftinstance.json");
        if(data.isEmpty())throw Error("No CurseForge instance metadata was found here.");
        const auto loader=data["baseModLoader"].toObject();
        row["name"]=data["name"];row["version"]=first(data,{"gameVersion","minecraftVersion"});
        if(row["version"].toString().isEmpty())row["version"]=loader["minecraftVersion"];
        auto name=first(loader,{"name","forgeVersion"});row["loader"]=normalizeLoader(name);
        name.remove(QRegularExpression("^(fabric|quilt|neo?forge|forge)-",QRegularExpression::CaseInsensitiveOption));
        name.remove(QRegularExpression("^"+QRegularExpression::escape(row["version"].toString())+"-"));
        row["loaderVersion"]=name;
        row["iconPath"]=path+"/profileImage.png";
    } else if(provider=="ATLauncher") {
        const auto data=metadata(path+"/instance.json");
        if(data.isEmpty())throw Error("No ATLauncher instance metadata was found here.");
        const auto launcher=data["launcher"].toObject(),loader=launcher["loaderVersion"].toObject();
        row["name"]=launcher["name"];row["version"]=first(data,{"inheritsFrom","id","version"});
        row["loader"]=normalizeLoader(first(loader,{"type","loader","name"}));row["loaderVersion"]=first(loader,{"version","rawVersion","id"});
        row["iconPath"]=path+"/instance.png";
    } else {
        auto data=metadata(path+"/profile.json");
        if(data.isEmpty())data=metadata(path+"/version.json");
        row["name"]=first(data,{"name","title"});
        row["version"]=first(data,{"game_version","minecraftVersion","inheritsFrom","minecraft_version"});
        row["loader"]=normalizeLoader(first(data,{"loader","mod_loader"}));
        row["loaderVersion"]=first(data,{"loader_version","loaderVersion","mod_loader_version"});
        row["iconPath"]=path+"/icon.png";
        if(QFileInfo(path+"/.minecraft").isDir())row["gameDir"]=path+"/.minecraft";
        if(provider=="Lunar Client")row["warning"]="Copies ordinary Minecraft files only. Lunar's bundled client features and cosmetics cannot run in Moly.";
        if(data.isEmpty()&&!QFileInfo(path+"/mods").isDir()&&!QFileInfo(path+"/saves").isDir()&&!QFileInfo(path+"/options.txt").isFile())throw Error("No recognizable Minecraft setup was found here.");
    }
    if(!QFileInfo(row["gameDir"].toString()).isDir()||linked(QFileInfo(row["gameDir"].toString())))throw Error("This instance's game folder is missing or is a link.");
    finish(row);return row;
}
QJsonArray InstanceImport::scan(const QString &provider,const QStringList &roots,const Context &context) {
    if(!QStringList{"Modrinth","Prism","MultiMC","CurseForge","ATLauncher","Lunar Client","Minecraft","Folder"}.contains(provider))throw Error("Choose a supported import source.");
    QJsonArray out;QSet<QString> seen;
    auto append=[&](QJsonObject row){const auto id=row["id"].toString();if(!seen.contains(id)){seen.insert(id);out.append(row);}};
    QString databaseError;
    for(const auto &root:roots) {
        context.check();if(!QFileInfo(root).isDir()||linked(QFileInfo(root)))continue;
        if(provider=="Minecraft") {
            const auto profiles=metadata(root+"/launcher_profiles.json")["profiles"].toObject();
            for(auto it=profiles.begin();it!=profiles.end();++it) {
                const auto profile=it.value().toObject();auto game=profile["gameDir"].toString(root);
                if(!QFileInfo(game).isDir()||linked(QFileInfo(game)))continue;
                auto version=profile["lastVersionId"].toString();
                QJsonObject row{{"provider",provider},{"path",QDir(game).canonicalPath()},{"gameDir",QDir(game).canonicalPath()},
                    {"name",profile["name"]},{"sourceProfileId",it.key()},{"loader","Vanilla"}};
                if(QRegularExpression("^[a-zA-Z0-9_.+ -]{1,100}$").match(version).hasMatch()&&!version.startsWith("latest-")) {
                    const auto definition=metadata(root+"/versions/"+version+"/"+version+".json");
                    row["version"]=definition["inheritsFrom"].toString(version);
                    for(auto library:definition["libraries"].toArray()) {
                        const auto name=library.toObject()["name"].toString().split(':');
                        if(name.size()!=3)continue;
                        if(name[0]=="net.fabricmc"&&name[1]=="fabric-loader"){row["loader"]="Fabric";row["loaderVersion"]=name[2];}
                        if(name[0]=="org.quiltmc"&&name[1]=="quilt-loader"){row["loader"]="Quilt";row["loaderVersion"]=name[2];}
                        if((name[0]=="net.minecraftforge"&&name[1]=="forge")||(name[0]=="net.neoforged"&&name[1]=="neoforge")){
                            row["loader"]=name[0]=="net.neoforged"?"NeoForge":"Forge";auto loader=name[2];loader.remove(QRegularExpression("^"+QRegularExpression::escape(row["version"].toString())+"-"));row["loaderVersion"]=loader;
                        }
                    }
                }
                finish(row);append(row);
            }
            if(!profiles.isEmpty())continue;
        }
        if(provider=="Modrinth") {
            auto database=root+"/app.db";
            if(!QFileInfo::exists(database) && QFileInfo(root).fileName()=="profiles")database=QFileInfo(root).absolutePath()+"/app.db";
            try{for(auto row:modrinthDatabase(database,context))append(row.toObject());}catch(const std::exception &e){databaseError=QString::fromUtf8(e.what());}
        }
        QList<QPair<QString,int>> queue{{root,0}};
        for(qsizetype pos=0;pos<queue.size()&&pos<1500&&out.size()<500;++pos) {
            context.check();const auto [path,depth]=queue[pos];
            try{append(describe(provider,path));continue;}catch(const std::exception &) {context.check();}
            if(depth>=2)continue;
            for(const auto &child:QDir(path).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden,QDir::Name))
                if(!linked(child)&&!QStringList{"mods","saves","assets","libraries","cache","caches","logs","config","store","resourcepacks","shaderpacks"}.contains(child.fileName()))queue.append({child.absoluteFilePath(),depth+1});
        }
    }
    if(out.isEmpty()&&!databaseError.isEmpty())throw Error(databaseError);
    return out;
}
QJsonObject InstanceImport::copy(const QString &storage,const QJsonObject &candidate,const QString &id,bool worlds,const Context &context) {
    context.check();
    auto version=candidate["version"].toString(),loader=candidate["loader"].toString(),loaderVersion=candidate["loaderVersion"].toString();
    if(!QRegularExpression("^[a-zA-Z0-9_.+ -]{1,100}$").match(version).hasMatch()||!QStringList{"Vanilla","Fabric","Forge","NeoForge","Quilt"}.contains(loader)|| (loader!="Vanilla"&&!QRegularExpression("^[a-zA-Z0-9_.+ -]{1,100}$").match(loaderVersion).hasMatch()))throw Error("Choose the exact Minecraft and loader versions before importing.");
    const auto source=QFileInfo(candidate["gameDir"].toString()).canonicalFilePath();
    if(source.isEmpty()||linked(QFileInfo(candidate["gameDir"].toString()))||QDir(source).isRoot())throw Error("Choose a regular Minecraft game folder.");
    const auto destination=safePath(storage+"/instances",id);
    if(QFileInfo::exists(destination))throw Error("This destination already exists.");
    const auto base=QDir(storage).canonicalPath();
    if(base.toCaseFolded().startsWith(source.toCaseFolded()+"/")||source.toCaseFolded().startsWith(base.toCaseFolded()+"/")||source.compare(base,Qt::CaseInsensitive)==0)throw Error("Import from a folder outside Moly's own storage.");
    // Copy game data through an allowlist. Never copy accounts, executables, launcher
    // metadata, auth caches, shared downloads, or arbitrary external launch commands.
    const QStringList directories{"mods","config","defaultconfigs","resourcepacks","shaderpacks","datapacks","screenshots","kubejs","scripts","patchouli_books","journeymap","xaero","XaeroWaypoints","XaeroWorldMap","global_packs"};
    const QStringList ordinaryFiles{"options.txt","optionsof.txt","optionsshaders.txt","servers.dat","hotbar.nbt","icon.png"};
    QList<QFileInfo> files;
    std::function<void(QString,int)> collect=[&](QString path,int depth) {
        context.check();if(depth>64)throw Error("This setup contains folders nested too deeply.");
        for(const auto &file:QDir(path).entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden)) {
            if(linked(file))throw Error("This setup contains linked files. Remove links from a copy before importing.");
            if(file.isDir())collect(file.absoluteFilePath(),depth+1);
            else { if(files.size()>200000||file.size()>8ll*1024*1024*1024)throw Error("This setup is too large to import safely.");files.append(file); }
        }
    };
    for(const auto &dir:directories)if(QFileInfo(source+"/"+dir).isDir()){if(linked(QFileInfo(source+"/"+dir)))throw Error("Linked game folders cannot be imported.");collect(source+"/"+dir,0);}
    if(worlds&&QFileInfo(source+"/saves").isDir()){if(linked(QFileInfo(source+"/saves")))throw Error("Linked saves cannot be imported.");collect(source+"/saves",0);}
    for(const auto &name:ordinaryFiles)if(QFileInfo(source+"/"+name).isFile()){if(linked(QFileInfo(source+"/"+name)))throw Error("Linked game files cannot be imported.");files.append(QFileInfo(source+"/"+name));}
    qint64 total=0,done=0;for(const auto &file:files)total+=file.size();
    QElapsedTimer progressTimer;progressTimer.start();
    QJsonObject instance{{"schemaVersion",1},{"id",id},{"name",candidate["name"]},{"version",version},{"loader",loader},{"loaderVersion",loaderVersion},{"group","Imported"},{"settings",QJsonObject{}}};
    try {
        QDir().mkpath(destination+"/game");
        for(const auto &file:files) {
            context.check();
            if(linked(QFileInfo(file.absoluteFilePath()))||!QFileInfo(file.absoluteFilePath()).canonicalFilePath().startsWith(source+"/",Qt::CaseInsensitive))throw Error("A source file changed during import. Close the other launcher and try again.");
            const auto relative=QDir(source).relativeFilePath(file.absoluteFilePath());
            const auto target=safePath(destination+"/game",relative);QDir().mkpath(QFileInfo(target).absolutePath());
            QFile input(file.absoluteFilePath());QSaveFile output(target);
            if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly))throw Error("Cannot copy "+relative+". Close Minecraft and check free disk space.");
            while(!input.atEnd()) {
                context.check();const auto chunk=input.read(1024*1024);
                if(chunk.isEmpty()&&input.error()!=QFile::NoError)throw Error("Could not read "+relative);
                if(output.write(chunk)!=chunk.size())throw Error("Could not write "+relative+". Check free disk space.");
                done+=chunk.size();if(progressTimer.elapsed()>=60||done>=total){context.report("Copying "+relative,done,total);progressTimer.restart();}
            }
            const QFileInfo after(file.absoluteFilePath());
            if(after.size()!=file.size()||after.lastModified()!=file.lastModified())throw Error("A source file changed during import. Close Minecraft and try again.");
            if(!output.commit())throw Error("Could not finish copying "+relative);
        }
        auto icon=candidate["iconPath"].toString();
        if(QFileInfo(icon).isFile())try{instance["iconFile"]=prepareArtwork(QUrl::fromLocalFile(icon).toString(),destination,false);instance["iconRevision"]=id;}catch(const std::exception &){}
        context.check();writeObject(destination+"/instance.json",instance);
    }catch(...){QDir(destination).removeRecursively();throw;}
    return instance;
}
}
