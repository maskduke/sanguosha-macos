#include <QtTest>
#include "engine.h"
#include "settings.h"
#include "room.h"
#include "roomthread.h"
#include "serverplayer.h"
#include "ai.h"
#include "skill.h"
#include "lua.hpp"
class EnyuanCaptureAI : public TrustAI {
public:
 EnyuanCaptureAI(ServerPlayer *p,ServerPlayer *friendly):TrustAI(p),friendly(friendly){}
 QList<ServerPlayer *> asked; ServerPlayer *friendly;
 bool askForSkillInvoke(const QString &,const QVariant &data) override {auto giver=data.value<PlayerStar>();asked<<giver;return giver==friendly;}
};
class AIDecisionTest : public QObject {
 Q_OBJECT
private slots:
 void initTestCase() { QVERIFY(QDir::setCurrent(QS_ASSET_ROOT)); Sanguosha=new Engine;Config.init(); }
 void enyuanBatchSources() {
  Config.EnableAI=true;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  QList<ServerPlayer *> players;
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("source%1").arg(i));p->setRole(i==0?"lord":"rebel");p->setState("robot");p->setGeneralName(i==1?"fazheng":"caocao");p->setMaxHp(4);p->setHp(4);players<<p;}
  auto fazheng=players[1];auto enemy=players[0];auto ally=players[2];room.revivePlayer(fazheng);room.current=fazheng;
  auto ai=new EnyuanCaptureAI(fazheng,ally);fazheng->setAI(ai);
  const auto skill=Sanguosha->getTriggerSkill("enyuan");QVERIFY(skill);
  auto gain=[&](ServerPlayer *giver){CardMoveStruct move{};move.from=giver;move.to=fazheng;move.from_place=Player::Hand;move.to_place=Player::Hand;CardMoveStar ptr=&move;QVariant data=QVariant::fromValue(ptr);skill->trigger(CardGot,&room,fazheng,data);};
  auto finish=[&]{QVariant data;skill->trigger(CardGotDone,&room,fazheng,data);};
  ally->setFlags("xuanhuo_target");
  gain(enemy);gain(enemy);finish();
  QCOMPARE(ai->asked.size(),1);QCOMPARE(ai->asked.last(),enemy);QCOMPARE(enemy->getHandcardNum(),0);
  ai->asked.clear();gain(enemy);gain(ally);finish();QVERIFY(ai->asked.isEmpty());
  gain(nullptr);gain(nullptr);finish();QVERIFY(ai->asked.isEmpty());
  gain(ally);gain(ally);finish();QCOMPARE(ai->asked.size(),1);QCOMPARE(ai->asked.last(),ally);QCOMPARE(ally->getHandcardNum(),1);
  QCOMPARE(fazheng->getMark("enyuan"),0);QVERIFY(!enemy->hasFlag("EnyuanTarget"));
 }
 void enyuanRealLuaBinding() {
  Config.EnableAI=true;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setRole(i==0?"lord":"rebel");p->setState("robot");p->setGeneralName(i==1?"fazheng":"caocao");p->setMaxHp(4);p->setHp(4);}
  auto fazheng=room.getPlayers()[1];auto enemy=room.getPlayers()[0];auto ally=room.getPlayers()[2];room.revivePlayer(fazheng);room.current=fazheng;
  for(auto p:room.getPlayers())p->setAI(room.cloneAI(p));
  // Pin AI perception, rather than making the decision depend on hidden identity inference.
  int rc=luaL_dostring(room.getLuaState(),"sgs.ais.p1.isFriend=function(self,p) return p and p:objectName()=='p2' end");QCOMPARE(rc,0);
  ally->setFlags("xuanhuo_target");
  QVERIFY(!fazheng->getAI()->askForSkillInvoke("enyuan",QVariant::fromValue(static_cast<PlayerStar>(enemy))));
  QVERIFY(fazheng->getAI()->askForSkillInvoke("enyuan",QVariant::fromValue(static_cast<PlayerStar>(ally))));
  DamageStruct damage;damage.from=enemy;damage.to=fazheng;
  QVERIFY(fazheng->getAI()->askForSkillInvoke("enyuan",QVariant::fromValue(damage)));
  damage.from=ally;QVERIFY(!fazheng->getAI()->askForSkillInvoke("enyuan",QVariant::fromValue(damage)));
 }
 void playableHand_data() {
  QTest::addColumn<QString>("primary");QTest::addColumn<QString>("secondary");QTest::addColumn<int>("variant");
  QTest::newRow("sunshangxiang")<<QString("sunshangxiang")<<QString()<<-1;
  QTest::newRow("machao")<<QString("machao")<<QString()<<-1;
  QTest::newRow("sunshangxiang-machao")<<QString("sunshangxiang")<<QString("machao")<<-1;
  QTest::newRow("machao-sunshangxiang")<<QString("machao")<<QString("sunshangxiang")<<-1;
 }
 void fixedHandSweep_data() {
  QTest::addColumn<QString>("primary");QTest::addColumn<QString>("secondary");QTest::addColumn<int>("variant");
  for(int i=0;i<178;++i)QTest::newRow(qPrintable(QString("extra-card-%1").arg(i)))<<QString("sunshangxiang")<<QString("machao")<<i;
 }
 void fixedHandSweep() { playableHand(); }
 void playableHand() {
  QFETCH(QString,primary);QFETCH(QString,secondary);QFETCH(int,variant);
  Config.GameMode="08p";Config.EnableAI=true;Config.Enable2ndGeneral=!secondary.isEmpty();Config.EnableHegemony=false;
  Room room(nullptr,"08p");room.thread=new RoomThread(&room);
  const QStringList roles={"lord","rebel","loyalist","rebel","loyalist","renegade","rebel","loyalist"};
  for(int i=0;i<8;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setRole(roles[i]);p->setState("robot");p->setGeneralName(i==1?primary:"caocao");if(i==1&&!secondary.isEmpty())p->setGeneral2Name(secondary);p->setMaxHp(4);p->setHp(4);p->setSeat(i+1);}
  auto player=room.getPlayers()[1];room.revivePlayer(player);room.current=player;player->setPhase(Player::Play);
  const QStringList names={"slash","slash","deluge","savage_assault","collateral","jink","jink","peach","peach"};QSet<int> used;
  const QList<Card::Suit> fixedSuits={Card::Club,Card::Diamond,Card::Spade,Card::Spade,Card::Club};
  const QList<int> fixedNumbers={8,13,1,7,12};
  int slot=0;
  for(const auto &name:names){const Card *found=nullptr;for(int i=0;i<Sanguosha->getCardCount();++i){auto c=Sanguosha->getCard(i);if(c->objectName()==name&&!used.contains(i)&&(slot>=5||(c->getSuit()==fixedSuits[slot]&&c->getNumber()==fixedNumbers[slot]))){found=c;break;}}QVERIFY2(found,qPrintable(name));used.insert(found->getId());player->drawCard(found);room.setCardMapping(found->getId(),player,Player::Hand);++slot;}
  if(variant>=0){
   room.getPlayers()[3]->setHp(2);room.getPlayers()[0]->setHp(3);
   if(!used.contains(variant)){auto card=Sanguosha->getCard(variant);player->drawCard(card);room.setCardMapping(variant,player,Player::Hand);}
  }
  QStringList messages;connect(&room,&Room::room_message,this,[&](const QString &m){messages<<m;});
  for(auto p:room.getPlayers())p->setAI(room.cloneAI(p));
  CardUseStruct use;use.from=player;player->getAI()->activate(use);
  for(const auto &m:messages)if(m.contains(".lua:")||m.contains("stack traceback"))qWarning().noquote()<<m;
  QVERIFY2(!messages.join('\n').contains("stack traceback"),qPrintable(messages.join('\n')));
  QVERIFY2(use.card,"AI must choose an available play from this nine-card hand against an adjacent enemy lord");
  QVERIFY(use.isValid());
  qInfo()<<"AI_CHOSE"<<primary<<secondary<<use.card->metaObject()->className();
 }
};
QTEST_MAIN(AIDecisionTest)
#include "test_ai_decision.moc"
