#include "rustvisuals.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
namespace moly {
namespace {
QByteArray bytes(const QString &path) {
    QFile f(path);if(!f.exists())return {};
    if(f.size()>1024*1024||!f.open(QIODevice::ReadOnly))throw Error("Cannot read graphics settings.");
    return f.readAll();
}
QMap<QString,QString> parse(const QByteArray &data) {
    QMap<QString,QString> out;
    for(auto line:QString::fromUtf8(data).split('\n')){auto split=line.indexOf(':');if(split>0)out[line.left(split).trimmed()]=line.mid(split+1).trimmed();}
    return out;
}
QByteArray merge(const QByteArray &data,QMap<QString,QString> changed) {
    QStringList out;
    for(auto line:QString::fromUtf8(data).split('\n')){
        line.remove('\r');const auto split=line.indexOf(':');
        if(split>0&&changed.contains(line.left(split).trimmed()))continue;
        if(!line.isEmpty())out<<line;
    }
    for(auto i=changed.cbegin();i!=changed.cend();++i)out<<i.key()+':'+i.value();
    return (out.join('\n')+'\n').toUtf8();
}
}
QJsonObject RustVisuals::read(const QString &game) {
    const auto base=parse(bytes(game+"/options.txt")),extra=parse(bytes(game+"/horizon-graphics.txt"));
    const auto number=[](const auto &map,const QString &key,int fallback){bool ok=false;const auto n=map.value(key).toInt(&ok);return ok?n:fallback;};
    return {{"nearChunks",qBound(2,number(base,"renderDistance",12),64)},
        {"distantChunks",qBound(0,number(extra,"lodDistance",256),1024)},
        {"fps",qBound(30,number(base,"maxFps",144),1000)},
        {"resolutionScale",qBound(50,number(extra,"resolutionScale",100),150)},
        {"fxaa",extra.value("fxaa")=="true"},{"vsync",base.value("enableVsync","true")=="true"},
        {"graphicsApi",extra.value("graphicsApi","auto")},{"shaderPack",extra.value("pickedShaderPack")},
        {"profile","custom"}};
}
QJsonObject RustVisuals::validate(const QJsonObject &values) {
    QJsonObject out;
    for(const auto &key:QStringList{"nearChunks","distantChunks","fps","resolutionScale","graphicsApi","shaderPack","profile"})
        if(values.contains(key))out[key]=values[key];
    out["fxaa"]=values["fxaa"].toBool();out["vsync"]=values["vsync"].toBool(true);
    for(const auto &entry:QList<QPair<QString,QPair<int,int>>>{{"nearChunks",{2,64}},{"distantChunks",{0,1024}},{"fps",{30,1000}},{"resolutionScale",{50,150}}}){
        const auto n=values[entry.first].toInt(-1);
        if(n<entry.second.first||n>entry.second.second)throw Error("Choose a valid value for "+entry.first+'.');
    }
    if(values["distantChunks"].toInt()>0&&values["distantChunks"].toInt()<values["nearChunks"].toInt())throw Error("Distant terrain must reach at least as far as detailed chunks, or be off.");
    if(!QStringList{"auto","vulkan","dx12","opengl"}.contains(values["graphicsApi"].toString()))throw Error("Choose a supported graphics API.");
    const auto pack=values["shaderPack"].toString();
    if(pack.size()>160||pack.contains('/')||pack.contains('\\')||pack.contains('\n')||pack.contains('\r')||pack.contains('\0')||pack.contains(':')||pack=="."||pack=="..")throw Error("Choose a shader from this instance's shaderpacks folder.");
    if(!QStringList{"swift","balanced","vista","custom"}.contains(out["profile"].toString()))out["profile"]="custom";
    return out;
}
void RustVisuals::apply(const QString &game,const QJsonObject &input,const Context &context) {
    const auto values=validate(input);const auto pack=values["shaderPack"].toString();
    if(pack=="Moly Colour"){
        const auto source=QCoreApplication::applicationDirPath()+"/rust-shaders/Moly Colour";
        const auto dest=game+"/shaderpacks/Moly Colour";
        if(!QFileInfo::exists(dest+"/horizon-shaders.toml")){
            if(!QFileInfo::exists(source+"/horizon-shaders.toml"))throw Error("The bundled Moly shader is missing. Reinstall Moly.");
            copyTree(source,dest,context);
        }
    }
    if(!pack.isEmpty()&&!QFileInfo::exists(safePath(game+"/shaderpacks",pack)))throw Error("The selected shader was removed. Select another shader or Off in Rust visuals.");
    const auto options=game+"/options.txt",graphics=game+"/horizon-graphics.txt";
    const bool hadOptions=QFileInfo::exists(options),hadGraphics=QFileInfo::exists(graphics);
    const auto beforeOptions=bytes(options),beforeGraphics=bytes(graphics);
    const auto boolean=[](bool value){return value?QString("true"):QString("false");};
    context.check();
    try {
        writeBytes(options,merge(beforeOptions,{{"renderDistance",QString::number(values["nearChunks"].toInt())},{"maxFps",QString::number(values["fps"].toInt())},{"enableVsync",boolean(values["vsync"].toBool())}}));
        writeBytes(graphics,merge(beforeGraphics,{{"preset","custom"},{"lodDistance",QString::number(values["distantChunks"].toInt())},{"resolutionScale",QString::number(values["resolutionScale"].toInt())},{"fxaa",boolean(values["fxaa"].toBool())},{"graphicsApi",values["graphicsApi"].toString()},{"pickedShaderPack",pack}}));
    }catch(...){
        if(hadOptions)writeBytes(options,beforeOptions);else QFile::remove(options);
        if(hadGraphics)writeBytes(graphics,beforeGraphics);else QFile::remove(graphics);
        throw;
    }
}
}
