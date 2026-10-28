#include "discordpresence.h"
#include "startup.h"
#include <QQmlExpression>
#include <QEventLoop>
#include <QMouseEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCryptographicHash>
#include <QJsonDocument>
#include "fileassociation.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QSettings>
#include <QLockFile>
#include <QDir>
#include <QTimer>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QFontDatabase>
#include <QIcon>
#include <QPointer>
#include "launcher.h"
#include "playtargets.h"
#include "skinstore.h"
#include "skinmodel.h"
#include "artwork.h"

static QObject *findVisualObject(QQuickItem *item,const QString &name) {
    if(!item)return nullptr;
    if(item->objectName()==name)return item;
    for(auto *child:item->childItems())if(auto *found=findVisualObject(child,name))return found;
    return nullptr;
}
static QString diagnosticPath;
static QMutex diagnosticMutex;
static void logMessage(QtMsgType, const QMessageLogContext &, const QString &message) {
    QMutexLocker locker(&diagnosticMutex);
    QFile file(diagnosticPath);
    if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write((moly::redact(message)+"\n").toUtf8());
}
int main(int argc,char *argv[]) {
    QGuiApplication app(argc,argv); app.setOrganizationName("Moly");app.setApplicationName("Moly Launcher");app.setApplicationVersion("0.8.3");
    QQuickStyle::setStyle("Basic");
    QFontDatabase::addApplicationFont(":/qt/qml/Moly/assets/Inter.ttf");
    app.setWindowIcon(QIcon(":/qt/qml/Moly/assets/moly.svg"));
    QCommandLineParser parser;parser.addHelpOption();parser.addVersionOption();parser.addOption({"data-dir","Use isolated launcher storage.","path"});parser.addOption({"screenshot","Save the window to PNG and exit (UI smoke test).","path"});parser.addOption({"page","Start on a screen for UI testing.","name"});parser.addOption({"size","Window size for UI testing (e.g. 1024x640).","size"});parser.addOption({"content-type","Discover content type for UI testing.","type"});parser.addOption({"dialog","Open a preview dialog (create, microsoft, remove, install, or pack).","name"});parser.addOption({"verify-switch","Verify the installed-mod switch in isolated storage (requires --data-dir)."});parser.addOption({"verify-motion","Verify immediate reduced-motion behavior in isolated storage (requires --data-dir)."});parser.addOption({"instance-tab","Instance section for UI testing (content, files, places, logs, share, settings).","name"});parser.addOption({"verify-task-ui","Check compact task activity and manual task-panel opening with real isolated exports (requires --data-dir)."});parser.addOption({"settings-tab","Settings section for UI testing (0-3).","index"});parser.addOption({"instance-settings-tab","Instance settings section for UI testing (0-2).","index"});parser.addOption({"verify-confirm-ui","Verify changing a version stays idle, then confirm one real mod install in isolated storage."});parser.addOption({"import-pack","Open a local pack for import.","path"});parser.addOption({"register-file-types","Register .mlypack files for this Windows user."});parser.addPositionalArgument("pack","Optional local .mlypack, .mrpack, or ZIP file.","[pack]");parser.addOption({"verify-reorder-ui","Drag the sidebar reorder handle in isolated storage."});parser.addOption({"verify-pack-ui","Capture a real pack preparation, then cancel before it is ready (isolated storage only).","path"});parser.addOption({"capture-startup","Capture the startup after library loading (UI check).","path"});parser.addOption({"verify-startup","Check real startup stages and menu fade-in."});parser.addOption({"import-source","Import source for UI preview.","name"});parser.addOption({"verify-browser-ui","Install live Iris through the dedicated one-click browser in isolated storage."});parser.addOption({"verify-intro","Check introduction steps and replay in isolated storage."});parser.addOption({"verify-login-ui","Check live sign-in remains responsive and cancellable in isolated storage."});parser.process(app);
    if(parser.isSet("register-file-types")) {try{moly::registerMolyPackFiles(QCoreApplication::applicationFilePath());return 0;}catch(const std::exception &e){qCritical("%s",e.what());return 1;}}
    QString requestedPack=parser.value("import-pack");
    if(requestedPack.isEmpty()&&!parser.positionalArguments().isEmpty())requestedPack=parser.positionalArguments().first();
    if(!requestedPack.isEmpty()) {
        const QFileInfo file(requestedPack);
        if(!file.isFile()||!QStringList{"mlypack","mrpack","zip"}.contains(file.suffix().toLower())){qCritical("Choose a local .mlypack, .mrpack, or ZIP file.");return 2;}
        requestedPack=file.absoluteFilePath();
    }
    QString root=parser.value("data-dir");if(root.isEmpty())root=QSettings().value("dataRoot",QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).toString();root=QDir(root).absolutePath();QDir().mkpath(root);
    diagnosticPath=root+"/launcher-ui.log";
    const auto serverName="MolyLauncher-"+QString::fromLatin1(QCryptographicHash::hash(root.toCaseFolded().toUtf8(),QCryptographicHash::Sha256).toHex().left(20));
    QLockFile lock(root+"/launcher.lock");lock.setStaleLockTime(0);
    if(!lock.tryLock(100)) {
        QLocalSocket socket;socket.connectToServer(serverName);
        if(socket.waitForConnected(600)) {socket.write(QJsonDocument(QJsonObject{{"pack",requestedPack}}).toJson(QJsonDocument::Compact)+'\n');socket.flush();socket.waitForBytesWritten(600);return 0;}
        qCritical("Moly is already using this storage folder.");return 2;
    }
    QFile::remove(diagnosticPath);qInstallMessageHandler(logMessage);
    QLocalServer instanceServer;instanceServer.setSocketOptions(QLocalServer::UserAccessOption);QLocalServer::removeServer(serverName);instanceServer.listen(serverName);
    try {
        qmlRegisterType<SkinModel>("Moly",1,0,"SkinModel");
        qmlRegisterType<ArtworkView>("Moly",1,0,"ArtworkView");
        StartupState startup;
        try{const auto config=moly::readObject(root+"/settings.json",true);startup.configure(QColor(config["accentColor"].toString("#8b5cf6")),config["reducedMotion"].toBool());}catch(const std::exception &){}
        QQmlApplicationEngine startupEngine;startupEngine.rootContext()->setContextProperty("startup",&startup);startupEngine.loadFromModule("Moly","Startup");
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,20);
        Launcher launcher(root);
        startup.configure(QColor(launcher.settings()["accentColor"].toString()),launcher.settings()["reducedMotion"].toBool());startup.next("Preparing player skins");
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,20);
        if(parser.isSet("capture-startup"))for(auto object:startupEngine.rootObjects())if(auto window=qobject_cast<QQuickWindow*>(object))window->grabWindow().save(parser.value("capture-startup"));
        moly::PlayTargets targets(root);SkinStore skins(root);
        const auto discordOverride=QString::fromUtf8(qgetenv("MOLY_DISCORD_APPLICATION_ID"));
        moly::DiscordPresence discord(discordOverride.isEmpty()?QString(MOLY_DISCORD_APPLICATION_ID):discordOverride);
        discord.setActivityProvider([&](QString page,bool share){return launcher.presenceActivity(page,share);});
        auto configurePresence=[&]{discord.configure(launcher.settings().value("discordPresence",true).toBool(),launcher.settings().value("discordShareServer",true).toBool());discord.update();};
        configurePresence();QObject::connect(&launcher,&Launcher::changed,&discord,configurePresence);
        QObject::connect(&launcher,&Launcher::gameStarted,&discord,&moly::DiscordPresence::update);
        QObject::connect(&launcher,&Launcher::gameEnded,&discord,&moly::DiscordPresence::update);
        targets.setPingEnabled(launcher.settings().value("serverPing",true).toBool());
        QTimer targetRefresh;targetRefresh.setSingleShot(true);targetRefresh.setInterval(250);
        QObject::connect(&targetRefresh,&QTimer::timeout,&targets,&moly::PlayTargets::refresh);
        QObject::connect(&launcher,&Launcher::changed,&targets,[&]{targets.setPingEnabled(launcher.settings().value("serverPing",true).toBool());targetRefresh.start();});
        QObject::connect(&targets,&moly::PlayTargets::notice,&launcher,[&](const QString &message){emit launcher.notice(message,false);});
        QObject::connect(&targets,&moly::PlayTargets::errorOccurred,&launcher,[&](const QString &error){emit launcher.notice(error,true);});
        QObject::connect(&skins,&SkinStore::applyRequested,&launcher,&Launcher::applySkin);
        startup.next("Loading your workspace");QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,20);
        QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("startup",&startup);engine.rootContext()->setContextProperty("launcher",&launcher);
        engine.rootContext()->setContextProperty("playTargets",&targets);engine.rootContext()->setContextProperty("skinStore",&skins);engine.rootContext()->setContextProperty("discordPresence",&discord);
        targetRefresh.start();
        QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
        engine.rootContext()->setContextProperty("diagnosticRun",parser.isSet("data-dir"));
        QTimer::singleShot(1500,&launcher,[&]{if(!parser.isSet("data-dir")) {
            QSettings registration;const auto executable=QCoreApplication::applicationFilePath();
            if(registration.value("packAssociationExecutable").toString()!=executable)try{moly::registerMolyPackFiles(executable);registration.setValue("packAssociationExecutable",executable);}catch(const std::exception &e){emit launcher.notice(QString::fromUtf8(e.what()),true);}
        }});
        engine.loadFromModule("Moly","Main");
        startup.next("Rendering the interface");
        if(!engine.rootObjects().isEmpty())if(auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first())) {
            for(auto object:startupEngine.rootObjects())if(auto boot=qobject_cast<QQuickWindow*>(object))boot->setTransientParent(window);
            auto completion=std::make_shared<QMetaObject::Connection>();
            *completion=QObject::connect(window,&QQuickWindow::frameSwapped,&startup,[&startup,completion]{if(startup.ready())return;QObject::disconnect(*completion);startup.next("Ready");},Qt::QueuedConnection);
            startup.showInterface();window->requestUpdate();
        }
        if(parser.isSet("verify-startup")) {
            QObject::connect(&startup,&StartupState::changed,&app,[&]{if(startup.ready())QTimer::singleShot(230,&app,[&]{
                auto window=engine.rootObjects().isEmpty()?nullptr:qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                auto workspace=window?findVisualObject(window->contentItem(),"workspace"):nullptr;
                const bool ok=window&&window->isVisible()&&workspace&&workspace->property("opacity").toDouble()>0.99&&startup.completed()==4;
                qInfo("Startup check: %d completed local stages, %lld ms; interface ready: %s",startup.completed(),startup.elapsedMs(),ok?"yes":"no");QCoreApplication::exit(ok?0:4);
            });});
        }
        auto openPack=[&engine](const QString &path) {
            if(engine.rootObjects().isEmpty())return;
            auto view=engine.rootObjects().first();
            if(path.isEmpty()){if(auto window=qobject_cast<QQuickWindow*>(view)){window->showNormal();window->raise();window->requestActivate();}return;}
            const QFileInfo file(path);
            if(!file.isFile()||!QStringList{"mlypack","mrpack","zip"}.contains(file.suffix().toLower()))return;
            QMetaObject::invokeMethod(view,"requestPackImport",Q_ARG(QVariant,QVariant(file.absoluteFilePath())));
        };
        QObject::connect(&instanceServer,&QLocalServer::newConnection,&app,[&]{
            while(auto socket=instanceServer.nextPendingConnection()) {
                auto buffer=std::make_shared<QByteArray>();
                QObject::connect(socket,&QLocalSocket::readyRead,&app,[socket,buffer,openPack]{
                    buffer->append(socket->readAll());
                    if(buffer->size()>65536){socket->disconnectFromServer();return;}
                    if(!buffer->contains('\n'))return;
                    const auto message=QJsonDocument::fromJson(buffer->left(buffer->indexOf('\n'))).object();
                    openPack(message["pack"].toString());socket->disconnectFromServer();
                });
                QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
                QTimer::singleShot(3000,socket,[socket]{socket->disconnectFromServer();});
            }
        });
        if(!requestedPack.isEmpty())QTimer::singleShot(350,&app,[openPack,requestedPack]{openPack(requestedPack);});
        if(!engine.rootObjects().isEmpty()){auto view=engine.rootObjects().first();if(parser.isSet("size")){auto parts=parser.value("size").split('x');if(parts.size()==2){view->setProperty("width",parts[0].toInt());view->setProperty("height",parts[1].toInt());}}if(parser.isSet("content-type"))view->setProperty("discoverType",parser.value("content-type"));}
        if(parser.isSet("page")&&!engine.rootObjects().isEmpty())engine.rootObjects().first()->setProperty("page",parser.value("page"));
        if(parser.isSet("instance-tab")&&!engine.rootObjects().isEmpty()){
            auto view=engine.rootObjects().first();view->setProperty("page","instance");
            QTimer::singleShot(300,&app,[&,view]{
                const QStringList tabs{"content","files","places","logs","share","settings"};
                const auto index=tabs.indexOf(parser.value("instance-tab"));
                auto loader=view->findChild<QObject*>("pageLoader");
                if(index>=0&&loader)if(auto content=loader->property("item").value<QObject*>())QMetaObject::invokeMethod(content,"switchTab",Q_ARG(QVariant,QVariant(index)));
            });
        }
        if(parser.isSet("settings-tab")||parser.isSet("instance-settings-tab"))QTimer::singleShot(500,&app,[&]{
            if(engine.rootObjects().isEmpty())return;
            auto view=engine.rootObjects().first();
            if(auto loader=view->findChild<QObject*>("pageLoader"))if(auto content=loader->property("item").value<QObject*>()) {
                if(parser.isSet("settings-tab"))content->setProperty("section",parser.value("settings-tab").toInt());
                if(parser.isSet("instance-settings-tab"))if(auto window=qobject_cast<QQuickWindow*>(view))if(auto settings=findVisualObject(window->contentItem(),"instanceSettings"))settings->setProperty("section",parser.value("instance-settings-tab").toInt());
            }
        });
        if(parser.isSet("verify-login-ui")&&parser.isSet("data-dir")) {
            auto heartbeat=new QTimer(&app);heartbeat->setInterval(25);
            QObject::connect(heartbeat,&QTimer::timeout,&app,[&,heartbeat,ticks=0,phase=0,waitingTicks=0]()mutable{
                if(++ticks>1600){launcher.cancelSignIn();heartbeat->stop();QCoreApplication::exit(4);return;}
                if(phase==0&&!launcher.busy()){launcher.signIn();phase=1;}
                else if(phase==1&&launcher.loginState()=="error"){qWarning("Sign-in probe failed before code arrival.");QCoreApplication::exit(4);heartbeat->stop();}
                else if(phase==1&&launcher.loginState()=="waiting") {
                    if(++waitingTicks==40){QQuickWindow *overlay=nullptr;for(auto window:QGuiApplication::allWindows())if(window->objectName()=="loginDesktopOverlay")overlay=qobject_cast<QQuickWindow*>(window);
                        bool copySignal=false;const auto copyConnection=QObject::connect(&launcher,&Launcher::loginCodeCopied,&app,[&copySignal]{copySignal=true;});
                        launcher.copyLoginCode();QObject::disconnect(copyConnection);
                        const bool responsive=!launcher.busy()&&!launcher.loginCode().isEmpty()&&overlay&&overlay->isVisible()&&!overlay->transientParent()&&(overlay->flags()&Qt::WindowStaysOnTopHint)&&copySignal&&overlay->property("copied").toBool()&&QGuiApplication::clipboard()->text()==launcher.loginCode();launcher.cancelSignIn();phase=responsive?2:3;}
                }else if(phase>=2&&launcher.loginState()=="cancelled"&&launcher.loginMessage()=="Sign-in cancelled.") {
                    const bool ok=phase==2&&launcher.loginCode().isEmpty();qInfo("Login UI: event-loop heartbeat during polling, global tasks available, code delivered and cancellation cleared code: %s",ok?"passed":"failed");heartbeat->stop();QCoreApplication::exit(ok?0:4);
                }
            });heartbeat->start();
        }
        if(parser.isSet("verify-intro")&&parser.isSet("data-dir"))QTimer::singleShot(900,&app,[&]{
            auto view=engine.rootObjects().first();auto intro=view->findChild<QObject*>("firstLaunchIntro");if(!intro){QCoreApplication::exit(4);return;}
            const auto account=launcher.activeAccount();QMetaObject::invokeMethod(view,"replayIntro");
            bool ok=intro->property("step").toInt()==0;
            for(int n=1;n<=3;++n){QMetaObject::invokeMethod(intro,"next");ok&=intro->property("step").toInt()==n;}
            QMetaObject::invokeMethod(intro,"finish");ok&=launcher.settings()["introVersion"].toInt()==1;
            QMetaObject::invokeMethod(view,"replayIntro");ok&=intro->property("step").toInt()==0&&launcher.activeAccount()==account;
            qInfo("Intro steps, completion persistence and safe replay: %s; Microsoft configured: %s",ok?"passed":"failed",launcher.microsoftAvailable()?"yes":"no");QCoreApplication::exit(ok&&launcher.microsoftAvailable()?0:4);
        });
        if(parser.isSet("verify-browser-ui")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty()||launcher.instances().isEmpty())return 4;
            auto timer=new QTimer(&app);timer->setInterval(50);
            QObject::connect(timer,&QTimer::timeout,&app,[&,timer,stage=0,ticks=0]()mutable{
                if(++ticks>2400){launcher.cancel();timer->stop();qWarning("One-click browser timed out.");QCoreApplication::exit(4);return;}
                auto view=engine.rootObjects().first();
                if(stage==0&&!launcher.busy()){QMetaObject::invokeMethod(view,"browse",Q_ARG(QVariant,QVariant("mod")));stage=1;return;}
                QQuickWindow *browser=nullptr;for(auto window:QGuiApplication::allWindows())if(window->objectName()=="contentBrowser")browser=qobject_cast<QQuickWindow*>(window);
                if(stage==1&&!launcher.searchLoading()&&!launcher.busy()&&browser) {
                    if(auto button=findVisualObject(browser->contentItem(),"installProject-YL57xq9U")){QMetaObject::invokeMethod(button,"click");stage=2;if(!launcher.installAutomatic()){qWarning("One-click action did not begin.");QCoreApplication::exit(4);timer->stop();}}
                }else if(stage==2&&!launcher.busy()&&!launcher.installAutomatic()) {
                    const auto files=launcher.contentFiles("mod");auto button=browser?findVisualObject(browser->contentItem(),"installProject-YL57xq9U"):nullptr;const auto modal=view->findChild<QObject*>("installDialog");
                    const bool ok=launcher.installError().isEmpty()&&files.size()>=2&&button&&button->property("text").toString()=="Installed"&&modal&&!modal->property("visible").toBool();
                    timer->stop();if(ok)qInfo("One-click UI passed: separate browser, actual Iris button, dependencies installed, Installed state, no confirmation modal.");else qWarning("One-click UI failed: %s",qPrintable(launcher.installError()));QCoreApplication::exit(ok?0:4);
                }
            });timer->start();
        }
        if(parser.isSet("verify-confirm-ui")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty()||launcher.instances().isEmpty())return 4;
            auto timer=new QTimer(&app);timer->setInterval(25);
            QObject::connect(timer,&QTimer::timeout,&app,[&,timer,stage=0,ticks=0]()mutable{
                if(++ticks>2400){timer->stop();qWarning("Confirm UI check timed out.");QCoreApplication::exit(4);return;}
                auto view=engine.rootObjects().first();auto window=qobject_cast<QQuickWindow*>(view);
                if(stage==0&&!launcher.busy()) {
                    const QVariantMap project{{"provider","Modrinth"},{"projectId","iris"},{"title","Iris"},{"type","mod"}};
                    QMetaObject::invokeMethod(view,"installProject",Q_ARG(QVariant,QVariant(project)));stage=1;
                }else if(stage==1&&!launcher.loadingInstallChoices()&&!launcher.busy()) {
                    auto choice=window?findVisualObject(window->contentItem(),"installVersionChoice"):nullptr;
                    auto button=window?findVisualObject(window->contentItem(),"confirmInstallButton"):nullptr;
                    if(!choice||!button||!button->property("visible").toBool()||!button->property("enabled").toBool()||launcher.installVersions().isEmpty()){qWarning("Confirm UI controls were not ready.");QCoreApplication::exit(4);timer->stop();return;}
                    choice->setProperty("currentIndex",qMin(1,int(launcher.installVersions().size()-1)));stage=2;
                }else if(stage==2) {
                    if(launcher.busy()||!launcher.pendingPlan().isEmpty()||!launcher.jobs().isEmpty()){qWarning("Version selection started an install.");QCoreApplication::exit(4);timer->stop();return;}
                    auto button=findVisualObject(window->contentItem(),"confirmInstallButton");
                    QMetaObject::invokeMethod(button,"click");stage=3;
                }else if(stage==3&&!launcher.busy()&&!launcher.installAutomatic()) {
                    const bool ok=launcher.installError().isEmpty()&&!launcher.contentFiles("mod").isEmpty();
                    timer->stop();if(ok)qInfo("Confirm UI check passed: version switch stayed idle; Confirm installed content without a second modal.");else qWarning("Confirm UI install failed.");QCoreApplication::exit(ok?0:4);
                }
            });timer->start();
        }
        if(parser.isSet("verify-reorder-ui")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty()||launcher.instances().size()<2)return 4;
            auto timer=new QTimer(&app);timer->setInterval(100);
            QObject::connect(timer,&QTimer::timeout,&app,[&,timer,ticks=0]()mutable{
                if(++ticks>150){timer->stop();qWarning("Reorder UI check timed out.");QCoreApplication::exit(4);return;}
                if(launcher.busy())return;timer->stop();
                auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                const auto order=launcher.instances();const auto first=order.first().toMap()["id"].toString(),last=order.last().toMap()["id"].toString();
                auto startItem=qobject_cast<QQuickItem*>(findVisualObject(window->contentItem(),"reorder-"+first));
                auto targetItem=qobject_cast<QQuickItem*>(findVisualObject(window->contentItem(),"reorder-"+last));
                if(!startItem||!targetItem){qWarning("Reorder handles missing.");QCoreApplication::exit(4);return;}
                const auto start=startItem->mapToScene(QPointF(12,15)),target=targetItem->mapToScene(QPointF(12,85));
                auto mouse=[window](QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,point,window->mapToGlobal(point.toPoint()),button,buttons,Qt::NoModifier);QCoreApplication::sendEvent(window,&event);};
                mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
                QTimer::singleShot(60,&app,[mouse,start]{mouse(QEvent::MouseMove,start+QPointF(0,14),Qt::NoButton,Qt::LeftButton);});
                QTimer::singleShot(120,&app,[mouse,target]{mouse(QEvent::MouseMove,target,Qt::NoButton,Qt::LeftButton);});
                QTimer::singleShot(180,&app,[mouse,target]{mouse(QEvent::MouseButtonRelease,target,Qt::LeftButton,Qt::NoButton);});
                QTimer::singleShot(450,&app,[&,order,first]{
                    const bool ok=launcher.instances().last().toMap()["id"].toString()==first;
                    for(auto value:order)launcher.moveInstance(value.toMap()["id"].toString(),QString());
                    if(ok)qInfo("Reorder UI check passed: actual handle drag, persisted custom order, originals restored.");else qWarning("Reorder UI check failed.");
                    QCoreApplication::exit(ok?0:4);
                });
            });timer->start();
        }
        if(parser.isSet("verify-pack-ui")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty()||!launcher.instances().isEmpty())return 4;
            auto timer=new QTimer(&app);timer->setInterval(50);
            QObject::connect(timer,&QTimer::timeout,&app,[&,timer,stage=0,ticks=0,captured=std::make_shared<bool>(false)]()mutable{
                if(++ticks>1600){timer->stop();launcher.cancel();qWarning("Pack UI check timed out.");QCoreApplication::exit(4);return;}
                if(stage==0&&!launcher.busy()&&ticks>3){launcher.search("Modrinth","modpack","Fabulously Optimized");stage=1;}
                else if(stage==1&&!launcher.busy()) {
                    QVariantMap project;for(auto value:launcher.results())if(value.toMap()["title"].toString()=="Fabulously Optimized"){project=value.toMap();break;}
                    if(project.isEmpty()){timer->stop();qWarning("Could not find the live pack for UI verification.");QCoreApplication::exit(4);return;}
                    launcher.beginInstall(project,QString(),QString(),true);stage=2;
                    if(launcher.instances().isEmpty()||!launcher.instances().first().toMap()["_pending"].toBool()){timer->stop();launcher.cancel();QCoreApplication::exit(4);return;}
                    QTimer::singleShot(200,&app,[&,captured]{
                        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                        *captured=window&&!window->grabWindow().isNull()&&window->grabWindow().save(parser.value("verify-pack-ui"));
                        launcher.cancel();
                    });
                }else if(stage==2&&*captured&&!launcher.busy()) {
                    bool ready=false;for(auto value:launcher.instances())ready|=value.toMap()["installed"].toBool();
                    const bool ok=!ready&&*captured;timer->stop();if(ok)qInfo("Pack UI check passed: real name/icon and pending cards appeared during preparation; cancellation never marked the pack ready.");else qWarning("Pack UI check failed.");QCoreApplication::exit(ok?0:4);
                }
            });timer->start();
        }
        if(parser.isSet("verify-task-ui")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty()||launcher.instances().isEmpty())return 4;
            auto timer=new QTimer(&app);timer->setInterval(25);
            QObject::connect(timer,&QTimer::timeout,&app,[&,timer,stage=0,ticks=0,sawCompact=false]()mutable{
                if(++ticks>1200){timer->stop();qWarning("Task UI check timed out.");QCoreApplication::exit(4);return;}
                auto view=engine.rootObjects().first();
                auto window=qobject_cast<QQuickWindow*>(view);
                auto panel=view->findChild<QObject*>("downloadPanel");
                auto activity=window?findVisualObject(window->contentItem(),"taskActivity"):nullptr;
                if(!panel||!activity){timer->stop();QCoreApplication::exit(4);return;}
                if(launcher.busy()) {
                    if(launcher.taskOperation()=="export") {
                        sawCompact |= activity->property("activeTask").toBool();
                        if(panel->property("visible").toBool()){qWarning("A task opened the full panel automatically.");QCoreApplication::exit(4);timer->stop();}
                    }
                    return;
                }
                if(stage==0){
                    launcher.exportInstance(QUrl::fromLocalFile(root+"/task-ui-first.mlypack").toString(),"mlypack",false);sawCompact=activity->property("activeTask").toBool();stage=1;
                }else if(stage==1){
                    if(panel->property("visible").toBool()||launcher.jobs().isEmpty()||launcher.canRetry()){timer->stop();qWarning("Tasks should leave the full panel closed.");QCoreApplication::exit(4);return;}
                    launcher.exportInstance(QUrl::fromLocalFile(root+"/task-ui-second.mlypack").toString(),"mlypack",false);stage=2;
                }else{
                    const bool stayedClosed=!panel->property("visible").toBool();
                    QMetaObject::invokeMethod(panel,"open");
                    const bool ok=stayedClosed&&sawCompact&&panel->property("visible").toBool()&&!launcher.canRetry();
                    timer->stop();
                    if(ok)qInfo("Task UI check passed: compact activity, full panel stays closed, manual open, and real export history.");
                    else qWarning("Task UI check failed.");
                    QCoreApplication::exit(ok?0:4);
                }
            });timer->start();
        }
        if(parser.isSet("verify-motion")) {
            if(!parser.isSet("data-dir")||engine.rootObjects().isEmpty())return 4;
            auto view=engine.rootObjects().first();view->setProperty("page","settings");
            const bool initial=launcher.settings()["reducedMotion"].toBool();
            launcher.setReducedMotion(false);
            QTimer::singleShot(450,&app,[&,view,initial]{
                auto window=qobject_cast<QQuickWindow*>(view);
                auto control=window?findVisualObject(window->contentItem(),"reduceMotionSwitch"):nullptr;
                auto probe=window?findVisualObject(window->contentItem(),"motionPreviewSwitch"):nullptr;
                auto memory=window?findVisualObject(window->contentItem(),"memoryDraftSlider"):nullptr;
                if(!control||!probe){qWarning("Motion UI check: controls missing.");QCoreApplication::exit(4);return;}
                if(memory)memory->setProperty("value",2048);
                const bool before=probe->property("checked").toBool();
                const bool invoked=QMetaObject::invokeMethod(control,"click");
                const bool persisted=moly::readObject(root+"/settings.json")["reducedMotion"].toBool();
                probe->setProperty("checked",!before);
                const bool snapped=qAbs(probe->property("thumbPosition").toDouble()-(!before?1.0:0.0))<0.001;
                const bool off=invoked&&persisted&&snapped&&launcher.settings()["reducedMotion"].toBool()&&memory&&memory->property("value").toInt()==2048;
                QMetaObject::invokeMethod(control,"click");
                probe->setProperty("checked",before);
                QTimer::singleShot(60,&app,[&,probe,off,initial]{
                    const double position=probe->property("thumbPosition").toDouble();
                    const bool ok=off&&!launcher.settings()["reducedMotion"].toBool()&&position>0.001&&position<0.999;
                    launcher.setReducedMotion(initial);
                    if(ok)qInfo("Motion UI check passed: click saves immediately, reduced motion snaps, normal motion animates.");
                    else qWarning("Motion UI check failed.");
                    QCoreApplication::exit(ok?0:4);
                });
            });
        }
        if(parser.isSet("verify-switch")) {
            if(!parser.isSet("data-dir"))return 4;
            for(auto value:launcher.instances())if(value.toMap()["modCount"].toInt()>0){launcher.selectInstance(value.toMap()["id"].toString());break;}
            if(!engine.rootObjects().isEmpty())engine.rootObjects().first()->setProperty("page","instance");
            auto checkTimer=new QTimer(&app);checkTimer->setInterval(100);
            QObject::connect(checkTimer,&QTimer::timeout,&app,[&,checkTimer,attempt=0]()mutable{
                if(++attempt>100){checkTimer->stop();qWarning("Switch UI check timed out.");QCoreApplication::exit(4);return;}
                if(launcher.busy())return;
                auto rootObject=engine.rootObjects().isEmpty()?nullptr:engine.rootObjects().first();
                auto control=rootObject?rootObject->findChild<QObject*>("installedModSwitch"):nullptr;
                if(!control)if(auto window=qobject_cast<QQuickWindow*>(rootObject))control=findVisualObject(window->contentItem(),"installedModSwitch");
                auto files=launcher.contentFiles("mod");if(!control||files.isEmpty())return;
                checkTimer->stop();QPointer<QObject> original=control;bool initial=control->property("checked").toBool();
                auto path=files.first().toMap()["path"].toString();auto toggled=path.endsWith(".disabled")?path.left(path.size()-9):path+".disabled";
                launcher.toggleFile(path);
                auto passed=std::make_shared<bool>(false);
                QTimer::singleShot(60,&app,[original,initial,passed]{
                    if(!original)return;const double position=original->property("thumbPosition").toDouble();
                    *passed=original->property("checked").toBool()!=initial&&position>0.001&&position<0.999;
                });
                QTimer::singleShot(350,&app,[&,original,initial,passed,toggled]{
                    launcher.toggleFile(toggled);
                    QTimer::singleShot(300,&app,[original,initial,passed]{
                        const bool ok=*passed&&original&&original->property("checked").toBool()==initial;
                        if(ok)qInfo("Switch UI check passed: delegate preserved, thumb slides, file state restored.");
                        else qWarning("Switch UI check failed.");
                        QCoreApplication::exit(ok?0:4);
                    });
                });
            });checkTimer->start();
        }
        if(parser.isSet("dialog"))QTimer::singleShot(300,&app,[&]{
            if(engine.rootObjects().isEmpty())return;
            auto view=engine.rootObjects().first();
            if(parser.value("dialog").startsWith("intro")){QMetaObject::invokeMethod(view,"replayIntro");if(auto intro=view->findChild<QObject*>("firstLaunchIntro"))intro->setProperty("step",parser.value("dialog")=="intro-colors"?2:parser.value("dialog")=="intro-account"?3:0);}
            else if(parser.value("dialog")=="create"||parser.value("dialog")=="find-pack"){QMetaObject::invokeMethod(view,"create");if(parser.value("dialog")=="find-pack")if(auto dialog=view->findChild<QObject*>("createDialog"))dialog->setProperty("choice",1);}
            else if(parser.value("dialog")=="import"){QMetaObject::invokeMethod(view,"importInstances");if(parser.isSet("import-source"))if(auto dialog=view->findChild<QObject*>("importInstancesDialog"))QMetaObject::invokeMethod(dialog,"chooseSource",Q_ARG(QVariant,QVariant(parser.value("import-source"))));}
            else if(parser.value("dialog")=="tasks")QMetaObject::invokeMethod(view,"openTaskCenter");
            else if((parser.value("dialog")=="install"||parser.value("dialog")=="pack")&&parser.isSet("data-dir")){
                const bool pack=parser.value("dialog")=="pack";
                const QVariantMap project{{"provider","Modrinth"},{"projectId",pack?"fabulously-optimized":"sodium"},{"title",pack?"Fabulously Optimized":"Sodium"},{"type",pack?"modpack":"mod"}};
                QMetaObject::invokeMethod(view,"installProject",Q_ARG(QVariant,QVariant(project)));
            }
            else if(parser.value("dialog")=="remove"){
                auto files=launcher.contentFiles("mod");
                if(!files.isEmpty())launcher.removeFile(files.first().toMap()["path"].toString());
            }
            else if(parser.value("dialog")=="provider"||parser.value("dialog")=="instance-menu") {
                if(auto window=qobject_cast<QQuickWindow*>(view)) {
                    if(parser.value("dialog")=="provider") {
                        if(auto control=findVisualObject(window->contentItem(),"providerChoice"))if(auto popup=control->property("popup").value<QObject*>())QMetaObject::invokeMethod(popup,"open");
                    } else if(auto menu=view->findChild<QObject*>("instanceMenu"))QMetaObject::invokeMethod(menu,"open");
                }
            }
            else if(parser.value("dialog")=="microsoft"||parser.value("dialog")=="account-chooser")QMetaObject::invokeMethod(view,"showAccountChooser");
            else if(parser.value("dialog")=="browser")QMetaObject::invokeMethod(view,"browse",Q_ARG(QVariant,QVariant("mod")));
            else if(parser.value("dialog")=="server")QMetaObject::invokeMethod(view,"addServer");
            else if(parser.value("dialog")=="versions") {
                view->setProperty("page","instance");QTimer::singleShot(600,&app,[view,&launcher]{if(auto dialog=view->findChild<QObject*>("contentVersionsDialog")){QVariantMap file{{"provider","Modrinth"},{"projectId","P7dR8mSH"},{"title","Fabric API"},{"type","mod"},{"versionId",""}};for(auto value:launcher.installedContent())if(value.toMap()["projectId"]=="P7dR8mSH")file=value.toMap();QMetaObject::invokeMethod(dialog,"openFile",Q_ARG(QVariant,QVariant(file)));}});
            }

        });
        if(parser.value("dialog")=="logs-preview"&&parser.isSet("data-dir")) {
            auto view=engine.rootObjects().first();view->setProperty("page","instance");
            QTimer::singleShot(900,&app,[view]{if(auto log=view->findChild<QObject*>("logView")){
                if(auto loader=view->findChild<QObject*>("pageLoader"))if(auto page=loader->property("item").value<QObject*>())QMetaObject::invokeMethod(page,"switchTab",Q_ARG(QVariant,QVariant(3)));
                log->setProperty("shown",QString("[INFO] UI preview: Minecraft output\n[WARN] UI preview warning\n[ERROR] UI preview error\n[INFO] <b>Log markup stays literal</b>"));
            }});
        }
        if(parser.value("dialog")=="play-loading") {
            if(!engine.rootObjects().isEmpty())engine.rootObjects().first()->setProperty("page","instance");
            QTimer::singleShot(2200,&app,[&]{
                auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                auto button=findVisualObject(window->contentItem(),"instancePlayButton");
                if(button){QQmlExpression expression(engine.rootContext(),button,"(loading = true, text = 'Starting')");expression.evaluate();if(expression.hasError())qWarning("Play loading preview could not be prepared.");}
            });
        }
        if(parser.isSet("screenshot"))QTimer::singleShot(2500,&app,[&]{
            bool saved=false;
            if(parser.value("dialog")=="browser") {
                for(auto window:QGuiApplication::allWindows())if(window->title()=="Moly · Add content")if(auto quick=qobject_cast<QQuickWindow*>(window))saved=quick->grabWindow().save(parser.value("screenshot"))||saved;
            }else for(auto object:engine.rootObjects())if(auto window=qobject_cast<QQuickWindow*>(object))saved=window->grabWindow().save(parser.value("screenshot"))||saved;
            QCoreApplication::exit(saved?0:3);
        });
        return app.exec();
    } catch(const std::exception &e) {qCritical("%s",e.what());return 1;}
}
