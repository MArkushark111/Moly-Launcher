#include "localedition.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
namespace moly {
void seedLocalLibrary(const QString &source,const QString &destination,const Context &context) {
    const auto manifest=readObject(source+"/manifest.json");
    if(manifest["schemaVersion"].toInt()!=1||!manifest["privatePersonalBackup"].toBool())throw Error("Invalid personal local library manifest.");
    const auto revision=fileHash(source+"/manifest.json","sha256");
    if(readObject(destination+"/local-library-state.json",true)["revision"].toString()==revision)return;
    qint64 completed=0;const auto entries=manifest["files"].toArray();
    for(auto value:entries) {
        context.check();const auto entry=value.toObject();const auto relative=entry["path"].toString();
        if(!relative.startsWith("cache/")&&!relative.startsWith("runtimes/"))throw Error("Local library contains a non-game path.");
        const auto from=safePath(source,relative),to=safePath(destination,relative),hash=entry["sha256"].toString();
        if(!QFileInfo::exists(to)||QFileInfo(to).size()!=qint64(entry["size"].toDouble())||fileHash(to,"sha256")!=hash) {
            QFile input(from);if(!input.open(QIODevice::ReadOnly))throw Error("Missing bundled game file: "+relative);
            QDir().mkpath(QFileInfo(to).absolutePath());QSaveFile output(to);
            if(!output.open(QIODevice::WriteOnly))throw Error("Cannot prepare local game file: "+relative);
            QCryptographicHash actual(QCryptographicHash::Sha256);qint64 size=0;
            while(!input.atEnd()) {context.check();const auto bytes=input.read(1024*1024);if(bytes.isEmpty()&&input.error()!=QFile::NoError)throw Error("Could not read local game file.");actual.addData(bytes);size+=bytes.size();if(output.write(bytes)!=bytes.size())throw Error("Could not copy local game file.");}
            if(QString::fromLatin1(actual.result().toHex())!=hash||size!=qint64(entry["size"].toDouble()))throw Error("Bundled game checksum mismatch: "+relative);
            if(!output.commit())throw Error("Could not finish local game file.");
        }
        context.report("Preparing local library",++completed,entries.size());
    }
    writeObject(destination+"/local-library-state.json",{{"revision",revision}});
}
}
