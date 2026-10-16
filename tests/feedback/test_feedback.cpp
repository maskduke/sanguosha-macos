#include <QtTest>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include "engine.h"
#include "settings.h"
#include "room.h"
#include "serverplayer.h"
#include "skill.h"
#include "standard.h"
#include "carditem.h"
#include "sprite.h"
#include "mainwindow.h"
#include "server.h"
#include "client.h"
#include "clientplayer.h"
#include "roomthread.h"
#include <QTcpServer>
#include <QPointer>
#include <QTextEdit>
#include <QScopeGuard>
#include "generaloverview.h"
#include "general.h"
#include "roomscene.h"
#include "dashboard.h"
#include "photo.h"
#include <QGraphicsView>
#include <QTemporaryFile>

class HoverCard : public CardItem {
public:
    using CardItem::CardItem;
    void enter() { QGraphicsSceneHoverEvent e(QEvent::GraphicsSceneHoverEnter); hoverEnterEvent(&e); }
    void leave() { QGraphicsSceneHoverEvent e(QEvent::GraphicsSceneHoverLeave); hoverLeaveEvent(&e); }
};
class FeedbackTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QVERIFY(QDir::setCurrent(QS_ASSET_ROOT));
        Sanguosha = new Engine;
        Config.init();
    }
    void skillDescriptionFollowsPalette() {
        const auto oldPalette=qApp->palette();const auto oldStyle=qApp->styleSheet();
        auto restore=qScopeGuard([&]{qApp->setStyleSheet(oldStyle);qApp->setPalette(oldPalette);});
        QFile file("sanguosha.qss");QVERIFY(file.open(QIODevice::ReadOnly));qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
        GeneralOverview dialog;auto edit=dialog.findChild<QTextEdit *>("skillTextEdit");QVERIFY(edit);
        dialog.show();
        for(bool dark:{true,false,true}) {
            QPalette palette=oldPalette;QColor base=dark?QColor(30,30,30):QColor(250,250,250);QColor text=dark?QColor(235,235,235):QColor(20,20,20);
            palette.setColor(QPalette::Base,base);palette.setColor(QPalette::Text,text);
            edit->clear();
            qApp->setPalette(palette);QCoreApplication::processEvents();
            const QImage image=edit->viewport()->grab().toImage();
            QCOMPARE(image.pixelColor(image.width()/2,image.height()/2),base);
            QCOMPARE(edit->palette().color(QPalette::Text),text);
            edit->setHtml(Sanguosha->getGeneral("fazheng")->getSkillDescription());
            QCoreApplication::processEvents();
            const QImage rendered=edit->viewport()->grab().toImage();
            int ink=0;for(int y=0;y<rendered.height();++y)for(int x=0;x<rendered.width();++x) {
                const QColor c=rendered.pixelColor(x,y);
                if(dark ? c.lightness()>150 : c.lightness()<100) ++ink;
            }
            QVERIFY2(ink>40,"Unselected skill text must be visible against the background");
            const QString output=qEnvironmentVariable("QS_THEME_CAPTURE_DIR");
            if(!output.isEmpty()) QVERIFY(rendered.save(output+(dark?"/skills-dark.png":"/skills-light.png")));

        }
    }
    void returnToMenuReleasesLocalServer_data() {
        QTest::addColumn<bool>("completeGame");
        QTest::newRow("return-before-game") << false;
        QTest::newRow("two-completed-games") << true;
    }
    void returnToMenuReleasesLocalServer() {
        QFETCH(bool, completeGame);
        Config.GameMode="02p";
        QTcpServer probe;
        QVERIFY(probe.listen(QHostAddress::LocalHost,0));
        const quint16 port=probe.serverPort(); probe.close();
        Config.ServerPort=port;
        MainWindow window;
        for(int game=0;game<2;++game) {
            QTimer::singleShot(0,&window,[&window] {
                auto dialog=window.findChild<ServerDialog *>();
                if(dialog) dialog->accept();
            });
            QVERIFY(QMetaObject::invokeMethod(&window,"on_actionPC_Console_Start_triggered",Qt::DirectConnection));
            QPointer<Server> server=window.findChild<Server *>();
            QVERIFY(server);
            QVERIFY(!probe.listen(QHostAddress::LocalHost,port));
            if(completeGame) {
                // Exercise real game completion without waiting for human input.
                delete ClientInstance; ClientInstance=nullptr; Self=nullptr;
                Config.EnableAI=true; Config.AIDelay=0; Config.FreeChoose=false;
                Room *room=server->findChildren<Room *>().last();
                QSignalSpy finished(room,&Room::game_over);
                room->startTest("zhangliao");
                QTRY_COMPARE_WITH_TIMEOUT(finished.size(),1,20000);
                QVERIFY(room->getThread()->wait(5000));
            }
            QVERIFY(QMetaObject::invokeMethod(&window,"gotoStartScene",Qt::DirectConnection));
            QVERIFY2(server.isNull(),"Returning to menu must destroy the local server after stopping rooms");
            QVERIFY(probe.listen(QHostAddress::LocalHost,port)); probe.close();
        }
    }
    void repeatedRoomKeepsExpandedLayout() {
        Config.setValue("CircularView", true);
        Config.setValue("UI/ExpandDashboard", true);
        ServerInfo.GameMode="10pz";
        QTemporaryFile replay("/private/tmp/qs-layout-XXXXXX.txt");
        QVERIFY(replay.open());
        Client client(nullptr,replay.fileName());
        const auto cleanup=qScopeGuard([] { Self=nullptr; ClientInstance=nullptr; });
        MainWindow window;
        window.resize(1600,1000);
        window.show();
        QCoreApplication::processEvents();
        QList<QRectF> firstPhotos;
        QRectF firstDashboard;
        for(int entry=0;entry<3;++entry) {
            QVERIFY(QMetaObject::invokeMethod(&window,"enterRoom",Qt::DirectConnection));
            QCoreApplication::processEvents();
            auto view=window.findChild<QGraphicsView *>(); QVERIFY(view);
            QList<QRectF> photos;
            QRectF dashboard;
            for(auto item:view->scene()->items()) {
                if(dynamic_cast<Photo *>(item)) photos << item->sceneBoundingRect();
                if(dynamic_cast<Dashboard *>(item)) dashboard=item->sceneBoundingRect();
            }
            QCOMPARE(photos.size(),9);
            if(entry==0) { firstPhotos=photos; firstDashboard=dashboard; }
            QCOMPARE(dashboard,firstDashboard);
            QCOMPARE(photos,firstPhotos);
            for(int i=0;i<photos.size();++i)
                for(int j=i+1;j<photos.size();++j)
                    QVERIFY2(!photos[i].intersects(photos[j]),"Opponent panels overlap");
        }
    }
    void hoveredCardAboveNeighbours() {
        QGraphicsScene scene;
        auto first = new HoverCard(Sanguosha->getCard(0));
        auto second = new HoverCard(Sanguosha->getCard(1));
        scene.addItem(first); scene.addItem(second);
        first->setFlag(QGraphicsItem::ItemIsFocusable);
        first->setZValue(2.0); second->setZValue(2.0001);
        first->enter();
        QVERIFY(first->zValue() > second->zValue());
        first->leave();
        QCOMPARE(first->zValue(), 2.0);
        first->enter(); first->enter(); first->leave();
        QCOMPARE(first->zValue(), 2.0);
        first->enter(); first->setHandZValue(2.0003);
        QVERIFY(first->zValue() > second->zValue());
        first->leave(); QCOMPARE(first->zValue(), 2.0003);
    }
    void sentbackStaysInsidePhoto_data() {
        QTest::addColumn<double>("dpr");
        QTest::newRow("standard") << 1.0;
        QTest::newRow("retina") << 2.0;
    }
    void sentbackStaysInsidePhoto() {
        QFETCH(double, dpr);
        QGraphicsScene scene; scene.setSceneRect(0,0,200,200);
        QPixmap photo(40,60); photo.fill(Qt::red);
        auto item = scene.addPixmap(photo); item->setPos(20,20);
        auto effect = new SentbackEffect(true); item->setGraphicsEffect(effect); effect->setIndex(40);
        QImage image(QSize(200*dpr,200*dpr), QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr); image.fill(Qt::white);
        QPainter painter(&image); scene.render(&painter, QRectF(0,0,200,200), scene.sceneRect()); painter.end();
        QCOMPARE(image.pixelColor(80*dpr,100*dpr), QColor(Qt::white));
        QVERIFY(image.pixelColor(30*dpr,30*dpr) != QColor(Qt::red));
    }
    void emphasizeRetainsWholeCard_data() { sentbackStaysInsidePhoto_data(); }
    void emphasizeRetainsWholeCard() {
        QFETCH(double, dpr);
        QGraphicsScene scene; scene.setSceneRect(0,0,200,200);
        QPixmap card(40,60); card.fill(Qt::blue);
        { QPainter p(&card); p.fillRect(20,0,20,60,Qt::green); }
        auto item=scene.addPixmap(card); item->setPos(40,40);
        auto effect=new EmphasizeEffect(true); item->setGraphicsEffect(effect); effect->setIndex(40);
        QImage image(QSize(200*dpr,200*dpr), QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr); image.fill(Qt::white);
        QPainter painter(&image); scene.render(&painter,QRectF(0,0,200,200),scene.sceneRect()); painter.end();
        QCOMPARE(image.pixelColor(42*dpr,60*dpr),QColor(Qt::blue));
        QCOMPARE(image.pixelColor(78*dpr,60*dpr),QColor(Qt::green));
        QCOMPARE(image.pixelColor(100*dpr,110*dpr),QColor(Qt::white));
    }
    void shienWithoutLivingOwner() {
        Room room(nullptr, "02p");
        ServerPlayer player(&room);
        const TriggerSkill *skill = Sanguosha->getTriggerSkill("shien");
        QVERIFY(skill);
        QVERIFY(!skill->triggerable(&player));
        Dismantlement card(Card::Spade,3);
        CardUseStruct use; use.card=&card; use.from=&player;
        QVariant data = QVariant::fromValue(use);
        QVERIFY(!skill->trigger(CardUsed,&room,&player,data));
        data=QVariant::fromValue(static_cast<CardStar>(&card));
        QVERIFY(!skill->trigger(CardResponsed,&room,&player,data));
        data=QVariant::fromValue(static_cast<CardStar>(nullptr));
        QVERIFY(!skill->trigger(CardResponsed,&room,&player,data));
    }
};
QTEST_MAIN(FeedbackTest)
#include "test_feedback.moc"
