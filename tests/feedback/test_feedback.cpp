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
