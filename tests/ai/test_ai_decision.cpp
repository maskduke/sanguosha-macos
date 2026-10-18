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
#include "client.h"
#include "socket.h"
#include "yjcm-package.h"
#include "gamerule.h"
#include "playercarddialog.h"
#include <QCommandLinkButton>
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
class RecordingSocket : public ClientSocket {
public:
 QStringList messages;
 void connectToHost() override {}
 void disconnectFromHost() override {}
 void send(const QString &message) override { messages << message; }
 bool isConnected() const override { return true; }
 QString peerName() const override { return "fixture"; }
 QString peerAddress() const override { return "127.0.0.1"; }
};
class XuanhuoGiveAI : public TrustAI {
public:
 using TrustAI::TrustAI;
 bool equipFirst=false;
 bool askForSkillInvoke(const QString &,const QVariant &) override {return true;}
 ServerPlayer *askForPlayerChosen(const QList<ServerPlayer *> &targets,const QString &) override {return targets.first();}
 QString askForChoice(const QString &,const QString &,const QVariant &) override {return "give";}
 int askForCardChosen(ServerPlayer *who,const QString &,const QString &) override {
  if(who->hasEquip() && (equipFirst || who->isKongcheng()))return who->getEquips().first()->getId();
  return who->getHandcards().first()->getId();
 }
};
class ChoiceCapture : public TriggerSkill {
public:
 ChoiceCapture():TriggerSkill("test-choice-capture"){events<<ChoiceMade;}
 mutable QString chosen;
 bool triggerable(const ServerPlayer *) const override {return true;}
 bool trigger(TriggerEvent,Room *,ServerPlayer *,QVariant &data) const override {
  if(data.toString().startsWith("playerChosen:"))chosen=data.toString();return false;
 }
};
class AIDecisionTest : public QObject {
 Q_OBJECT
private slots:
 void initTestCase() { QVERIFY(QDir::setCurrent(QS_ASSET_ROOT)); Sanguosha=new Engine;Config.init(); }
 void lianliRefusalWithCrossbow_data() {
  QTest::addColumn<bool>("crossbow");QTest::addColumn<bool>("priorSlash");
  for(bool weapon:{false,true})for(bool used:{false,true})
   QTest::newRow(qPrintable(QString("crossbow-%1-prior-slash-%2").arg(weapon).arg(used)))<<weapon<<used;
 }
 void lianliRefusalWithCrossbow() {
  QFETCH(bool,crossbow);QFETCH(bool,priorSlash);
  Config.EnableAI=true;Config.AIDelay=0;Config.Enable2ndGeneral=false;
  Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setGeneralName(i==1?"xiahoujuan":"caocao");p->setRole(i<2? (i==0?"lord":"loyalist"):"rebel");p->setState("robot");p->setMaxHp(4);p->setHp(4);room.revivePlayer(p);}
  auto requester=room.getPlayers()[0];auto provider=room.getPlayers()[1];auto enemy=room.getPlayers()[3];
  room.current=requester;requester->setPhase(Player::Play);requester->setMark("@tied",1);provider->setMark("@tied",1);
  requester->acquireSkill("lianli-slash");
  if(crossbow)for(int i=0;i<Sanguosha->getCardCount();++i){auto c=Sanguosha->getCard(i);if(c->objectName()=="crossbow"){requester->addCard(c,Player::Equip);room.setCardMapping(i,requester,Player::Equip);break;}}
  if(priorSlash)requester->addHistory("Slash");
  for(auto p:room.getPlayers())p->setAI(room.cloneAI(p));
  GameRule rule(nullptr);room.thread->constructTriggerTable(&rule);
  const Card *skill=Card::Parse("@LianliSlashCard=.");QVERIFY(skill);
  CardUseStruct use;use.card=skill;use.from=requester;use.to<<enemy;
  // The empty provider takes the same refusal path as a cancelled human reply.
  room.useCard(use);
  QVERIFY2(requester->hasUsed("LianliSlashCard"),"Crossbow must not suppress the failed skill request history");
  QCOMPARE(enemy->getHp(),4);
  CardUseStruct next;next.from=requester;requester->getAI()->activate(next);
  QVERIFY2(!next.card || !next.card->inherits("LianliSlashCard"),"AI must not immediately repeat the refused request");
  if(crossbow) {
   QVERIFY(Slash::IsAvailable(requester));
   Slash ordinary(Card::NoSuit,0);CardUseStruct attack;attack.card=&ordinary;attack.from=requester;attack.to<<enemy;
   room.useCard(attack);const int count=requester->getSlashCount();
   room.useCard(attack);
   QCOMPARE(requester->getSlashCount(),count);
   QCOMPARE(enemy->getHp(),2);
   QVERIFY(Slash::IsAvailable(requester));
  }
 }
 void zaoyaoThreshold_data() {
  QTest::addColumn<int>("count");QTest::addColumn<int>("order");QTest::addColumn<QString>("mode");
  for(const QString &mode:QStringList{"04p","10pz"})for(int count:{13,14,16})for(int order:{0,1,2})QTest::newRow(qPrintable(QString("%1-%2-order-%3").arg(mode).arg(count).arg(order)))<<count<<order<<mode;
 }
 void zaoyaoThreshold() {
  QFETCH(int,count);QFETCH(int,order);QFETCH(QString,mode);Config.EnableAI=true;Config.Enable2ndGeneral=order!=0;Config.EnableScene=false;Config.EnableBasara=false;Config.EnableHegemony=false;
  Room room(nullptr,mode);room.thread=new RoomThread(&room);
  auto p=room.addSocket(nullptr);p->setObjectName("caochong");p->setGeneralName(order==2?"caocao":"caochong");if(order)p->setGeneral2Name(order==2?"caochong":"caocao");p->setRole("lord");p->setMaxHp(3);p->setHp(3);p->setState("robot");room.revivePlayer(p);room.current=p;
  for(int i=1;i<(mode=="10pz"?10:4);++i){auto other=room.addSocket(nullptr);other->setObjectName(QString("other%1").arg(i));other->setGeneralName("caocao");other->setRole("rebel");other->setMaxHp(4);other->setHp(4);other->setState("robot");}
  room.revivePlayer(p);
  for(int i=0;i<count;++i){p->drawCard(Sanguosha->getCard(i));room.setCardMapping(i,p,Player::Hand);}
  GameRule rule(nullptr);room.thread->constructTriggerTable(&rule);
  p->play(QList<Player::Phase>{Player::Discard,Player::Finish});
  QCOMPARE(p->getHandcardNum(),count>13?0:count);QCOMPARE(p->getHp(),count>13?2:3);
 }
 void yanxiaoHelpsControlledFriend_data() {
  QTest::addColumn<int>("handCount");QTest::addColumn<QString>("scenario");QTest::addColumn<QString>("expected");
  QTest::newRow("last-diamond")<<1<<QString("friend")<<QString("p1");
  QTest::newRow("two-hand-cards")<<2<<QString("friend")<<QString("p1");
  QTest::newRow("self-controlled")<<1<<QString("self")<<QString("p0");
  QTest::newRow("no-control")<<1<<QString("none")<<QString();
  QTest::newRow("already-covered")<<1<<QString("covered")<<QString();
 }
 void yanxiaoHelpsControlledFriend() {
  QFETCH(int,handCount);QFETCH(QString,scenario);QFETCH(QString,expected);Config.EnableAI=true;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setGeneralName(i==0?"bgm_daqiao":"caocao");p->setRole(i==0?"lord":i==1?"loyalist":"rebel");p->setState("robot");p->setMaxHp(4);p->setHp(4);}
  auto p=room.getPlayers()[0];auto friendly=room.getPlayers()[1];room.revivePlayer(p);room.current=p;
  const Card *diamond=nullptr,*delay=nullptr;
  for(int i=0;i<Sanguosha->getCardCount();++i){auto c=Sanguosha->getCard(i);if(!diamond&&c->getSuit()==Card::Diamond&&c->inherits("BasicCard"))diamond=c;if(c->objectName()=="indulgence")delay=c;}
  QVERIFY(diamond);QVERIFY(delay);p->drawCard(diamond);room.setCardMapping(diamond->getId(),p,Player::Hand);
  if(handCount>1){p->drawCard(Sanguosha->getCard(0));room.setCardMapping(0,p,Player::Hand);}
  if(scenario!="none"){
   auto target=scenario=="self"?p:friendly;target->addDelayedTrick(delay);room.setCardMapping(delay->getId(),target,Player::Judging);
   if(scenario=="covered"){room.setCardMapping(177,nullptr,Player::DrawPile);target->addToPile("smile",177);}
  }
  for(auto player:room.getPlayers())player->setAI(room.cloneAI(player));
  QString script="assert(type(sgs.ai_skill_invoke.anxian)=='function' and type(sgs.ai_skill_cardask['@anxian-discard'])=='function'); local ai=sgs.ais.p0; ai.friends_noself={sgs.ais.p1.player}; ai.friends={ai.player,sgs.ais.p1.player}; local use={isDummy=true,to=sgs.SPlayerList()}; sgs.ai_skill_use_func.YanxiaoCard(sgs.Card_Parse('@YanxiaoCard=.'),use,ai); ";
  if(expected.isEmpty())script+="assert(not use.card, 'Do not waste a diamond when nobody needs protection')";
  else script+=QString("assert(use.card and use.to:length()==1 and use.to:first():objectName()=='%1', 'Yanxiao target must match the ally needing protection')").arg(expected);
  int rc=luaL_dostring(room.getLuaState(),qPrintable(script));QVERIFY2(rc==0,rc?lua_tostring(room.getLuaState(),-1):"");
 }
 void fuzuoSupportsFriend_data() {
  QTest::addColumn<bool>("helpFrom");QTest::addColumn<int>("aidNumber");QTest::addColumn<bool>("shouldHelp");QTest::addColumn<bool>("bothFriends");QTest::addColumn<bool>("committed");
  QTest::newRow("from-can-win")<<true<<6<<true<<false<<false;
  QTest::newRow("from-insufficient")<<true<<2<<false<<false<<false;
  QTest::newRow("to-tie-is-enough")<<false<<4<<true<<false<<false;
  QTest::newRow("illegal-eight")<<true<<8<<false<<false<<false;
  QTest::newRow("from-tie-not-enough")<<true<<4<<false<<false<<false;
  QTest::newRow("one-adds-zero")<<true<<1<<false<<false<<false;
  QTest::newRow("both-friends")<<true<<6<<false<<true<<false;
  QTest::newRow("already-committed")<<true<<7<<false<<false<<true;
 }
 void fuzuoSupportsFriend() {
  QFETCH(bool,helpFrom);QFETCH(int,aidNumber);QFETCH(bool,shouldHelp);QFETCH(bool,bothFriends);QFETCH(bool,committed);
  Config.EnableAI=true;Config.AIDelay=0;Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setGeneralName(i==0?"wiszhangzhao":i==1?"caocao":"liubei");p->setRole(i==0?"lord":"rebel");p->setState("robot");p->setMaxHp(4);p->setHp(4);}
  auto zz=room.getPlayers()[0];room.revivePlayer(zz);room.current=zz;
  const Card *aid=nullptr;for(int i=0;i<Sanguosha->getCardCount();++i)if(Sanguosha->getCard(i)->getNumber()==aidNumber){aid=Sanguosha->getCard(i);break;}QVERIFY(aid);zz->drawCard(aid);room.setCardMapping(aid->getId(),zz,Player::Hand);
  for(auto p:room.getPlayers())p->setAI(room.cloneAI(p));
  QString friendly=committed?"p0":helpFrom?"p1":"p2";QString enemy=helpFrom?"p2":"p1";
  QString script=QString("local ai=sgs.ais.p0; ai.isFriend=function(self,p) return p:objectName()=='%1' end; ai.isEnemy=function(self,p) return p:objectName()=='%2' end").arg(friendly,enemy);
  if(bothFriends)script+="; ai.isEnemy=function() return false end; ai.isFriend=function() return true end";
  QCOMPARE(luaL_dostring(room.getLuaState(),qPrintable(script)),0);
  const Card *from=nullptr,*to=nullptr;
  for(int i=0;i<Sanguosha->getCardCount();++i){auto c=Sanguosha->getCard(i);if(c->getNumber()==(helpFrom?7:9))from=c;if(c->getNumber()==(helpFrom?9:7))to=c;}
  QVERIFY(from);QVERIFY(to);if(committed)from=aid;
  PindianStruct pd;pd.from=committed?zz:room.getPlayers()[1];pd.to=room.getPlayers()[2];pd.from_card=from;pd.to_card=to;pd.reason="test";
  auto skill=Sanguosha->getTriggerSkill("fuzuo");QVERIFY(skill);QVariant data=QVariant::fromValue(static_cast<PindianStar>(&pd));skill->trigger(Pindian,&room,pd.from,data);
  QCOMPARE(zz->getHandcardNum(),shouldHelp?0:1);
  QCOMPARE(helpFrom?pd.isSuccess():!pd.isSuccess(),shouldHelp);
  if(pd.from_card!=from)delete pd.from_card;if(pd.to_card!=to)delete pd.to_card;
 }
 void xuanhuoObserverHandCount_data() {
  QTest::addColumn<bool>("shichou");QTest::addColumn<int>("equipmentOrder");
  QTest::newRow("xuanhuo")<<false<<0;
  QTest::newRow("shichou-bgm-liubei-noslingtong")<<true<<0;
  QTest::newRow("shichou-hand-equipment")<<true<<2;
  QTest::newRow("shichou-equipment-hand")<<true<<1;
 }
 void xuanhuoObserverHandCount() {
  QFETCH(bool,shichou);QFETCH(int,equipmentOrder);
  Config.EnableAI=true;Config.AIDelay=0;
  Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  auto observerSocket=new RecordingSocket;observerSocket->setParent(&room);
  auto observer=room.addSocket(observerSocket);
  auto lord=room.addSocket(nullptr);auto fazheng=room.addSocket(nullptr);
  observer->setObjectName("observer");lord->setObjectName("lord");fazheng->setObjectName("fazheng");
  for(auto p:room.getPlayers()){p->setGeneralName(p==fazheng?"fazheng":"caocao");p->setMaxHp(4);p->setHp(4);p->setRole(p==lord?"lord":"rebel");p->setState("robot");p->setAI(new XuanhuoGiveAI(p));}
  room.revivePlayer(lord);room.current=fazheng;
  observerSocket->messages.clear();
  if(shichou){
   lord->setGeneralName("bgm_liubei");lord->setGeneral2Name("noslingtong");lord->setPhase(Player::Start);
   if(equipmentOrder){
    const Card *equip=nullptr;
    for(int i=0;i<Sanguosha->getCardCount();++i)if(Sanguosha->getCard(i)->inherits("EquipCard")){equip=Sanguosha->getCard(i);break;}
    QVERIFY(equip);room.setCardMapping(equip->getId(),nullptr,Player::DrawPile);room.moveCardTo(equip,lord,Player::Equip,true);
    static_cast<XuanhuoGiveAI *>(lord->getAI())->equipFirst=equipmentOrder==1;
   }
   room.drawCards(lord,equipmentOrder?1:2);QVERIFY(lord->hasLordSkill("shichou"));
   auto skill=Sanguosha->getTriggerSkill("shichou");QVERIFY(skill);QVariant data;
   skill->trigger(PhaseChange,&room,lord,data);QCOMPARE(lord->getMark("hate"),1);
  }else{
   XuanhuoCard card;CardEffectStruct effect;effect.from=fazheng;effect.to=lord;effect.card=&card;card.onEffect(effect);
  }
  QCOMPARE(lord->getHandcardNum(),0);QCOMPARE(fazheng->getHandcardNum(),2);
  QTemporaryFile replay("/private/tmp/qs-sync-XXXXXX.txt");QVERIFY(replay.open());
  Client client(nullptr,replay.fileName());Self->setObjectName("observer");Self->setGeneralName("caocao");
  auto feed=[&](const QString &message){return QMetaObject::invokeMethod(&client,"processReply",Qt::DirectConnection,Q_ARG(QByteArray,message.toUtf8()));};
  QVERIFY(feed("addPlayer lord:bG9yZA==:caocao"));QVERIFY(feed("addPlayer fazheng:ZmF6aGVuZw==:fazheng"));
  QVERIFY(feed("setPileNumber 178"));
  for(const QString &message:observerSocket->messages)
   if(message.startsWith("drawNCards ")||message.startsWith("moveNCards ")||message.startsWith("moveCard ")||message.startsWith("pile ")){qInfo().noquote()<<message;QVERIFY(feed(message));}
  auto visibleLord=client.getPlayer("lord");visibleLord->setGeneralName("caocao");visibleLord->setHp(4);
  QCOMPARE(visibleLord->getHandcardNum(),lord->getHandcardNum());
  QCOMPARE(client.getPlayer("fazheng")->getHandcardNum(),fazheng->getHandcardNum());
  QVERIFY(!visibleLord->hasEquip());
  // Count-only notifications must not reveal the transferred hidden hand cards.
  QCOMPARE(client.getPlayer("fazheng")->getCards().size(),equipmentOrder==2?1:0);
  PlayerCardDialog dialog(visibleLord,"hej");
  auto button=dialog.findChild<QCommandLinkButton *>("handcard_button");QVERIFY(button);QVERIFY(!button->isEnabled());
  Self=nullptr;ClientInstance=nullptr;
 }
 void oldXuanfengChoosesEnemy_data() {
  QTest::addColumn<QString>("action");QTest::newRow("damage")<<QString("damage");QTest::newRow("slash")<<QString("slash");QTest::newRow("friends-only")<<QString("nothing");
 }
 void oldXuanfengChoosesEnemy() {
  QFETCH(QString,action);Config.EnableAI=true;Config.AIDelay=0;Config.Enable2ndGeneral=true;
  Room room(nullptr,"04p");room.thread=new RoomThread(&room);
  for(int i=0;i<4;++i){auto p=room.addSocket(nullptr);p->setObjectName(QString("p%1").arg(i));p->setRole(i==0?"lord":i==3?"rebel":"loyalist");p->setState("robot");p->setGeneralName(i==0?"bgm_liubei":"caocao");p->setMaxHp(4);p->setHp(4);}
  auto lord=room.getPlayers()[0];lord->setGeneral2Name("noslingtong");room.revivePlayer(lord);room.current=lord;
  for(auto p:room.getPlayers())p->setAI(room.cloneAI(p));
  // Pin perception and make the missing-handler random fallback choose the ally.
  QString lua="local ai=sgs.ais.p0; ai.isEnemy=function(self,p) return p:objectName()=='p3' end; ai.isFriend=function(self,p) return p:objectName()~='p3' end; ai.enemies={sgs.ais.p3.player}; math.random=function(a,b) return a end";
  if(action=="nothing")lua+="; ai.enemies={}; ai.isEnemy=function() return false end";
  if(action=="slash")lua+="; sgs.ai_skill_choice.nosxuanfeng=function() return 'slash' end";
  QCOMPARE(luaL_dostring(room.getLuaState(),qPrintable(lua)),0);
  ChoiceCapture capture;room.thread->addTriggerSkill(&capture);
  const auto skill=Sanguosha->getTriggerSkill("nosxuanfeng");QVERIFY(skill);
  lord->tag["InvokeNosXuanfeng"]=true;QVariant data;skill->trigger(CardLostDone,&room,lord,data);
  if(action=="nothing")QVERIFY(capture.chosen.isEmpty());
  else QVERIFY2(capture.chosen.endsWith(":p3"),qPrintable(capture.chosen));
 }
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
