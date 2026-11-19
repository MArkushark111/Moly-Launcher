#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QElapsedTimer>
#include <QProcessEnvironment>
#include "minecraft.h"
#include "localedition.h"
using namespace moly;
static QJsonObject probe(LaunchCommand command,const QString &home) {
    QDir().mkpath(command.workingDirectory);
    writeBytes(command.workingDirectory+"/options.txt","soundCategory_master:0.0\nfullscreen:false\n");
    command.arguments.prepend("-Duser.home="+home);
    command.arguments.prepend("-Dhttp.proxyHost=127.0.0.1");command.arguments.prepend("-Dhttp.proxyPort=9");
    command.arguments.prepend("-Dhttps.proxyHost=127.0.0.1");command.arguments.prepend("-Dhttps.proxyPort=9");
    QProcess process;process.setWorkingDirectory(command.workingDirectory);process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(command.program,command.arguments);const bool started=process.waitForStarted(10000);
    QElapsedTimer timer;timer.start();QByteArray output;bool rendered=false;int firstFrame=-1;
    while(started&&process.state()!=QProcess::NotRunning&&timer.elapsed()<60000) {
        process.waitForReadyRead(250);output+=process.readAll();
        if(output.contains("Created:")||output.contains("Created texture")) {rendered=true;if(firstFrame<0)firstFrame=int(timer.elapsed());}
        if(rendered&&timer.elapsed()-firstFrame>5000)break;
    }
    const bool alive=process.state()!=QProcess::NotRunning;
    process.terminate();if(!process.waitForFinished(4000)){process.kill();process.waitForFinished(3000);}output+=process.readAll();
    writeBytes(command.workingDirectory+"/local-launch-probe.log",redact(QString::fromUtf8(output)).toUtf8());
    return {{"processStarted",started},{"rendererInitialized",rendered},{"runningAfterInitialization",alive},{"httpProxyBlocked",true},{"elapsedMs",timer.elapsed()}};
}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QJsonArray result;
    if(app.arguments().size()<2)return 2;
    const auto root=app.arguments()[1];
    for(auto entry:Minecraft(root).versions(true)) {
        const auto version=entry.toObject()["id"].toString();
        QJsonObject instance{{"id","check-"+version},{"name","Local verification"},{"version",version},{"loader","Vanilla"},{"settings",QJsonObject{}}};
        try{
            auto state=Minecraft(root).install(instance,{});
            auto command=Minecraft::command(state,instance,{{"memoryMb",2048},{"width",960},{"height",540}},{{"type","offline"},{"name","LocalVerifier"},{"uuid",offlineUuid("LocalVerifier")}},{});
            bool complete=QFileInfo::exists(command.program);for(auto file:state["classpath"].toArray())complete=complete&&QFileInfo::exists(file.toString());
            QJsonObject row{{"version",version},{"installed",complete},{"javaMajor",state["javaMajor"]},{"classpathCount",state["classpath"].toArray().size()}};
            if(app.arguments().contains("--launch"))row["launchProbe"]=probe(command,root+"/test-home");
            result.append(row);
        }catch(const std::exception &e){result.append(QJsonObject{{"version",version},{"installed",false},{"error",redact(QString::fromUtf8(e.what()))}});}
    }
    for(const auto &game:QDir(root+"/cache/local-loaders").entryList(QDir::Dirs|QDir::NoDotAndDotDot))for(const auto &loader:QDir(root+"/cache/local-loaders/"+game).entryList(QDir::Dirs|QDir::NoDotAndDotDot))for(const auto &v:Minecraft(root).loaders(game,loader)){
        try{
            QJsonObject instance{{"id","check-"+loader+"-"+game},{"version",game},{"loader",loader},{"loaderVersion",v}};
            auto state=Minecraft(root).install(instance,{});
            QJsonObject row{{"version",game},{"loader",loader},{"loaderVersion",v},{"installed",true},{"classpathCount",state["classpath"].toArray().size()}};
            if(app.arguments().contains("--modded-launch"))row["launchProbe"]=probe(Minecraft::command(state,instance,{{"memoryMb",2048},{"width",960},{"height",540}},{{"type","offline"},{"name","LocalVerifier"},{"uuid",offlineUuid("LocalVerifier")}},{}),root+"/test-home");
            result.append(row);
        }catch(const std::exception&e){result.append(QJsonObject{{"version",game},{"loader",loader},{"installed",false},{"error",redact(QString::fromUtf8(e.what()))}});}
    }
    if(QFileInfo::exists(QCoreApplication::applicationDirPath()+"/engines/moly-rust.exe")){
        try{auto state=Minecraft(root).install({{"id","check-rust"},{"engine","moly-rust"},{"version","26.3"},{"loader","Rust Edition"}},{});result.append(QJsonObject{{"version","26.3"},{"engine","moly-rust"},{"installed",QFileInfo::exists(state["resources"].toString()+"/horizon-resources.json")}});}catch(const std::exception&e){result.append(QJsonObject{{"engine","moly-rust"},{"installed",false},{"error",redact(QString::fromUtf8(e.what()))}});}
    }
    QTextStream(stdout)<<QJsonDocument(result).toJson();writeBytes(root+"/offline-install-report.json",QJsonDocument(result).toJson());
    for(auto value:result)if(!value.toObject()["installed"].toBool())return 1;
    return result.isEmpty()?1:0;
}
