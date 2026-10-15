#include <QtTest>
#include "engine.h"
#include "settings.h"
#include "room.h"
#include "roomthread.h"
#include "serverplayer.h"
#include "ai.h"
#include "skill.h"
#include "standard.h"
#include "lua.hpp"
class EnyuanCaptureAI : public TrustAI {
public:
 EnyuanCaptureAI(ServerPlayer *p,ServerPlayer *friendly):TrustAI(p),friendly(friendly){}
 QList<ServerPlayer *> asked; ServerPlayer *friendly;
 bool askForSkillInvoke(const QString &,const QVariant &data) override {auto giver=data.value<PlayerStar>();asked<<giver;return giver==friendly;}
};
class CardChoiceAI : public TrustAI {
public:
 explicit CardChoiceAI(ServerPlayer *p):TrustAI(p){}
 int reply = 999999;
 bool removeBeforeReply = false;
 int askForCardChosen(ServerPlayer *who, const QString &flags, const QString &) override {
  if(removeBeforeReply) for(auto card:who->getCards(flags)) who->removeCard(card,who->getRoom()->getCardPlace(card->getId()));
  return reply;
 }
};
class AIDecisionTest : public QObject {
 Q_OBJECT
private slots:
 void initTestCase() { QVERIFY(QDir::setCurrent(QS_ASSET_ROOT)); Sanguosha=new Engine;Config.init(); }
 void cardChoiceRejectsInvalidReply() {
  Config.EnableAI=true;Config.AIDelay=0;
  Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  auto actor=room.addSocket(nullptr);auto lord=room.addSocket(nullptr);
  actor->setObjectName("actor");actor->setState("robot");actor->setRole("rebel");
  lord->setObjectName("lord");lord->setRole("lord");
  auto ai=new CardChoiceAI(actor);actor->setAI(ai);
  const Card *equip=nullptr;
  for(int i=0;i<Sanguosha->getCardCount();++i)if(Sanguosha->getCard(i)->inherits("EquipCard")){equip=Sanguosha->getCard(i);break;}
  QVERIFY(equip);lord->setEquip(static_cast<const EquipCard *>(equip));
  room.setCardMapping(equip->getId(),lord,Player::Equip);
  // A stale/invalid choice must resolve only to a card in the requested area.
  QCOMPARE(room.askForCardChosen(actor,lord,"hej","snatch"),equip->getId());
 }
 void snatchEmptyHand_data() {
  QTest::addColumn<QString>("zone");QTest::addColumn<int>("reply");QTest::addColumn<bool>("remove");
  for(const QString &zone:QStringList{"h","e","j"})
   for(int reply:QList<int>{-1,-2,999999,0})
    QTest::newRow(qPrintable(zone+QString::number(reply)))<<zone<<reply<<false;
  QTest::newRow("target-emptied-during-choice")<<QString("e")<<-1<<true;
 }
 void snatchEmptyHand() {
  QFETCH(QString,zone);QFETCH(int,reply);QFETCH(bool,remove);
  Config.EnableAI=true;Config.AIDelay=0;
  Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  auto actor=room.addSocket(nullptr);auto lord=room.addSocket(nullptr);
  for(auto p:room.getPlayers()){p->setGeneralName("caocao");p->setMaxHp(4);p->setHp(4);}
  actor->setObjectName("actor");actor->setState("robot");actor->setRole("rebel");
  lord->setObjectName("lord");lord->setRole("lord");room.revivePlayer(lord);room.current=actor;
  auto ai=new CardChoiceAI(actor);ai->reply=reply;ai->removeBeforeReply=remove;actor->setAI(ai);
  const Card *chosen=nullptr;
  for(int i=1;i<Sanguosha->getCardCount();++i){auto c=Sanguosha->getCard(i);
   if((zone=="h" && c->inherits("BasicCard")) || (zone=="e" && c->inherits("EquipCard")) || (zone=="j" && c->inherits("DelayedTrick"))){chosen=c;break;}}
  QVERIFY(chosen);
  auto place=zone=="h"?Player::Hand:zone=="e"?Player::Equip:Player::Judging;
  if(zone=="h")lord->drawCard(chosen);
  else if(zone=="e")lord->setEquip(static_cast<const EquipCard *>(chosen));
  else lord->addDelayedTrick(chosen);
  room.setCardMapping(chosen->getId(),lord,place);
  CardEffectStruct effect;effect.from=actor;effect.to=lord;
  Snatch snatch(Card::Spade,3);effect.card=&snatch;snatch.onEffect(effect);
  QCOMPARE(actor->getHandcardNum(),remove?0:1);
  if(!remove)QVERIFY(actor->handCards().contains(chosen->getId()));
  QVERIFY(lord->isAllNude());
 }
 void emptyCardAreas() {
  Config.EnableAI=true;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  auto actor=room.addSocket(nullptr);auto target=room.addSocket(nullptr);
  QCOMPARE(target->getRandomHandCard(),nullptr);QCOMPARE(target->getRandomHandCardId(),-1);
  for(const QString &flags:QStringList{"h","he","hej","e","j"})
   QCOMPARE(room.askForCardChosen(actor,target,flags,"snatch"),-1);
 }
 void equipmentChoiceCannotTakeHand() {
  Config.EnableAI=true;Config.AIDelay=0;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  auto actor=room.addSocket(nullptr);auto target=room.addSocket(nullptr);actor->setState("robot");
  auto ai=new CardChoiceAI(actor);ai->reply=-1;actor->setAI(ai);
  auto hand=Sanguosha->getCard(0);target->drawCard(hand);room.setCardMapping(0,target,Player::Hand);
  const EquipCard *equip=nullptr;
  for(int i=1;i<Sanguosha->getCardCount();++i)if((equip=qobject_cast<const EquipCard *>(Sanguosha->getCard(i))))break;
  QVERIFY(equip);target->setEquip(equip);room.setCardMapping(equip->getId(),target,Player::Equip);
  QCOMPARE(room.askForCardChosen(actor,target,"e","snatch"),equip->getId());
 }
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
