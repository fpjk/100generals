#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"
#include "sp.h"
#include "wind.h"
#include "maneuvering.h"
#include "json.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "util.h"
#include "wrapped-card.h"
#include "roomthread.h"

class Jianxiong : public MasochismSkill
{
public:
    Jianxiong() : MasochismSkill("jianxiong")
    {
    }

    void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            QVariant data = QVariant::fromValue(damage);
            QStringList choices;
            choices << "draw" << "cancel";

            const Card *card = damage.card;
            if (card) {
                QList<int> ids;
                if (card->isVirtualCard())
                    ids = card->getSubcards();
                else
                    ids << card->getEffectiveId();
                if (ids.length() > 0) {
                    bool all_place_table = true;
                    foreach (int id, ids) {
                        if (room->getCardPlace(id) != Player::PlaceTable) {
                            all_place_table = false;
                            break;
                        }
                    }
                    if (all_place_table) choices.append("obtain");
                }
            }

            QString choice = room->askForChoice(caocao, objectName(), choices.join("+"), data);
            if (choice != "cancel") {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = caocao;
                log.arg = objectName();
                room->sendLog(log);

                room->notifySkillInvoked(caocao, objectName());
                room->broadcastSkillInvoke(objectName());
                if (choice == "obtain")
                    caocao->obtainCard(card);
                else
                    caocao->drawCards(1, objectName());
            }
        }
    }
};

class Baye : public TriggerSkill
{
public:
    Baye() : TriggerSkill("baye")
    {
        events << EventPhaseChanging;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !player->isSkipped(Player::Judge) && player->getHandcardNum() > player->getHp() && !player->getJudgingArea().isEmpty())
        {
            if (!room->askForSkillInvoke(player, objectName(), data))
                return false;
            room->broadcastSkillInvoke(objectName());
            player->skip(Player::Judge, true);
        }
        
        return false;
    }
};

class Hujia : public TriggerSkill
{
public:
    Hujia() : TriggerSkill("hujia$")
    {
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasLordSkill("hujia");
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *caocao, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);
        if (pattern != "jink" || prompt.startsWith("@hujia-jink"))
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("wei", caocao);
        if (lieges.isEmpty())
            return false;

        if (!room->askForSkillInvoke(caocao, objectName(), data))
            return false;

        room->broadcastSkillInvoke(objectName());
        QVariant tohelp = QVariant::fromValue(caocao);
        foreach (ServerPlayer *liege, lieges) {
            const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + caocao->objectName(),
                tohelp, Card::MethodResponse, caocao, false, QString(), true);
            if (jink) {
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class Guiming : public RetrialSkill
{
public:
    Guiming() : RetrialSkill("guiming")
    {

    }

    const Card *onRetrial(ServerPlayer *player, JudgeStruct *judge) const
    {
        if (player->isNude())
            return NULL;

        QStringList prompt_list;
        prompt_list << "@guiming-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        Room *room = player->getRoom();

        const Card *card = room->askForCard(player, "..", prompt, QVariant::fromValue(judge), Card::MethodResponse, judge->who, true);

        if (card) {
            room->broadcastSkillInvoke(objectName());
        }

        return card;
    }
};

class Yangshi : public MasochismSkill
{
public:
    Yangshi() : MasochismSkill("yangshi")
    {
    }
    
    void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const
    {
        Room *room = simayi->getRoom();    
        ServerPlayer *from = damage.from;
        ServerPlayer *current = room->getCurrent();    
        for (int i = 0; i < damage.damage; i++) {
            QList<ServerPlayer *> targets;
            if (from && !from->isNude())
                targets << from;
            if (current && !current->isNude())
                targets << current;
            QVariant data = QVariant::fromValue(from);
            if (!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(simayi, targets, "yangshi", "@yangshi", true);
                room->broadcastSkillInvoke(objectName());
                int card_id = room->askForCardChosen(simayi, target, "he", "yangshi");
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
                Card *card = Sanguosha->getCard(card_id);
                room->obtainCard(simayi, card, reason, room->getCardPlace(card_id) != Player::PlaceHand);
                QList<ServerPlayer *> givees;
                foreach(ServerPlayer *p, room->getOtherPlayers(simayi))
                {
                    if (p->isMale())
                        givees << p;
                }
                ServerPlayer *givee = room->askForPlayerChosen(simayi, givees, "yangshigive", "@yangshi-give", true);
                if (givee != NULL) {
                    CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, simayi->objectName(), givee->objectName(), "yangshi", QString());
                    room->obtainCard(givee, card, reason2, false);
                }
            } else {
                break;
            }
        }
    }
};

class Ganglie : public TriggerSkill
{
public:
    Ganglie() : TriggerSkill("ganglie")
    {
        events << Damaged << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data) const
    {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(xiahou)) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;

            for (int i = 0; i < damage.damage; i++) {
                if (room->askForSkillInvoke(xiahou, "ganglie", data)) {
                    room->broadcastSkillInvoke(objectName());

                    JudgeStruct judge;
                    judge.pattern = ".";
                    judge.play_animation = false;
                    judge.reason = objectName();
                    judge.who = xiahou;

                    room->judge(judge);
                    if (!from || from->isDead()) continue;
                    Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
                    switch (suit) {
                    case Card::Heart:
                    case Card::Diamond: {
                        room->damage(DamageStruct(objectName(), xiahou, from));
                        break;
                    }
                    case Card::Club:
                    case Card::Spade: {
                        if (xiahou->canDiscard(from, "he")) {
                            int id = room->askForCardChosen(xiahou, from, "he", objectName(), false, Card::MethodDiscard);
                            room->throwCard(id, from, xiahou);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                } else {
                    break;
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return false;
    }
};

class Qingjian : public TriggerSkill
{
public:
    Qingjian() : TriggerSkill("qingjian")
    {
        events << CardsMoveOneTime;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw
            && move.to == player && move.to_place == Player::PlaceHand) {
            int n = 0;
            foreach (int id, move.card_ids) {
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                    n++;
            }
            if (n == 0)
                return false;
            player->tag["QingjianCurrentMoveSkill"] = QVariant(move.reason.m_skillName);
            while(n > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "qingjian-invoke", true, true);
                if (!target)
                    break;
                else {
                    const Card *to_give = room->askForExchange(player, objectName(), n, 1, true,
                        QString("@qingjian-give::%1:%2").arg(target->objectName()).arg(n));
                    n -= to_give->subcardsLength();
                    target->addToPile("ji", to_give);
                    delete to_give;
                }
                if (player->isDead()) 
                    break;
            }
            /* QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                    ids << id;
            }
            if (ids.isEmpty())
                return false;
            player->tag["QingjianCurrentMoveSkill"] = QVariant(move.reason.m_skillName);
            while (room->askForYiji(player, ids, objectName(), false, false, true, -1,
                QList<ServerPlayer *>(), CardMoveReason(), "@qingjian-distribute", true)) {
                if (player->isDead()) return false;
            } */
        }
        return false;
    }
};

class TuxiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    TuxiViewAsSkill() : ZeroCardViewAsSkill("tuxi")
    {
        response_pattern = "@@tuxi";
    }

    const Card *viewAs() const
    {
        return new TuxiCard;
    }
};

class Tuxi : public DrawCardsSkill
{
public:
    Tuxi() : DrawCardsSkill("tuxi")
    {
        view_as_skill = new TuxiViewAsSkill;
    }

    int getPriority(TriggerEvent) const
    {
        return 1;
    }

    int getDrawNum(ServerPlayer *zhangliao, int n) const
    {
        Room *room = zhangliao->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), n);
        foreach(ServerPlayer *p, room->getOtherPlayers(zhangliao))
            p->setFlags("-TuxiTarget");

        if (num > 0) {
            room->setPlayerMark(zhangliao, "tuxi", num);
            int count = 0;
            if (room->askForUseCard(zhangliao, "@@tuxi", "@tuxi-card:::" + QString::number(num))) {
                foreach(ServerPlayer *p, room->getOtherPlayers(zhangliao))
                    if (p->hasFlag("TuxiTarget")) count++;
            } else {
                room->setPlayerMark(zhangliao, "tuxi", 0);
            }
            return n - count;
        } else
            return n;
    }
};

class TuxiAct : public TriggerSkill
{
public:
    TuxiAct() : TriggerSkill("#tuxi")
    {
        events << AfterDrawNCards;
    }

    bool triggerable(const ServerPlayer *player) const
    {
        return player != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &) const
    {
        if (zhangliao->getMark("tuxi") == 0) return false;
        room->setPlayerMark(zhangliao, "tuxi", 0);

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao)) {
            if (p->hasFlag("TuxiTarget")) {
                p->setFlags("-TuxiTarget");
                targets << p;
            }
        }
        foreach (ServerPlayer *p, targets) {
            if (!zhangliao->isAlive())
                break;
            if (p->isAlive() && !p->isKongcheng()) {
                int card_id = room->askForCardChosen(zhangliao, p, "h", "tuxi");

                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhangliao->objectName());
                room->obtainCard(zhangliao, Sanguosha->getCard(card_id), reason, false);
            }
        }
        return false;
    }
};

class LuoyiBuff : public TriggerSkill
{
public:
    LuoyiBuff() : TriggerSkill("#luoyi")
    {
        events << DamageCaused;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasFlag("luoyi") && target->isAlive();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Luoyi : public TriggerSkill
{
public:
    Luoyi() : TriggerSkill("luoyi")
    {
        events << EventPhaseStart;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw && room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
            player->skip(Player::Draw, true);
            room->setPlayerFlag(player, "luoyi");

            QList<int> ids = room->getNCards(3, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "luoyi", QString()));
            room->moveCardsAtomic(move, true);

            room->getThread()->delay();
            room->getThread()->delay();

            QList<int> card_to_throw;
            QList<int> card_to_gotback;
            for (int i = 0; i < 3; i++) {
                const Card *card = Sanguosha->getCard(ids[i]);
                if (card->getTypeId() == Card::TypeBasic || card->isKindOf("Weapon") || card->isKindOf("Duel"))
                    card_to_gotback << ids[i];
                else
                    card_to_throw << ids[i];
            }
            if (!card_to_throw.isEmpty()) {
                DummyCard *dummy = new DummyCard(card_to_throw);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "luoyi", QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
            }
            if (!card_to_gotback.isEmpty()) {
                DummyCard *dummy = new DummyCard(card_to_gotback);
                room->obtainCard(player, dummy);
                delete dummy;
            }
            
            return true;
        }
        return false;
    }
};

class Tiandu : public TriggerSkill
{
public:
    Tiandu() : TriggerSkill("tiandu")
    {
        events << EventPhaseStart;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Judge) return false;
        if (player->getJudgingArea().isEmpty()) return false;
        foreach (ServerPlayer *guojia, room->getOtherPlayers(player)) {
            if(TriggerSkill::triggerable(guojia) && room->askForSkillInvoke(guojia, objectName()) && guojia->canDiscard(guojia, "e") && guojia->getEquips().length() != 0){
                room->broadcastSkillInvoke(objectName());
                /*int card_id = room->askForCardChosen(guojia, player, "j", objectName());
                player->addToPile("tiandu_judge",card_id);*/
                DummyCard *dummy = new DummyCard;
                foreach (const Card *card, guojia->getCards("e")) {
                        dummy->addSubcard(card);
                }
                room->throwCard(dummy, guojia);
                delete dummy;
                room->damage(DamageStruct(objectName(), NULL, guojia, 1, DamageStruct::Thunder));
                return true;
            }
        }
        return false;
    }
};

/*class TianduGot : public TriggerSkill
{
public:
    TianduGot() : TriggerSkill("#tiandu-got")
    {
        events << EventPhaseEnd;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Judge || player->getPile("tiandu_judge").length() == 0) return false;
        foreach (int card_id, player->getPile("tiandu_judge")) {
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(card_id, player, Player::PlaceDelayedTrick, CardMoveReason(CardMoveReason::S_REASON_PUT, player->objectName()));
            exchangeMove.push_back(move1);
            room->moveCardsAtomic(exchangeMove,true);
        }
        return false;
    }
};*/

class YijiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    YijiViewAsSkill() : ZeroCardViewAsSkill("yiji")
    {
        response_pattern = "@@yiji";
    }

    const Card *viewAs() const
    {
        return new YijiCard;
    }
};

class Yiji : public MasochismSkill
{
public:
    Yiji() : MasochismSkill("yiji")
    {
        view_as_skill = new YijiViewAsSkill;
        frequency = Frequent;
    }

    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            if (target->isAlive() && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName());
                target->drawCards(2, objectName());
                room->askForUseCard(target, "@@yiji", "@yiji");
            } else {
                break;
            }
        }
    }
};

class Luoshen : public TriggerSkill
{
public:
    Luoshen() : TriggerSkill("luoshen")
    {
        events << EventPhaseStart << FinishJudge;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhenji, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && zhenji->getPhase() == Player::Start) {
            bool canRetrial = zhenji->hasSkills("guiming|nosguiming|guidao|huanshi");
            bool first = true;
            while (zhenji->askForSkillInvoke("luoshen")) {
                if (first) {
                    room->broadcastSkillInvoke(objectName());
                    first = false;
                }

                JudgeStruct judge;
                judge.pattern = ".|black";
                judge.good = true;
                judge.reason = objectName();
                judge.play_animation = false;
                judge.who = zhenji;
                judge.time_consuming = true;

                if (canRetrial)
                    zhenji->setFlags("LuoshenRetrial");
                try {
                    room->judge(judge);
                }
                catch (TriggerEvent triggerEvent) {
                    if ((triggerEvent == TurnBroken || triggerEvent == StageChange) && zhenji->hasFlag("LuoshenRetrial"))
                        zhenji->setFlags("-LuoshenRetrial");
                    throw triggerEvent;
                }

                if (judge.isBad())
                    break;
            }
            if (canRetrial && zhenji->tag.contains(objectName())) {
                DummyCard *dummy = new DummyCard(VariantList2IntList(zhenji->tag[objectName()].toList()));
                if (dummy->subcardsLength() > 0)
                    zhenji->obtainCard(dummy);
                zhenji->tag.remove(objectName());
                delete dummy;
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                bool canRetrial = zhenji->hasFlag("LuoshenRetrial");
                if (judge->card->isBlack()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        if (canRetrial) {
                            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, zhenji->objectName(), QString(), judge->reason);
                            room->moveCardTo(judge->card, zhenji, NULL, Player::PlaceTable, reason, true);
                            QVariantList luoshen_list = zhenji->tag[objectName()].toList();
                            luoshen_list << judge->card->getEffectiveId();
                            zhenji->tag[objectName()] = luoshen_list;
                        } else {
                            zhenji->obtainCard(judge->card);
                        }
                    }
                } else {
                    if (canRetrial) {
                        DummyCard *dummy = new DummyCard(VariantList2IntList(zhenji->tag[objectName()].toList()));
                        if (dummy->subcardsLength() > 0)
                            zhenji->obtainCard(dummy);
                        zhenji->tag.remove(objectName());
                        delete dummy;
                    }
                }
            }
        }

        return false;
    }
};

class Qingguo : public OneCardViewAsSkill
{
public:
    Qingguo() : OneCardViewAsSkill("qingguo")
    {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }
};

class Quhu : public ZeroCardViewAsSkill
{
public:
    Quhu() : ZeroCardViewAsSkill("quhu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QuhuCard") && !player->isKongcheng();
    }

    const Card *viewAs() const
    {
        return new QuhuCard;
    }
};

class Jieming : public MasochismSkill
{
public:
    Jieming() : MasochismSkill("jieming")
    {
    }

    void onDamaged(ServerPlayer *xunyu, const DamageStruct &damage) const
    {
        Room *room = xunyu->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAlivePlayers())
            if (p->getHandcardNum() <= p->getHp() && damage.from != p) 
                targets << p;
            
            ServerPlayer *to = room->askForPlayerChosen(xunyu, targets, objectName(), "jieming-invoke", true, true);
            if (!to) break;

            room->broadcastSkillInvoke(objectName());
            to->drawCards(2, objectName());
            if (!xunyu->isAlive())
                break;
        }
    }
};

class RendeViewAsSkill : public ViewAsSkill
{
public:
    RendeViewAsSkill() : ViewAsSkill("rende")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("rende") >= 2)
            return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@rende") {
                QList<int> rende_list = StringList2IntList(Self->property("rende").toString().split("+"));
                return rende_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("rende") >= 2)
            return false;
        return !player->hasUsed("RendeCard") && !player->isKongcheng();
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@rende";
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Rende : public TriggerSkill
{
public:
    Rende() : TriggerSkill("rende")
    {
        events << EventPhaseChanging;
        view_as_skill = new RendeViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("rende") > 0;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "rende", 0);
        room->setPlayerProperty(player, "rende", QString());
        return false;
    }
};

class Jijiang : public ZeroCardViewAsSkill
{
public:
    Jijiang() : ZeroCardViewAsSkill("jijiang$")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JijiangCard");
    }

    const Card *viewAs() const
    {
        return new JijiangCard;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill("jijiang") && player->getMark("ruoyu") > 0)
            index += 2;

        return index;
    }
};

class Wusheng : public OneCardViewAsSkill
{
public:
    Wusheng() : OneCardViewAsSkill("wusheng")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    bool viewFilter(const Card *card) const
    {
        if (!card->isRed())
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class YijueViewAsSkill : public ZeroCardViewAsSkill
{
public:
    YijueViewAsSkill() : ZeroCardViewAsSkill("yijue")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YijueCard") && !player->isKongcheng();
    }

    const Card *viewAs() const
    {
        return new YijueCard;
    }
};

class Yijue : public TriggerSkill
{
public:
    Yijue() : TriggerSkill("yijue")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new YijueViewAsSkill;
    }

    int getPriority(TriggerEvent) const
    {
        return 5;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return false;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            if (player->getMark("yijue") == 0) continue;
            player->removeMark("yijue");
            room->removeFixedDistance(target, player, 1);

            /*foreach(ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);*/

            room->removePlayerCardLimitation(player, "use,response", ".|.|.|hand$1");
        }
        return false;
    }
};

class NonCompulsoryInvalidity : public InvaliditySkill
{
public:
    NonCompulsoryInvalidity() : InvaliditySkill("#non-compulsory-invalidity")
    {
    }

    bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return player->getMark("@skill_invalidity") == 0 || skill->getFrequency(player) == Skill::Compulsory || skill->isAttachedLordSkill();
    }
};

class Paoxiao : public TargetModSkill
{
public:
    Paoxiao() : TargetModSkill("paoxiao")
    {
        frequency = NotCompulsory;
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};

class Mangzhi : public TriggerSkill
{
public:
    Mangzhi() : TriggerSkill("mangzhi")
    {
        events << TargetSpecified;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash")) 
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName());
                    QString choice = room->askForChoice(p, objectName(), "haveslash+noslash", QVariant::fromValue(p));
                    bool guess = (choice == "haveslash");

                    LogMessage log;
                    log.from = p;
                    log.type = guess ? "#MangzhiSlash" : "#MangzhiNoSlash";
                    room->sendLog(log);

                    bool have = false;
                    if (!player->isKongcheng()){
                        room->showAllCards(player);
                        QList<const Card *> cards = player->getHandcards();
                        foreach (const Card *card, cards) {
                            if (card->isKindOf("Slash")) {
                                have = true;
                                break;
                            }
                        }
                    }
                    if (guess != have){
                        if (!p->isNude()){
                            int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                            room->throwCard(disc, p, player);
                        }
                        player->drawCards(1);
                    }
                }
            }
        }
        return false;
    }
};

class Guanxing : public PhaseChangeSkill
{
public:
    Guanxing() : PhaseChangeSkill("guanxing")
    {
        frequency = Frequent;
    }

    int getPriority(TriggerEvent) const
    {
        return 1;
    }

    bool onPhaseChange(ServerPlayer *zhuge) const
    {
        if (zhuge->getPhase() == Player::Start && zhuge->askForSkillInvoke(this)) {
            Room *room = zhuge->getRoom();
            int index = qrand() % 2 + 1;
            if (objectName() == "guanxing" && !zhuge->hasInnateSkill(this) && zhuge->hasSkill("zhiji"))
                index += 2;
            room->broadcastSkillInvoke(objectName(), index);
            QList<int> guanxing = room->getNCards(getGuanxingNum(room));

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = zhuge;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->sendLog(log, zhuge);

            room->askForGuanxing(zhuge, guanxing);
        }

        return false;
    }

    int getGuanxingNum(Room *room) const
    {
        return qMin(5, room->alivePlayerCount());
    }
};

class Kongcheng : public TriggerSkill
{
public:
    Kongcheng() : TriggerSkill("kongcheng")
    {
        events << BeforeCardsMove << CardsMoveOneTime << EventPhaseStart ;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == BeforeCardsMove || triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player) return false;

            if (triggerEvent == BeforeCardsMove) {
                if (move.from_places.contains(Player::PlaceSpecial) && move.from_pile_names.contains("ji")) {
                    int reason = move.reason.m_reason;
                    int basic_reason = reason & CardMoveReason::S_MASK_BASIC_REASON;
                    if (player->getPhase() == Player::NotActive && (basic_reason == CardMoveReason::S_REASON_USE || basic_reason == CardMoveReason::S_REASON_RESPONSE)) {
                        player->addMark(objectName());
                    }
                    if (reason == CardMoveReason::S_REASON_EXCHANGE_FROM_PILE) {
                        room->sendCompulsoryTriggerLog(player, objectName());
                        QList<int> ids = player->getPile("ji");
                        room->fillAG(ids, player);
                        int id = room->askForAG(player, ids, false, objectName());
                        room->clearAG(player);
                        room->throwCard(id, NULL);
                        QList<int> remove_ids;
                        remove_ids << id;
                        move.removeCardIds(remove_ids);
                        data = QVariant::fromValue(move);
                    }
                }
            } else {
                int n = player->getMark(objectName());
                for (int i = 0; i < n; i++) {
                    player->removeMark(objectName());
                    if (player->isAlive() && player->askForSkillInvoke(this, data)) {
                        player->drawCards(2, objectName());
                    } else {
                        break;
                    }
                }
                player->setMark(objectName(), 0);
            }
        }
        else {
            if (player->getPhase() == Player::Discard) {
                if (player->isKongcheng()) return false;
                QString pattern = ".|.|.|hand";
                const Card *card = room->askForCard(player, pattern, "@kongcheng", QVariant(), Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    room->broadcastSkillInvoke(objectName());

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    player->addToPile("ji", card, false);
                }
            }
        }
        return false;
    }
};

class Longwei : public TriggerSkill
{
public:
    Longwei() : TriggerSkill("longwei")
    {
        events << CardAsked ;
    }
    
    bool triggerable(const ServerPlayer *player) const
    {
        return player && player->isAlive() && player->getPhase() == Player::NotActive ;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer *zhaoyun = room->findPlayerBySkillName(objectName());
        if (!zhaoyun || !zhaoyun->isAlive() || player == zhaoyun || !zhaoyun->inMyAttackRange(player))
            return false;
        QString pattern = data.toStringList().first();
        if ((pattern == "slash" || pattern == "jink") && room->askForSkillInvoke(player, objectName(), data)) {
            const Card *card = room->askForCard(zhaoyun, pattern, "@longwei-card:" + player->objectName(), QVariant(), Card::MethodResponse, player);
            if (card){
                bool will_dis = room->askForSkillInvoke(zhaoyun, "longwei_dis", "discard");
                if (will_dis) { 
                    ServerPlayer *current = room->getCurrent();
                    int disc = room->askForCardChosen(zhaoyun, current, "he", "longwei", false, Card::MethodDiscard);
                    room->throwCard(disc, current, zhaoyun);
                }
                room->provide(card);
                return true;
            }
            room->setPlayerFlag(player, "Global_longweiFailed");
        }
        return false;
    }
};

class LongweiAttach : public TriggerSkill
{
public:
    LongweiAttach() : TriggerSkill("#longwei-attach")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Debut;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == GameStart && TriggerSkill::triggerable(player))
            || (triggerEvent == EventAcquireSkill && data.toString() == "longwei")) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->hasSkill("longwei_VS"))
                    room->attachSkillToPlayer(p, "longwei_VS");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "longwei") {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasSkill("longwei_VS"))
                    room->detachSkillFromPlayer(p, "longwei_VS", true);
            }
        } else if (triggerEvent == Debut) {
            QList<ServerPlayer *> zhaoyuns = room->findPlayersBySkillName("longwei");
            foreach (ServerPlayer *zhaoyun, zhaoyuns) {
                if (player != zhaoyun && !player->hasSkill("longwei-attach")) {
                    room->attachSkillToPlayer(player, "longwei-attach");
                    break;
                }
            }
        }
        return false;
    }
};

class LongweiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    LongweiViewAsSkill() : ZeroCardViewAsSkill("longwei_VS")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        const Player *zhaoyun = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("longwei") && p->inMyAttackRange(player)) {
                zhaoyun = p;
                break;
            }
        }
        if (zhaoyun == NULL || player->hasFlag("Global_longweiFailed") || player->getPhase() != Player::NotActive) return false;
        if (pattern == "slash")
            return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
        else if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    const Card *viewAs() const
    {
        LongweiCard *longwei_card = new LongweiCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        longwei_card->setUserString(pattern);
        return longwei_card;
    }
};

/*class Yajiao : public TriggerSkill
{
public:
    Yajiao() : TriggerSkill("yajiao")
    {
        events << CardUsed << CardResponded;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() != Player::NotActive) return false;
        const Card *cardstar = NULL;
        bool isBasic = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
            isBasic = (cardstar->getTypeId() == Card::TypeBasic);
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
            isBasic = (cardstar->getTypeId() == Card::TypeBasic);
        }
        if (isBasic && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "yajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            room->fillAG(ids, player);
            bool dealt = false;
            if (card->getTypeId() == cardstar->getTypeId()) {
                QVariant carddata = QVariant::fromValue(card);
                if (room->askForChoice(player, objectName(), "draw+cancel", carddata) == "draw") {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, player->objectName(), "yajiao", QString());
                    room->obtainCard(player, card, reason);
                }
            } else {
                QVariant carddata = QVariant::fromValue(card);
                if (room->askForChoice(player, objectName(), "throw+cancel", carddata) == "throw") {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "yajiao", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
            if (!dealt) {
                room->clearAG(player);
                room->returnToTopDrawPile(ids);
            }
        }
        return false;
    }
};*/

class Longhun : public OneCardViewAsSkill
{
public:
    Longhun() : OneCardViewAsSkill("longhun")
    {
        response_or_use = true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive) return false;
        return pattern == "slash" && player->getHp() <= 4
            || pattern == "jink" && player->getHp() <= 2
            || (pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0) && player->getHp() <= 3
            || pattern == "nullification" && player->getHp() <= 1;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool viewFilter(const Card *to_select) const
    {
        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {

        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink")
                return to_select->getSuit() == Card::Club;
            else if (pattern == "nullification")
                return to_select->getSuit() == Card::Spade;
            else if (pattern == "peach" || pattern == "peach+analeptic")
                return to_select->getSuit() == Card::Heart;
            else if (pattern == "slash")
                return to_select->getSuit() == Card::Diamond;
        }
        default:
            break;
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *new_card = NULL;

        switch (originalCard->getSuit()) {
        case Card::Spade: {
            if (Self->getHp() > 1) break;
            new_card = new Nullification(Card::SuitToBeDecided, 0);
            break;
        }
        case Card::Heart: {
            if (Self->getHp() > 3) break;
            new_card = new Peach(Card::SuitToBeDecided, 0);
            break;
        }
        case Card::Club: {
            if (Self->getHp() > 2) break;
            new_card = new Jink(Card::SuitToBeDecided, 0);
            break;
        }
        case Card::Diamond: {
            if (Self->getHp() > 4) break;
            new_card = new FireSlash(Card::SuitToBeDecided, 0);
            break;
        }
        default:
            break;
        }

        if (new_card) {
            new_card->setSkillName(objectName());
            new_card->addSubcard(originalCard->getId());
        }

        return new_card;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *card) const
    {
        return static_cast<int>(player->getRoom()->getCard(card->getSubcards().first())->getSuit()) + 1;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->getHp() <= 1 && (!player->isKongcheng() || !player->getHandPile().isEmpty());
    }
};

class Mashu : public DistanceSkill
{
public:
    Mashu() : DistanceSkill("mashu")
    {
    }

    int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this))
            return -1;
        else
            return 0;
    }
};

class Tieji : public TriggerSkill
{
public:
    Tieji() : TriggerSkill("tieji")
    {
        events << TargetSpecified;// << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            QList<ServerPlayer *> tos;
            foreach (ServerPlayer *p, use.to) {
                if (!player->isAlive()) break;
                if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName());
                    
                    player->drawCards(1, objectName());

                    /*JudgeStruct judge;
                    judge.pattern = ".";
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;
                    judge.play_animation = false;

                    room->judge(judge);*/
                    if(!player->isNude())
                    {
                        const Card *c = room->askForCard(player, "..!", "@tieji");
                        if (c == NULL) {
                            c = player->getCards("he").at(qrand() % player->getCardCount());
                            room->throwCard(c, player);
                        }
                        
                        QString pattern = (c->isBlack()) ? "black" : "red";
                        
                        if ((p->isAlive() && !p->canDiscard(p, "he"))
                            || !room->askForCard(p, ".|" + pattern, "@tieji-discard:::" + pattern, data, Card::MethodDiscard)) {
                            LogMessage log;
                            log.type = "#NoJink";
                            log.from = p;
                            room->sendLog(log);
                            jink_list.replace(index, QVariant(0));
                            if (!tos.contains(p)) {
                                p->addMark("tieji");
                                room->addPlayerMark(p, "@skill_invalidity");
                                tos << p;

                                foreach(ServerPlayer *pl, room->getAllPlayers())
                                    room->filterCards(pl, pl->getCards("he"), true);
                                JsonArray args;
                                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                            }
                        }
                    }
                }
                index++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            return false;
        }/* else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = judge->card->getSuitString();
            }
        }*/
        return false;
    }
};

class TiejiClear : public TriggerSkill
{
public:
    TiejiClear() : TriggerSkill("#tieji-clear")
    {
        events << EventPhaseChanging << Death;
    }

    int getPriority(TriggerEvent) const
    {
        return 5;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return false;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            if (player->getMark("tieji") == 0) continue;
            room->removePlayerMark(player, "@skill_invalidity", player->getMark("tieji"));
            player->setMark("tieji", 0);

            foreach(ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return false;
    }
};

class Qicai : public TriggerSkill
{
public:
    Qicai() : TriggerSkill("qicai")
    {
        frequency = Frequent;
        events << CardUsed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->getTypeId() == Card::TypeTrick && room->askForSkillInvoke(yueying, objectName())) {
            room->broadcastSkillInvoke(objectName());
            yueying->drawCards(1, objectName());
            if(!yueying->isNude()){
                ServerPlayer *target = room->askForPlayerChosen(yueying, (room->alivePlayerCount() >=4 )?room->getOtherPlayers(yueying):room->getAlivePlayers(), objectName(), "qicai-invoke", false, true);
                const Card *dummy = room->askForExchange(yueying, "qicai", 1, 1, false, "QicaiGive");
                target->addToPile("ji", dummy, false);
                delete dummy;
            }
        }
        return false;
    }
};

class Cangji : public TriggerSkill
{
public:
    Cangji() : TriggerSkill("cangji")
    {
        events << BeforeCardsMove << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(objectName());
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
        if (triggerEvent == BeforeCardsMove) {
            if (yueying->getPhase() != Player::NotActive) return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            int reasonx = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
            QList<int> card_ids;
            if (move.from && (move.from == yueying) && reasonx == CardMoveReason::S_REASON_DISCARD) {
                int i = 0;
                foreach(int card_id, move.card_ids) {
                    if ((yueying->getWeapon() && card_id == yueying->getWeapon()->getEffectiveId()) || (yueying->getArmor() && card_id == yueying->getArmor()->getEffectiveId()) || (yueying->getTreasure() && card_id == yueying->getTreasure()->getEffectiveId()))
                        card_ids.append(card_id);
                    i++;
                }
                if (!card_ids.isEmpty() && room->askForSkillInvoke(yueying, objectName())) {
                    room->notifySkillInvoked(yueying, objectName());
                    room->broadcastSkillInvoke("cangji", 1);

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = yueying;
                    log.arg = objectName();
                    room->sendLog(log);

                    move.removeCardIds(card_ids);
                    data = QVariant::fromValue(move);
                }
            }
        }
        else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != yueying)
                return false;
            if (yueying->getEquips().length() == 0)
                return false;
            QList<int> cangji_card;
            foreach(const Card *equip, yueying->getEquips())
                cangji_card.append(equip->getId());

            while (room->askForYiji(yueying, cangji_card, objectName(), false, true, true, -1));
        }
        return false;
    }
};

class Zhue : public TriggerSkill
{
public:
    Zhue() : TriggerSkill("zhue")
    {
        events << EventPhaseStart << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            ServerPlayer *xushu = room->findPlayerBySkillName(objectName());
            if (xushu && xushu != player && xushu->canSlash(player, false) && player->getMark("damage_point_round") > 0) {
                xushu->setFlags("ZhueSlash");
                QString prompt = QString("@zhue-slash:%1:%2").arg(xushu->objectName()).arg(player->objectName());
                if (!room->askForUseSlashTo(xushu, player, prompt, false))
                    xushu->setFlags("-ZhueSlash");
            }
        } else if (triggerEvent == ChoiceMade && player->hasFlag("ZhueSlash") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->setFlags("-ZhueSlash");
        }
        return false;
    }
};

class Qianxin : public TriggerSkill
{
public:
    Qianxin() : TriggerSkill("qianxin")
    {
        events << Damage;
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target)
            && target->getMark("qianxin") == 0
            && target->isWounded();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        //room->doLightbox("$QianxinAnimate");

        room->doSuperLightbox("xushu", "qianxin");

        LogMessage log;
        log.type = "#QianxinWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->setPlayerMark(player, "qianxin", 1);
        if (room->changeMaxHpForAwakenSkill(player)) {
            if (room->askForChoice(player, objectName(), "recover+draw") == "recover")
                room->recover(player, RecoverStruct(player));
            else
                room->drawCards(player, 2, objectName());
            if (player->getMark("qianxin") == 1)
                room->acquireSkill(player, "jianyan");
        }
        return false;
    }
};

class Jianyan : public ZeroCardViewAsSkill
{
public:
    Jianyan() : ZeroCardViewAsSkill("jianyan")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JianyanCard");
    }

    const Card *viewAs() const
    {
        return new JianyanCard;
    }
};

class Zhiheng : public ViewAsSkill
{
public:
    Zhiheng() : ViewAsSkill("zhiheng")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() > Self->getAliveSiblings().length()) return false;
        return !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);
        zhiheng_card->setSkillName(objectName());
        return zhiheng_card;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ZhihengCard");
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@zhiheng";
    }
};

class Zuoli : public TriggerSkill
{
public:
    Zuoli() : TriggerSkill("zuoli$")
    {
        events << CardsMoveOneTime;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasLordSkill("zuoli") && target->getPhase() == Player::NotActive;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
                CardMoveReason reason = move.reason;
                if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE && (reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_RESPONSE
                && player->askForSkillInvoke("zuoli", data)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isNude() && p->getKingdom() == "wu")
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "zuoli-invoke", true, true);
                if (target) {
                    const Card *card = NULL;
                    card = room->askForCard(target, "..", "@zuoli-give:" + player->objectName(), data, Card::MethodNone);
                    if (card) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "zuoli", QString());
                        room->obtainCard(player, card, reason);
                    }
                }
            }
        }
        return false;
    }
};

class Qixi : public OneCardViewAsSkill
{
public:
    Qixi() : OneCardViewAsSkill("qixi")
    {
        filter_pattern = ".|black";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

class FenweiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    FenweiViewAsSkill() :ZeroCardViewAsSkill("fenwei")
    {
        response_pattern = "@@fenwei";
    }

    const Card *viewAs() const
    {
        return new FenweiCard;
    }
};

class Fenwei : public TriggerSkill
{
public:
    Fenwei() : TriggerSkill("fenwei")
    {
        events << TargetSpecifying;
        view_as_skill = new FenweiViewAsSkill;
        frequency = Limited;
        limit_mark = "@fenwei";
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        ServerPlayer *ganning = room->findPlayerBySkillName(objectName());
        if (!ganning || ganning->getMark("@fenwei") <= 0) return false;

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.card->isNDTrick())
            return false;

        QStringList target_list;
        foreach(ServerPlayer *p, use.to)
            target_list << p->objectName();
        room->setPlayerProperty(ganning, "fenwei_targets", target_list.join("+"));
        ganning->tag["fenwei"] = data;
        room->askForUseCard(ganning, "@@fenwei", "@fenwei-card");
        data = ganning->tag["fenwei"];

        return false;
    }
};

class Keji : public TriggerSkill
{
public:
    Keji() : TriggerSkill("keji")
    {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        frequency = Frequent;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            bool can_trigger = true;
            if (lvmeng->hasFlag("KejiSlashInPlayPhase")) {
                can_trigger = false;
                lvmeng->setFlags("-KejiSlashInPlayPhase");
            }
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && lvmeng->isAlive() && lvmeng->hasSkill(this)) {
                if (can_trigger && lvmeng->askForSkillInvoke(this)) {
                    if (lvmeng->getHandcardNum() > lvmeng->getMaxCards()) {
                        room->broadcastSkillInvoke(objectName());
                    }
                    lvmeng->skip(Player::Discard);
                }
            }
        } else if (lvmeng->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;
            if (card->isKindOf("Slash"))
                lvmeng->setFlags("KejiSlashInPlayPhase");
        }

        return false;
    }
};

class Qinxue : public PhaseChangeSkill
{
public:
    Qinxue() : PhaseChangeSkill("qinxue")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("qinxue") == 0;
    }

    bool onPhaseChange(ServerPlayer *lvmeng) const
    {
        Room *room = lvmeng->getRoom();
        int n = lvmeng->getHandcardNum() - lvmeng->getHp();
        int wake_lim = (Sanguosha->getPlayerCount(room->getMode()) >= 7) ? 2 : 3;
        if (n < wake_lim) return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(lvmeng, objectName());
        //room->doLightbox("$QinxueAnimate");
        room->doSuperLightbox("lvmeng", "qinxue");

        LogMessage log;
        log.type = "#QinxueWake";
        log.from = lvmeng;
        log.arg = QString::number(n);
        log.arg2 = "qinxue";
        room->sendLog(log);

        room->setPlayerMark(lvmeng, "qinxue", 1);
        if (room->changeMaxHpForAwakenSkill(lvmeng) && lvmeng->getMark("qinxue") == 1)
            room->acquireSkill(lvmeng, "gongxin");

        return false;
    }
};

class Kurou : public ZeroCardViewAsSkill
{
public:
    Kurou() : ZeroCardViewAsSkill("kurou")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KurouCard");
    }

    const Card *viewAs() const
    {
        return new KurouCard;
    }
};

class KurouTargetMod : public TargetModSkill
{
public:
    KurouTargetMod() : TargetModSkill("#kurou-target")
    {
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        return from->hasFlag("kurou");
    }

    int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (card->isRed() && from->hasFlag("kurou"))
            return 1000;
        else
            return 0;
    }
};

class Yingzi : public DrawCardsSkill
{
public:
    Yingzi() : DrawCardsSkill("yingzi")
    {
        frequency = Compulsory;
    }

    int getDrawNum(ServerPlayer *zhouyu, int n) const
    {
        Room *room = zhouyu->getRoom();

        int index = qrand() % 2 + 1;
        if (!zhouyu->hasInnateSkill(this)) {
            if (zhouyu->hasSkill("hunshang"))
                index = 3;
        }
        room->broadcastSkillInvoke(objectName(), index);
        room->sendCompulsoryTriggerLog(zhouyu, objectName());

        return n + 1;
    }
};

class YingziMaxCards : public MaxCardsSkill
{
public:
    YingziMaxCards() : MaxCardsSkill("#yingzi")
    {
    }

    int getFixed(const Player *target) const
    {
        if (target->hasSkill("yingzi"))
            return target->getMaxHp();
        else
            return -1;
    }
};

class Fanjian : public OneCardViewAsSkill
{
public:
    Fanjian() : OneCardViewAsSkill("fanjian")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("FanjianCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        FanjianCard *card = new FanjianCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class GuoseViewAsSkill : public OneCardViewAsSkill
{
public:
    GuoseViewAsSkill() : OneCardViewAsSkill("guose")
    {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GuoseCard") && !(player->isNude() && player->getHandPile().isEmpty());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        GuoseCard *card = new GuoseCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Guose : public TriggerSkill
{
public:
    Guose() : TriggerSkill("guose")
    {
        events << CardFinished;
        view_as_skill = new GuoseViewAsSkill;
    }

    int getPriority(TriggerEvent) const
    {
        return 1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Indulgence") && use.card->getSkillName() == objectName())
            player->drawCards(1, objectName());
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return (card->isKindOf("Indulgence") ? 1 : 2);
    }
};

class LiuliViewAsSkill : public OneCardViewAsSkill
{
public:
    LiuliViewAsSkill() : OneCardViewAsSkill("liuli")
    {
        filter_pattern = ".!";
        response_pattern = "@@liuli";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        LiuliCard *liuli_card = new LiuliCard;
        liuli_card->addSubcard(originalCard);
        return liuli_card;
    }
};

class Liuli : public TriggerSkill
{
public:
    Liuli() : TriggerSkill("liuli")
    {
        events << TargetConfirming;
        view_as_skill = new LiuliViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(daqiao) && !daqiao->isNude()) {
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            bool can_invoke = false;
            foreach (ServerPlayer *p, players) {
                if (use.from->canSlash(p, use.card, false) && daqiao->inMyAttackRange(p)) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke) {
                QString prompt = "@liuli:" + use.from->objectName();
                room->setPlayerFlag(use.from, "LiuliSlashSource");
                // a temp nasty trick
                daqiao->tag["liuli-card"] = QVariant::fromValue(use.card); // for the server (AI)
                room->setPlayerProperty(daqiao, "liuli", use.card->toString()); // for the client (UI)
                if (room->askForUseCard(daqiao, "@@liuli", prompt, -1, Card::MethodDiscard)) {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                    ServerPlayer *target = NULL;
                    foreach(ServerPlayer *p, players) {
                        if (p->hasFlag("LiuliTarget")) {
                            p->setFlags("-LiuliTarget");
                            target = p;
                            break;
                        }
                    }
                    if (target && room->askForDiscard(use.from, objectName(), 1, 1, true, true, QString("@liuli-dis::%1").arg(target->objectName()))) {
                        if (!use.from->canSlash(target, false))
                            return false;
                        use.to.removeOne(daqiao);
                        use.to.append(target);
                        room->sortByActionOrder(use.to);
                        data = QVariant::fromValue(use);
                        room->getThread()->trigger(TargetConfirming, room, target, data);
                        return false;
                    } else {
                        use.to.removeOne(daqiao);
                        data = QVariant::fromValue(use);
                        return false;
                    }
                } else {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                }
            }
        }

        return false;
    }
};

class Qianxun : public TriggerSkill
{
public:
    Qianxun() : TriggerSkill("qianxun")
    {
        events << TrickEffect << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TrickEffect && TriggerSkill::triggerable(player)) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.multiple && effect.card->getTypeId() == Card::TypeTrick
                && (effect.card->isKindOf("DelayedTrick") || effect.from != player)
                && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                player->tag["QianxunEffectData"] = data;

                CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString());
                QList<int> handcards = player->handCards();
                QList<ServerPlayer *> open;
                open << player;
                player->addToPile("moral", handcards, false, open, reason);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("moral").length() > 0) {
                        DummyCard *dummy = new DummyCard(p->getPile("moral"));
                        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "moral", QString());
                        room->obtainCard(p, dummy, reason, false);
                        delete dummy;
                    }
                }
            }
        }
        return false;
    }
};

class LianyingViewAsSkill : public ZeroCardViewAsSkill
{
public:
    LianyingViewAsSkill() : ZeroCardViewAsSkill("lianying")
    {
        response_pattern = "@@lianying";
    }

    const Card *viewAs() const
    {
        return new LianyingCard;
    }
};

class Lianying : public TriggerSkill
{
public:
    Lianying() : TriggerSkill("lianying")
    {
        events << CardsMoveOneTime;
        view_as_skill = new LianyingViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            luxun->tag["LianyingMoveData"] = data;
            int count = 0;
            for (int i = 0; i < move.from_places.length(); i++) {
                if (move.from_places[i] == Player::PlaceHand) count++;
            }
            room->setPlayerMark(luxun, "lianying", count);
            room->askForUseCard(luxun, "@@lianying", "@lianying-card:::" + QString::number(count));
        }
        return false;
    }
};

class Jieyin : public ViewAsSkill
{
public:
    Jieyin() : ViewAsSkill("jieyin")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHandcardNum() >= 2 && !player->hasUsed("JieyinCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        JieyinCard *jieyin_card = new JieyinCard();
        jieyin_card->addSubcards(cards);
        return jieyin_card;
    }
};

class Xiaoji : public TriggerSkill
{
public:
    Xiaoji() : TriggerSkill("xiaoji")
    {
        events << CardsMoveOneTime << CardUsed; 
        frequency = Frequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == sunshangxiang && move.from_places.contains(Player::PlaceEquip)) {
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (!sunshangxiang->isAlive())
                        return false;
                    if (move.from_places[i] == Player::PlaceEquip) {
                        if (room->askForSkillInvoke(sunshangxiang, objectName())) {
                            room->broadcastSkillInvoke(objectName());
                            sunshangxiang->drawCards(1, objectName());
                            break;
                        }
                        else {
                            break;
                        }
                    }
                }
            }
        }
        else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeEquip && room->askForSkillInvoke(sunshangxiang, objectName())) {
            room->broadcastSkillInvoke(objectName());
            sunshangxiang->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class Chuli : public OneCardViewAsSkill
{
public:
    Chuli() : OneCardViewAsSkill("chuli")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ChuliCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ChuliCard *chuli_card = new ChuliCard;
        chuli_card->addSubcard(originalCard->getId());
        return chuli_card;
    }
};

class Jijiu : public OneCardViewAsSkill
{
public:
    Jijiu() : OneCardViewAsSkill("jijiu")
    {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("peach") && !player->hasFlag("Global_PreventPeach")
            && player->getPhase() == Player::NotActive && player->canDiscard(player, "he");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

class Wushuang : public TriggerSkill
{
public:
    Wushuang() : TriggerSkill("wushuang")
    {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(player)) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());

                QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                for (int i = 0; i < use.to.length(); i++) {
                    if (jink_list.at(i).toInt() == 1)
                        jink_list.replace(i, QVariant(2));
                }
                player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            } else if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(player)) {
                    room->broadcastSkillInvoke(objectName());
                    room->sendCompulsoryTriggerLog(player, objectName());

                    QStringList wushuang_tag;
                    foreach(ServerPlayer *to, use.to)
                        wushuang_tag << to->objectName();
                    player->tag["Wushuang_" + use.card->toString()] = wushuang_tag;
                }
                foreach (ServerPlayer *p, use.to.toSet()) {
                    if (TriggerSkill::triggerable(p)) {
                        room->broadcastSkillInvoke(objectName());
                        room->sendCompulsoryTriggerLog(p, objectName());

                        p->tag["Wushuang_" + use.card->toString()] = QStringList(player->objectName());
                    }
                }
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Duel")) {
                foreach(ServerPlayer *p, room->getAllPlayers())
                    p->tag.remove("Wushuang_" + use.card->toString());
            }
        }

        return false;
    }
};

class Liyu : public TriggerSkill
{
public:
    Liyu() : TriggerSkill("liyu")
    {
        events << Damage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && player != damage.to && !damage.to->hasFlag("Global_DebutFlag") && !damage.to->isNude()
            && damage.card && damage.card->isKindOf("Slash")) {
            Duel *duel = new Duel(Card::NoSuit, 0);
            duel->setSkillName("_liyu");

            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (/*p != damage.to &&*/ !player->isProhibited(p, duel) && p->getHp() > damage.to->getHp())
                    targets << p;
            }
            if (targets.isEmpty()) {
                delete duel;
            } else {
                ServerPlayer *target = room->askForPlayerChosen(damage.to, targets, objectName(), "@liyu:" + player->objectName(), true);
                if (target) {
                    room->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());

                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = damage.to;
                    log.to << player;
                    log.arg = objectName();
                    room->sendLog(log);

                    int id = room->askForCardChosen(player, damage.to, "he", objectName());
                    room->obtainCard(player, id);
                    if (player->isAlive() && target->isAlive() && !player->isLocked(duel))
                        room->useCard(CardUseStruct(duel, player, target));
                    else
                        delete duel;
                }
            }
        }
        return false;
    }
};

class Lijian : public OneCardViewAsSkill
{
public:
    Lijian() : OneCardViewAsSkill("lijian")
    {
        filter_pattern = "^BasicCard!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getAliveSiblings().length() > 1
            && player->canDiscard(player, "he") && !player->hasUsed("LijianCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class Biyue : public PhaseChangeSkill
{
public:
    Biyue() : PhaseChangeSkill("biyue")
    {
        frequency = Frequent;
    }

    bool onPhaseChange(ServerPlayer *diaochan) const
    {
        if (diaochan->getPhase() == Player::Finish) {
            Room *room = diaochan->getRoom();
            if (room->askForSkillInvoke(diaochan, objectName())) {
                room->broadcastSkillInvoke(objectName());
                diaochan->drawCards(1, objectName());
            }
        }

        return false;
    }
};

class Wansha : public TriggerSkill
{
public:
    Wansha() : TriggerSkill("wansha")
    {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent) const
    {
        return 7;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player == room->getAllPlayers().first()) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *jiaxu = room->getCurrent();
            if (!jiaxu || !TriggerSkill::triggerable(jiaxu) || jiaxu->getPhase() == Player::NotActive)
                return false;
            if (jiaxu->hasInnateSkill("wansha") || !jiaxu->hasSkill("jilve"))
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 3);

            room->notifySkillInvoked(jiaxu, objectName());

            LogMessage log;
            log.from = jiaxu;
            log.arg = objectName();
            if (jiaxu != dying.who) {
                log.type = "#WanshaTwo";
                log.to << dying.who;
            } else {
                log.type = "#WanshaOne";
            }
            room->sendLog(log);
        }
        return false;
    }
};

class Luanwu : public ZeroCardViewAsSkill
{
public:
    Luanwu() : ZeroCardViewAsSkill("luanwu")
    {
        frequency = Limited;
        limit_mark = "@chaos";
    }

    const Card *viewAs() const
    {
        return new LuanwuCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@chaos") >= 1;
    }
};

class LuanwuLose : public TriggerSkill
{
public:
    LuanwuLose() : TriggerSkill("#luanwu-lose")
    {
        frequency = Compulsory;
        events << Predamage;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (room->getTag("Luanwu").toBool() && damage.card && damage.card->isKindOf("Slash")) {

            room->loseHp(damage.to, damage.damage);

            return true;
        }
        return false;
    }
};

class Weimu : public ProhibitSkill
{
public:
    Weimu() : ProhibitSkill("weimu")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && (card->isKindOf("TrickCard") || card->isKindOf("QiceCard"))
            && card->isBlack() && card->getSkillName() != "nosguhuo"; // Be care!!!!!!
    }
};

class Guidao : public RetrialSkill
{
public:
    Guidao() : RetrialSkill("guidao", true)
    {
    }

    bool triggerable(const ServerPlayer *target) const
    {
        if (!TriggerSkill::triggerable(target))
            return false;

        if (target->isKongcheng()) {
            bool has_black = false;
            for (int i = 0; i < 5; i++) {
                const EquipCard *equip = target->getEquip(i);
                if (equip && equip->isBlack()) {
                    has_black = true;
                    break;
                }
            }
            return has_black;
        } else
            return true;
    }

    const Card *onRetrial(ServerPlayer *player, JudgeStruct *judge) const
    {
        QStringList prompt_list;
        prompt_list << "@guidao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        Room *room = player->getRoom();

        const Card *card = room->askForCard(player, ".|black", prompt, QVariant::fromValue(judge), Card::MethodResponse, judge->who, true);

        if (card != NULL) {
            room->broadcastSkillInvoke(objectName());
        }

        return card;
    }
};

class Jishi : public TriggerSkill
{
public:
    Jishi() : TriggerSkill("jishi")
    {
        events << CardUsed << CardResponded;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data) const
    {
        if (zhangjiao->getPhase() != Player::NotActive) return false;
        const Card *card_star = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card_star = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card_star = resp.m_card;
        }
        if (card_star->isRed() && zhangjiao->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName(), 1);

            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = zhangjiao;

            room->judge(judge);

            if (judge.isBad()) {
                QList<ServerPlayer *> helps;
                foreach (ServerPlayer *p, room->getAlivePlayers()) 
                {
                    if (p->isWounded())
                        helps << p;
                }
                ServerPlayer *target1 = room->askForPlayerChosen(zhangjiao, helps, objectName(), "jishi1-invoke", true, true);
                room->recover(target1, RecoverStruct(zhangjiao));
                if (card_star->isKindOf("Jink")) {
                    ServerPlayer *target2 = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(), "jishi2-invoke", true, true);
                    if (target2)
                        room->broadcastSkillInvoke(objectName(), 2);
                        room->damage(DamageStruct(objectName(), zhangjiao, target2, 1, DamageStruct::Thunder));    
                }
            }
        }
        return false;
    }
};

class HuangtianViewAsSkill : public OneCardViewAsSkill
{
public:
    HuangtianViewAsSkill() :OneCardViewAsSkill("huangtian_attach")
    {
        attached_lord_skill = true;
        filter_pattern = ".|red|.|hand!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return shouldBeVisible(player) && !player->hasFlag("ForbidHuangtian");
    }

    bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "qun";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        HuangtianCard *card = new HuangtianCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Huangtian : public TriggerSkill
{
public:
    Huangtian() : TriggerSkill("huangtian$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "huangtian")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("huangtian_attach"))
                    room->attachSkillToPlayer(p, "huangtian_attach");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "huangtian") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("huangtian_attach"))
                    room->detachSkillFromPlayer(p, "huangtian_attach", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("ForbidHuangtian"))
                room->setPlayerFlag(player, "-ForbidHuangtian");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("HuangtianInvoked"))
                    room->setPlayerFlag(p, "-HuangtianInvoked");
            }
        }
        return false;
    }
};

class Wangzun : public PhaseChangeSkill
{
public:
    Wangzun() : PhaseChangeSkill("wangzun")
    {
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (!isNormalGameMode(room->getMode()))
            return false;
        if (target->isLord() && target->getPhase() == Player::Start) {
            ServerPlayer *yuanshu = room->findPlayerBySkillName(objectName());
            if (yuanshu && room->askForSkillInvoke(yuanshu, objectName())) {
                room->broadcastSkillInvoke(objectName());
                yuanshu->drawCards(1, objectName());
                room->setPlayerFlag(target, "WangzunDecMaxCards");
            }
        }
        return false;
    }
};

class WangzunMaxCards : public MaxCardsSkill
{
public:
    WangzunMaxCards() : MaxCardsSkill("#wangzun-maxcard")
    {
    }

    int getExtra(const Player *target) const
    {
        if (target->hasFlag("WangzunDecMaxCards"))
            return -1;
        else
            return 0;
    }
};

class Tongji : public ProhibitSkill
{
public:
    Tongji() : ProhibitSkill("tongji")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (card->isKindOf("Slash")) {
            // get rangefix
            int rangefix = 0;
            if (card->isVirtualCard()) {
                QList<int> subcards = card->getSubcards();
                if (from->getWeapon() && subcards.contains(from->getWeapon()->getId())) {
                    const Weapon *weapon = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
                    rangefix += weapon->getRange() - from->getAttackRange(false);
                }

                if (from->getOffensiveHorse() && subcards.contains(from->getOffensiveHorse()->getId()))
                    rangefix += 1;
            }
            // find yuanshu
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(this) && p != to && p->getHandcardNum() > p->getHp()
                    && from->inMyAttackRange(p, rangefix)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Yicong : public DistanceSkill
{
public:
    Yicong() : DistanceSkill("yicong")
    {
    }

    int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill(this))
            correct--;
        if (to->hasSkill(this) && to->getHp() <= 2)
            correct++;

        return correct;
    }
};

class YicongEffect : public TriggerSkill
{
public:
    YicongEffect() : TriggerSkill("#yicong-effect")
    {
        events << HpChanged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        int hp = player->getHp();
        int reduce = 0;       
        if (data.canConvert<DamageStruct>()) {
            DamageStruct damage = data.value<DamageStruct>();
            reduce = damage.damage;
        } else if (!data.isNull()) {
            reduce = data.toInt();
        }
        if (hp <= 2 && hp + reduce > 2) {
            room->broadcastSkillInvoke("yicong", 1);
        }
        return false;
    }
};

class Qiaomeng : public TriggerSkill
{
public:
    Qiaomeng() : TriggerSkill("qiaomeng")
    {
        events << Damage << BeforeCardsMove;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damage && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
                && damage.card && damage.card->isKindOf("Slash") && player->canDiscard(damage.to, "e") && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                int id = room->askForCardChosen(player, damage.to, "e", objectName(), false, Card::MethodDiscard);
                CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), damage.to->objectName(),
                    objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, damage.to, player);
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_skillName == objectName() && move.reason.m_playerId == player->objectName()
                && move.card_ids.length() > 0) {
                const Card *card = Sanguosha->getCard(move.card_ids.first());
                if (card->isKindOf("Horse")) {
                    move.card_ids.clear();
                    data = QVariant::fromValue(move);
                    room->obtainCard(player, card);
                }
            }
        }
        return false;
    }
};

class Tianming : public TriggerSkill
{
public:
    Tianming() : TriggerSkill("tianming")
    {
        events << TargetConfirmed << CardFinished;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card == NULL || use.to.length() != 1 || use.to.first() != player || !(use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack())))
                return false;
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName(), 1);
                //room->askForDiscard(player, objectName(), 2, 2, false, true);
                player->drawCards(2, objectName());
                room->addPlayerMark(player, objectName() + use.card->toString());
            }
        }
        else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card == NULL || use.to.length() != 1 || !(use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack())))
                return false;
            ServerPlayer *to = use.to.first();
            if (to->isDead() || to->getMark(objectName() + use.card->toString()) <= 0)
                return false;
            room->askForDiscard(to, objectName(), 2, 2, false, true);
            room->setPlayerMark(to, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};

class MizhaoViewAsSkill : public ZeroCardViewAsSkill
{
public:
    MizhaoViewAsSkill() : ZeroCardViewAsSkill("mizhao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("MizhaoCard");
    }

    const Card *viewAs() const
    {
        return new MizhaoCard;
    }
};

class Mizhao : public TriggerSkill
{
public:
    Mizhao() : TriggerSkill("mizhao")
    {
        events << Pindian;
        view_as_skill = new MizhaoViewAsSkill;
    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number == pindian->to_number)
            return false;

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        if (winner->canSlash(loser, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mizhao");
            room->useCard(CardUseStruct(slash, winner, loser));
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

class MizhaoSlashNoDistanceLimit : public TargetModSkill
{
public:
    MizhaoSlashNoDistanceLimit() : TargetModSkill("#mizhao-slash-ndl")
    {
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->isKindOf("Slash") && card->getSkillName() == "mizhao")
            return 1000;
        else
            return 0;
    }
};

class Xiaoxi : public TriggerSkill
{
public:
    Xiaoxi() : TriggerSkill("xiaoxi")
    {
        events << Debut;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_xiaoxi");
        if (player->isLocked(slash) || !player->canSlash(opponent, slash, false)) {
            delete slash;
            return false;
        }
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(slash, player, opponent));
        return false;
    }
};

void StandardPackage::addGenerals()
{
    // Wei
    General *caocao = new General(this, "caocao$", "wei"); // WEI 001
    caocao->addSkill(new Jianxiong);
    caocao->addSkill(new Baye);
    caocao->addSkill(new Hujia);

    General *simayi = new General(this, "simayi", "wei", 3); // WEI 002
    simayi->addSkill(new Yangshi);
    simayi->addSkill(new Guiming);

    General *xiahoudun = new General(this, "xiahoudun", "wei"); // WEI 003
    xiahoudun->addSkill(new Ganglie);
    xiahoudun->addSkill(new Qingjian);

    General *zhangliao = new General(this, "zhangliao", "wei"); // WEI 004
    zhangliao->addSkill(new Tuxi);
    zhangliao->addSkill(new TuxiAct);
    related_skills.insertMulti("tuxi", "#tuxi");

    General *xuchu = new General(this, "xuchu", "wei"); // WEI 005
    xuchu->addSkill(new Luoyi);
    xuchu->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill(new Tiandu);
    //guojia->addSkill(new TianduGot);
    //related_skills.insertMulti("tiandu", "#tiandu-got");
    guojia->addSkill(new Yiji);
    related_skills.insertMulti("yiji", "#yiji");

    General *zhenji = new General(this, "zhenji", "wei", 3, false); // WEI 007
    zhenji->addSkill(new Qingguo);
    zhenji->addSkill(new Luoshen);
   
    General *xunyu = new General(this, "xunyu", "wei", 3); // WEI 013
    xunyu->addSkill(new Quhu);
    xunyu->addSkill(new Jieming);
    
    // Shu
    General *liubei = new General(this, "liubei$", "shu"); // SHU 001
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    General *guanyu = new General(this, "guanyu", "shu"); // SHU 002
    guanyu->addSkill(new Wusheng);
    guanyu->addSkill(new Yijue);

    General *zhangfei = new General(this, "zhangfei", "shu"); // SHU 003
    zhangfei->addSkill(new Paoxiao);
    zhangfei->addSkill(new Mangzhi);

    General *zhugeliang = new General(this, "zhugeliang", "shu", 3); // SHU 004
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);

    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addSkill(new Longwei);
    zhaoyun->addSkill(new LongweiAttach);
    zhaoyun->addSkill(new Longhun);
    //zhaoyun->addSkill(new Yajiao);
    related_skills.insertMulti("longwei", "#longwei-attach");

    General *machao = new General(this, "machao", "shu"); // SHU 006
    machao->addSkill(new Mashu);
    machao->addSkill(new Tieji);
    machao->addSkill(new TiejiClear);
    related_skills.insertMulti("tieji", "#tieji-clear");

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Qicai);
    huangyueying->addSkill(new Cangji);

    General *xushu = new General(this, "xushu", "shu"); // SHU 017
    xushu->addSkill(new Zhue);
    xushu->addSkill(new Qianxin);
    xushu->addRelateSkill("jianyan");

    // Wu
    General *sunquan = new General(this, "sunquan$", "wu"); // WU 001
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new Zuoli);

    General *ganning = new General(this, "ganning", "wu"); // WU 002
    ganning->addSkill(new Qixi);
    ganning->addSkill(new Fenwei);

    General *lvmeng = new General(this, "lvmeng", "wu"); // WU 003
    lvmeng->addSkill(new Keji);
    lvmeng->addSkill(new Qinxue);

    General *huanggai = new General(this, "huanggai", "wu"); // WU 004
    huanggai->addSkill(new Kurou);
    huanggai->addSkill(new KurouTargetMod);
    related_skills.insertMulti("kurou", "#kurou-target");

    General *zhouyu = new General(this, "zhouyu", "wu", 3); // WU 005
    zhouyu->addSkill(new Yingzi);
    zhouyu->addSkill(new YingziMaxCards);
    zhouyu->addSkill(new Fanjian);
    related_skills.insertMulti("yingzi", "#yingzi");

    General *daqiao = new General(this, "daqiao", "wu", 3, false); // WU 006
    daqiao->addSkill(new Guose);
    daqiao->addSkill(new Liuli);

    General *luxun = new General(this, "luxun", "wu", 3); // WU 007
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    General *sunshangxiang = new General(this, "sunshangxiang", "wu", 3, false); // WU 008
    sunshangxiang->addSkill(new Jieyin);
    sunshangxiang->addSkill(new Xiaoji);

    // Qun
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Chuli);
    huatuo->addSkill(new Jijiu);

    General *lvbu = new General(this, "lvbu", "qun", 5); // QUN 002
    lvbu->addSkill(new Wushuang);
    lvbu->addSkill(new Liyu);

    General *diaochan = new General(this, "diaochan", "qun", 3, false); // QUN 003
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);

    General *jiaxu = new General(this, "jiaxu", "qun", 3); // QUN 007
    jiaxu->addSkill(new Wansha);
    jiaxu->addSkill(new Luanwu);
    jiaxu->addSkill(new LuanwuLose);
    jiaxu->addSkill(new Weimu);
    related_skills.insertMulti("luanwu", "#luanwu-lose");
    
    General *zhangjiao = new General(this, "zhangjiao$", "qun", 3); // QUN 010
    zhangjiao->addSkill(new Jishi);
    zhangjiao->addSkill(new Guidao);
    zhangjiao->addSkill(new Huangtian);

    General *yuanshu = new General(this, "yuanshu", "qun"); // QUN 021
    yuanshu->addSkill(new Wangzun);
    yuanshu->addSkill(new WangzunMaxCards);
    yuanshu->addSkill(new Tongji);
    related_skills.insertMulti("wangzun", "#wangzun-maxcard");

    General *gongsunzan = new General(this, "gongsunzan", "qun"); // QUN 026
    gongsunzan->addSkill(new Qiaomeng);
    gongsunzan->addSkill(new Yicong);
    gongsunzan->addSkill(new YicongEffect);
    related_skills.insertMulti("yicong", "#yicong-effect");

    General *liuxie = new General(this, "liuxie", "qun", 3);
    liuxie->addSkill(new Tianming);
    liuxie->addSkill(new Mizhao);
    liuxie->addSkill(new MizhaoSlashNoDistanceLimit);
    related_skills.insertMulti("mizhao", "#mizhao-slash-ndl");

    // for skill cards
    addMetaObject<TuxiCard>();
    addMetaObject<YijiCard>();   
    addMetaObject<QuhuCard>();
    addMetaObject<RendeCard>();
    addMetaObject<JijiangCard>();
    addMetaObject<YijueCard>();
    addMetaObject<LongweiCard>();
    addMetaObject<JianyanCard>();
    addMetaObject<ZhihengCard>();
    addMetaObject<FenweiCard>();
    addMetaObject<KurouCard>();
    addMetaObject<FanjianCard>();
    addMetaObject<GuoseCard>();
    addMetaObject<LiuliCard>();
    addMetaObject<LianyingCard>(); 
    addMetaObject<JieyinCard>();
    addMetaObject<ChuliCard>();
    addMetaObject<LijianCard>();
    addMetaObject<LuanwuCard>();
    addMetaObject<HuangtianCard>();
    addMetaObject<MizhaoCard>();

    skills << new Xiaoxi << new HuangtianViewAsSkill << new NonCompulsoryInvalidity << new Jianyan << new LongweiViewAsSkill;
}

