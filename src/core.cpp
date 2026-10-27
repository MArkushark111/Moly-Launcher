#include "core.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QUuid>
#include <QDateTime>
#include <QElapsedTimer>
#include <QSysInfo>
#include <array>
#include <private/qzipreader_p.h>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace moly {
void Context::check() const { if (cancel && cancel->load()) throw Error("Cancelled. Your existing files are unchanged."); }
void Context::report(const QString &s, qint64 d, qint64 t) const { check(); if (progress) progress(s,d,t); }
QJsonObject readObject(const QString &path, bool optional) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) { if(optional && !f.exists()) return {}; throw Error("Cannot read " + path); }
    QJsonParseError e; auto d=QJsonDocument::fromJson(f.readAll(), &e);
    if(e.error != QJsonParseError::NoError || !d.isObject()) throw Error("Invalid JSON in " + path);
    return d.object();
}
void writeBytes(const QString &p, const QByteArray &b) {
    if(!QDir().mkpath(QFileInfo(p).absolutePath())) throw Error("Cannot create folder for " + p);
    QSaveFile f(p); if(!f.open(QIODevice::WriteOnly) || f.write(b)!=b.size() || !f.commit()) throw Error("Cannot save " + p);
}
void writeObject(const QString &p, const QJsonObject &o) { writeBytes(p,QJsonDocument(o).toJson()); }
static QString checkedRelativePath(const QString &relative) {
    QString r=relative; r.replace('\\','/');
    if(r.isEmpty() || r.startsWith('/') || r.contains(':') || r.contains(QChar(0))) throw Error("Unsafe file path: " + relative);
    const auto parts=r.split('/');
    static QRegularExpression reserved("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)", QRegularExpression::CaseInsensitiveOption);
    for(const auto &part:parts) if(part.isEmpty() || part==".." || part=="." || part.endsWith('.') || part.endsWith(' ') || reserved.match(part).hasMatch()) throw Error("Unsafe file path: " + relative);
    return r;
}
QString safePath(const QString &root, const QString &relative) {
    const auto r=checkedRelativePath(relative);const auto parts=r.split('/');
    QString base=QDir(root).absolutePath(), path=QDir::cleanPath(base+'/'+r);
    if(!path.startsWith(base+'/', Qt::CaseInsensitive)) throw Error("Path escapes its folder.");
    QString cursor=base;
    if(QFileInfo(cursor).isSymLink()) throw Error("Linked destination folders are not supported.");
    for(const auto &part:parts) { cursor+='/'+part; if(QFileInfo(cursor).isSymLink()) throw Error("Refusing to follow a file link: " + cursor); }
    return path;
}
QString offlineUuid(const QString &name) {
    auto bytes=QCryptographicHash::hash("OfflinePlayer:"+name.toUtf8(), QCryptographicHash::Md5);
    bytes[6]=char((uchar(bytes[6])&0x0f)|0x30); bytes[8]=char((uchar(bytes[8])&0x3f)|0x80);
    return QUuid::fromRfc4122(bytes).toString(QUuid::WithoutBraces).remove('-');
}
QString redact(QString text, const QStringList &secrets) {
    for(const auto &s:secrets) if(s.size()>3) text.replace(s,"[hidden]");
    text.replace(QRegularExpression("(?i)(Bearer\\s+)[^\\s\\\"]+"),"\\1[hidden]");
    text.replace(QRegularExpression("(?i)((?:access[_-]?token|refresh[_-]?token|client_secret|identityToken|authorization|--accessToken)[\\\"\\s:=]+)[^\\s\\\",}]+"),"\\1[hidden]");
    text.replace(QRegularExpression("eyJ[A-Za-z0-9_-]{10,}\\.[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+"),"[hidden]");
    return text;
}
QString fileHash(const QString &p,const QString &algorithm) {
    QFile f(p); if(!f.open(QIODevice::ReadOnly)) return {};
    auto a=algorithm=="sha512"?QCryptographicHash::Sha512:algorithm=="sha256"?QCryptographicHash::Sha256:algorithm=="md5"?QCryptographicHash::Md5:QCryptographicHash::Sha1;
    QCryptographicHash h(a); h.addData(&f); return QString::fromLatin1(h.result().toHex());
}
static uint crc32(const QByteArray &data) {
    static const auto table=[] {std::array<uint,256> values{};for(uint i=0;i<256;++i){uint v=i;for(int n=0;n<8;++n)v=(v>>1)^((v&1)?0xedb88320u:0u);values[i]=v;}return values;}();
    uint crc=0xffffffffu;for(uchar b:data)crc=table[(crc^b)&0xff]^(crc>>8);return crc^0xffffffffu;
}
static void validateZip(QZipReader &z) {
    if(!z.isReadable() || z.status()!=QZipReader::NoError) throw Error("This archive cannot be read.");
    qint64 total=0; QSet<QString> seen;
    const auto entries=z.fileInfoList(); if(entries.size()>100000) throw Error("Archive has too many entries.");
    for(const auto &f:entries) {
        QString p=f.filePath; if(p.endsWith('/')) p.chop(1);
        p=checkedRelativePath(p);
        if(f.isSymLink || (!f.isDir && !f.isFile)) throw Error("Archive contains a file link or special entry.");
        if(f.size<0 || f.size>512ll*1024*1024 || (total+=f.size)>8ll*1024*1024*1024) throw Error("Archive exceeds safe extraction limits.");
        p=p.toLower(); if(seen.contains(p)) throw Error("Archive contains duplicate file paths."); seen.insert(p);
    }
}
QByteArray zipEntry(const QString &zip,const QString &entry) {
    QZipReader z(zip);validateZip(z);
    for(const auto &info:z.fileInfoList())if(info.filePath==entry){
        if(info.size>32ll*1024*1024)throw Error("Archive metadata entry is too large: "+entry);
        const auto bytes=z.fileData(entry);
        if(bytes.size()!=info.size||crc32(bytes)!=info.crc||z.status()!=QZipReader::NoError)throw Error("Corrupt archive entry: "+entry);
        return bytes;
    }
    throw Error("Archive is missing "+entry);
}
void extractZip(const QString &zip,const QString &dest,const QString &prefix,bool natives,const Context &ctx,bool atomicWrites) {
    QZipReader z(zip); validateZip(z);
    const auto entries=z.fileInfoList();
    QSet<QString> parentFolders;
    qint64 done=0,total=0;
    for(const auto &f:entries) {
        auto path=f.filePath.toLower();
        for(auto slash=path.lastIndexOf('/');slash>0;slash=path.lastIndexOf('/',slash-1))
            parentFolders.insert(path.left(slash));
    }
    for(const auto &f:entries)if(f.isFile&&!(f.size==0&&parentFolders.contains(f.filePath.toLower()))&&f.filePath.startsWith(prefix)&&f.filePath.size()>prefix.size()&&(!natives||f.filePath.endsWith(".dll",Qt::CaseInsensitive)))++total;
    QElapsedTimer clock;clock.start();
    for(const auto &f:entries) {
        // QZipReader strips trailing slashes and may call an attribute-less
        // directory a file. An empty entry containing other paths is a folder.
        if(!f.isFile || (f.size==0 && parentFolders.contains(f.filePath.toLower())) || !f.filePath.startsWith(prefix)) continue;
        ctx.check();
        QString name=f.filePath.mid(prefix.size()); if(name.isEmpty()) continue;
        if(natives && !name.endsWith(".dll",Qt::CaseInsensitive)) continue;
        if(natives) name=QFileInfo(name).fileName();
        auto data=z.fileData(f.filePath); if(data.size()!=f.size || crc32(data)!=f.crc || z.status()!=QZipReader::NoError) throw Error("Corrupt archive entry: "+name);
        const auto path=safePath(dest,name);
        if(atomicWrites)writeBytes(path,data);
        else{
            if(!QDir().mkpath(QFileInfo(path).absolutePath()))throw Error("Cannot create staged pack folder.");
            QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate)||file.write(data)!=data.size())throw Error("Cannot stage pack file: "+name);
        }
        ++done;if(done==total||clock.elapsed()>=100){ctx.report("Preparing pack files",done,total);clock.restart();}
    }
}
void copyTree(const QString &from,const QString &to,const Context &ctx) {
    if(!QDir(from).exists()) return;
    QDir().mkpath(to); QDirIterator it(from,QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden,QDirIterator::Subdirectories);
    while(it.hasNext()) {
        ctx.check(); it.next(); auto info=it.fileInfo(); if(info.isSymLink()) throw Error("Cannot import linked files: "+info.fileName());
        auto dest=safePath(to,QDir(from).relativeFilePath(it.filePath()));
        if(info.isDir()) QDir().mkpath(dest); else { QDir().mkpath(QFileInfo(dest).absolutePath()); if(QFile::exists(dest)) QFile::remove(dest); if(!QFile::copy(it.filePath(),dest)) throw Error("Could not copy "+info.fileName()); }
    }
}
void replaceTree(const QString &from,const QString &to) {
    const QString old=to+".previous";
    if(QFileInfo::exists(old)) throw Error("A previous recovery folder exists: "+old);
    bool existed=QFileInfo::exists(to);
    if(existed && !QDir().rename(to,old)) throw Error("Cannot prepare replacement. Close programs using these files.");
    if(!QDir().rename(from,to)) { if(existed) QDir().rename(old,to); throw Error("Cannot commit files. Original files restored."); }
    if(existed) QDir(old).removeRecursively();
}
bool rulesAllow(const QJsonArray &rules,const QJsonObject &features) {
    if(rules.isEmpty()) return true; bool allowed=false;
    for(const auto &v:rules) {
        auto r=v.toObject(), os=r["os"].toObject(); bool match=true;
        if(os.contains("name") && os["name"].toString()!="windows") match=false;
        if(os.contains("arch") && os["arch"].toString()!="x86_64" && os["arch"].toString()!="amd64") match=false;
        if(os.contains("version") && !QRegularExpression(os["version"].toString()).match(QSysInfo::kernelVersion()).hasMatch()) match=false;
        auto ff=r["features"].toObject(); for(auto i=ff.begin();i!=ff.end();++i) if(features.value(i.key()).toBool()!=i.value().toBool()) match=false;
        if(match) allowed=r["action"].toString()=="allow";
    } return allowed;
}
QStringList argumentValues(const QJsonArray &args,const QJsonObject &features) {
    QStringList result;
    for(const auto &v:args) {
        if(v.isString()) result<<v.toString();
        else { auto o=v.toObject(); if(!rulesAllow(o["rules"].toArray(),features)) continue; auto val=o["value"]; if(val.isArray()) for(auto a:val.toArray()) result<<a.toString(); else result<<val.toString(); }
    } return result;
}
QString mavenPath(const QString &name) {
    auto extParts=name.split('@'); auto p=extParts[0].split(':');
    if(p.size()<3 || p.size()>4) throw Error("Unsupported library coordinate: "+name);
    QString group=p[0]; group.replace('.','/');
    return group+'/'+p[1]+'/'+p[2]+'/'+p[1]+'-'+p[2]+(p.size()==4?'-'+p[3]:QString())+'.'+(extParts.size()>1?extParts[1]:"jar");
}
qint64 physicalMemoryMb() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX m{}; m.dwLength=sizeof(m); if(GlobalMemoryStatusEx(&m)) return qint64(m.ullTotalPhys/1024/1024);
#endif
    return 8192;
}
QString timestamp() { return QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz"); }
}
