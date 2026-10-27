#include "skinstore.h"
#include "core.h"
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <QtEndian>
#include <cstring>

using namespace moly;
namespace {
bool validId(const QString &id) {
    return id == "moly-default" || QStringList{"default-steve","default-alex","default-ari","default-efe","default-kai","default-makena","default-noor","default-sunny","default-zuri"}.contains(id) || QRegularExpression("^[0-9a-f]{8}-(?:[0-9a-f]{4}-){3}[0-9a-f]{12}$").match(id).hasMatch();
}
bool validModel(const QString &model) { return model == "classic" || model == "slim"; }
}

static void initializeDefaultSkins() {
    static const bool initialized=[] { Q_INIT_RESOURCE(default_skins);return true; }();
    Q_UNUSED(initialized)
}
static QStringList defaultNames(){return {"Steve","Alex","Ari","Efe","Kai","Makena","Noor","Sunny","Zuri"};}
static bool builtIn(const QString &id){return id.startsWith("default-")&&validId(id);}
QString SkinStore::modelFor(const QString &id) const {
    if(builtIn(id))return builtinModels.value(id).toString(QStringList{"default-alex","default-efe","default-makena","default-noor"}.contains(id)?"slim":"classic");
    for(const auto &value:entries)if(value.toObject()["id"]==id)return value.toObject()["model"].toString();
    return "classic";
}
SkinStore::SkinStore(const QString &dataRoot, QObject *parent) : QObject(parent), root(dataRoot + "/skins") {
    try {
        initializeDefaultSkins();QDir().mkpath(root);
        for(const auto &name:defaultNames())for(const auto &variant:QStringList{"classic","slim"}) {
            const auto id="default-"+name.toLower();
            const auto path=safePath(root,id+"-"+variant+".png");
            if(QFileInfo::exists(path))continue;
            const auto image=readSkin(":/moly/skins/"+QString(variant=="classic"?"wide":"slim")+"/"+name.toLower()+".png");
            QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);
            if(!image.save(&buffer,"PNG"))throw Error("Could not prepare a default skin.");
            writeBytes(path,png);
        }
        if (QFileInfo(root + "/library.json").size() > 2 * 1024 * 1024) throw Error("This skin library index is too large to read safely.");
        auto library = readObject(root + "/library.json", true);
        if (!library.isEmpty() && library["schemaVersion"].toInt() != 1) throw Error("This skin library was saved by an unsupported version of Moly.");
        const auto models=library["defaultModels"].toObject();
        for(auto it=models.begin();it!=models.end();++it)if(builtIn(it.key())&&validModel(it.value().toString()))builtinModels[it.key()]=it.value();
        for (const auto &value : library["skins"].toArray()) {
            const auto entry=value.toObject();const auto id=entry["id"].toString();
            if(!validId(id)||id=="moly-default"||builtIn(id)||!validModel(entry["model"].toString()))continue;
            if(QFileInfo::exists(imagePath(id))&&!QFileInfo(imagePath(id)).isSymLink())entries.append(entry);
        }
        const auto candidate=library["selected"].toString();
        if(builtIn(candidate))selection=candidate;
        else for(const auto &value:entries)if(value.toObject()["id"]==candidate)selection=candidate;
    }catch(const std::exception &e){fail(QString::fromUtf8(e.what()));}
}
QString SkinStore::imagePath(const QString &id) const {
    if(!validId(id))throw Error("Invalid saved skin identifier.");
    return safePath(root,id+(builtIn(id)?"-"+modelFor(id):QString())+".png");
}
QVariantList SkinStore::skins() const {
    QVariantList result;
    for(const auto &name:defaultNames()){
        const auto id="default-"+name.toLower();
        result<<QVariantMap{{"id",id},{"name",name},{"model",modelFor(id)},{"builtin",true},{"url",QUrl::fromLocalFile(imagePath(id)).toString()}};
    }
    for(auto value:entries){auto item=value.toObject().toVariantMap();item["builtin"]=false;item["url"]=QUrl::fromLocalFile(imagePath(item["id"].toString())).toString();result<<item;}
    return result;
}
QString SkinStore::selectedSkinUrl() const {return QUrl::fromLocalFile(imagePath(selection)).toString();}
QString SkinStore::selectedModel() const {return modelFor(selection);}
void SkinStore::fail(const QString &message) { errorText = message; emit errorChanged(); }
void SkinStore::save() { writeObject(root + "/library.json", {{"schemaVersion", 1}, {"selected", selection}, {"skins", entries},{"defaultModels",builtinModels}}); }
QImage SkinStore::readSkin(const QString &path) {
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink() || info.size() > 1024 * 1024)
        throw Error("Choose a local PNG skin no larger than 1 MB.");
    QFile source(path);
    if (!source.open(QIODevice::ReadOnly)) throw Error("This PNG skin could not be read.");
    const auto bytes=source.read(1024*1024+1);
    if (bytes.size()>1024*1024 || !bytes.startsWith(QByteArray::fromHex("89504e470d0a1a0a")))
        throw Error("Choose a local PNG skin no larger than 1 MB.");
    // Skin textures only need the image/palette/transparency chunks. Drop
    // compressed comments and profiles before libpng can inflate their data.
    QByteArray pixelsOnly=bytes.left(8);
    bool header=false, imageData=false, end=false;
    for (qsizetype offset=8; offset+12<=bytes.size();) {
        const auto length=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()+offset));
        if (length>quint32(bytes.size()-offset-12)) throw Error("This PNG skin is incomplete.");
        const auto kind=bytes.mid(offset+4,4);
        if (!header && kind!="IHDR") throw Error("This PNG skin has no valid image header.");
        if (kind=="IHDR") {
            if (header || length!=13) throw Error("This PNG skin has an invalid image header.");
            header=true;
        }
        if (kind=="IDAT") imageData=true;
        if (kind=="IHDR"||kind=="PLTE"||kind=="tRNS"||kind=="IDAT"||kind=="IEND")
            pixelsOnly.append(bytes.constData()+offset,qsizetype(length)+12);
        else if ((uchar(kind[0])&0x20)==0) throw Error("This PNG skin has an unsupported image chunk.");
        offset+=qsizetype(length)+12;
        if (kind=="IEND") { end=true; break; }
    }
    if (!header || !imageData || !end) throw Error("This PNG skin is incomplete.");
    QBuffer input(&pixelsOnly);input.open(QIODevice::ReadOnly);
    QImageReader reader(&input, "PNG");
    const auto dimensions = reader.size();
    if (dimensions != QSize(64, 64) && dimensions != QSize(64, 32))
        throw Error("Minecraft skins must be 64 × 64 or legacy 64 × 32 PNG images.");
    auto image = reader.read();
    if (image.isNull()) throw Error("This PNG skin could not be read.");
    image = image.convertToFormat(QImage::Format_ARGB32);
    // Copy pixels into a fresh image so text/EXIF/ICC metadata never survives
    // importing a skin or preparing it for an account upload.
    QImage clean(image.size(), QImage::Format_ARGB32);
    for (int y=0; y<image.height(); ++y)
        std::memcpy(clean.scanLine(y), image.constScanLine(y), size_t(image.width()) * 4);
    return clean;
}
bool SkinStore::importSkin(const QString &url, const QString &model) {
    QString destination;
    const auto previousEntries = entries;
    const auto previousSelection = selection;
    try {
        if (!validModel(model)) throw Error("Choose a Classic or Slim player model.");
        const QUrl source(url);
        if (!source.isLocalFile() && !source.scheme().isEmpty() && !QDir::isAbsolutePath(url)) throw Error("Choose a PNG from this computer.");
        const auto path = source.isLocalFile() ? source.toLocalFile() : url;
        auto image = readSkin(path);
        if (image.height() == 32 && model == "slim") throw Error("Legacy 64 × 32 skins use the Classic model.");
        const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        destination = imagePath(id);
        QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG")) throw Error("The skin could not be saved.");
        writeBytes(destination, png);
        auto name = QFileInfo(path).completeBaseName().left(48).trimmed();
        entries.append(QJsonObject{{"id", id}, {"name", name.isEmpty() ? "Imported skin" : name}, {"model", model}});
        selection = id; save(); fail({}); emit changed(); return true;
    } catch (const std::exception &e) {
        entries = previousEntries; selection = previousSelection;
        if (!destination.isEmpty()) QFile::remove(destination);
        fail(QString::fromUtf8(e.what())); return false;
    }
}
void SkinStore::selectSkin(const QString &id) {
    bool found = builtIn(id);
    for (const auto &value : entries) found = found || value.toObject()["id"] == id;
    if (!found) { fail("That saved skin is no longer available."); return; }
    const auto previous = selection;
    try { selection = id; save(); fail({}); emit changed(); }
    catch (const std::exception &e) { selection = previous; fail(QString::fromUtf8(e.what())); }
}
void SkinStore::setSelectedModel(const QString &model) {
    if (!validModel(model)) { fail("Choose a Classic or Slim player model."); return; }
    if(builtIn(selection)){const auto previous=builtinModels;try{builtinModels[selection]=model;save();fail({});emit changed();}catch(const std::exception &e){builtinModels=previous;fail(QString::fromUtf8(e.what()));}return;}
    const auto previous = entries;
    try {
        if (readSkin(imagePath(selection)).height() == 32 && model == "slim") throw Error("Legacy skins use the Classic model.");
        for (qsizetype i = 0; i < entries.size(); ++i) {
            auto entry = entries[i].toObject();
            if (entry["id"] == selection) { entry["model"] = model; entries[i] = entry; break; }
        }
        save(); fail({}); emit changed();
    } catch (const std::exception &e) { entries = previous; fail(QString::fromUtf8(e.what())); }
}
void SkinStore::requestApply() {
    try { readSkin(imagePath(selection)); fail({}); emit applyRequested(imagePath(selection), selectedModel()); }
    catch (const std::exception &e) { fail(QString::fromUtf8(e.what())); }
}
QImage SkinStore::defaultTexture() {
    initializeDefaultSkins();
    return readSkin(":/moly/skins/wide/steve.png");
}
