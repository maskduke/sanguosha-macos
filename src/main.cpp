#include <QApplication>

#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <cstring>
#include <QDateTime>

#include "mainwindow.h"
#include "settings.h"
#include "banpair.h"
#include "server.h"
#include "audio.h"
#include "scenario.h"
#include "room.h"
#include "roomthread.h"
#include <QTimer>

int main(int argc, char *argv[])
{    
    if(argc > 1 && strcmp(argv[1], "-server") == 0)
        new QCoreApplication(argc, argv);
    else
        new QApplication(argc, argv);

    QCoreApplication::setOrganizationName("org.qsanguosha.nirvana.modern");
    QCoreApplication::setApplicationName("QSanguoshaNirvana");
    const QString resources=QCoreApplication::applicationDirPath()+"/../Resources/game";
    if(!QFileInfo::exists(resources+"/lua/config.lua") || !QDir::setCurrent(resources)) {
        qCritical("Bundled game resources are missing"); return 2;
    }

    // initialize random seed for later use
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&translator);

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [] {
        // Stop readers before the engine's deferred resource destruction.
        if(!Server::shutdownAll()) { qCritical("Game shutdown timed out; skipping unsafe resource destruction"); std::_Exit(2); }
    });
    Sanguosha = new Engine;
    Config.init();
    BanPair::loadBanPairs();

    if(qApp->arguments().contains("-server")){
        Server *server = new Server(qApp);
        printf("Server is starting on port %u\n", Config.ServerPort);

        if(server->listen())
            printf("Starting successfully\n");
        else
            printf("Starting failed!\n");

        return qApp->exec();
    }

    if(qApp->arguments().contains("--ai-smoke") || qApp->arguments().contains("--shutdown-smoke")) {
        Config.GameMode=qEnvironmentVariable("QS_TEST_MODE", "02p");
        Config.EnableAI=true; Config.AIDelay=0; Config.FreeChoose=false;
        Server *server=new Server(qApp);
        Room *room=server->createNewRoom();
        QObject::connect(room, &Room::game_over, qApp, [room](const QString &winner) {
            qInfo() << "NIRVANA_AI_COMPLETE" << Config.GameMode << winner;
            if(!room->getThread()->wait(5000)) { qCritical("Game thread did not finish"); qApp->exit(2); }
            else if(!qApp->arguments().contains("--shutdown-smoke")) qApp->exit(0);
        });
        QTimer::singleShot(180000, qApp, [] { qCritical("AI smoke timed out"); qApp->exit(1); });
        room->startTest("zhangliao");
        if(qApp->arguments().contains("--shutdown-smoke")) QTimer::singleShot(500,qApp,[]{qInfo("NIRVANA_SHUTDOWN_TEST");qApp->quit();});
        return qApp->exec();
    }

    QFile file("sanguosha.qss");
    if(file.open(QIODevice::ReadOnly)){
        QTextStream stream(&file);
        qApp->setStyleSheet(stream.readAll());
    }

#ifdef AUDIO_SUPPORT

    Audio::init();

#endif

    if(qApp->arguments().contains("--smoke-test")) {
        qInfo() << "NIRVANA_SMOKE" << Sanguosha->getVersionNumber()
                << "cards" << Sanguosha->getCardCount()
                << "generals" << Sanguosha->getGeneralCount(true)
                << "packages" << Sanguosha->getExtensions()
                << "scenarios" << Sanguosha->getScenarioNames();
        bool valid = Sanguosha->getCardCount() == 178 && Sanguosha->getGeneralCount(true) == 135;
        for(int i=1;i<=33;++i) {
            const QString name=QString("_mini_%1").arg(i,2,10,QChar('0'));
            const Scenario *scene=Sanguosha->getScenario(name);
            if(!scene || scene->getPlayerCount()<2) { qCritical() << "Invalid mini scene" << name; valid=false; }
        }
        Config.sync(); Audio::quit(); return valid ? 0 : 1;
    }
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [] { Config.sync(); Audio::quit(); });
    MainWindow *main_window = new MainWindow;

    Sanguosha->setParent(main_window);
    main_window->show();

    foreach(QString arg, qApp->arguments()){
        if(arg.startsWith("-connect:")){
            arg.remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostAddress", arg);

            main_window->startConnection();

            break;
        }
    }

    return qApp->exec();
}
