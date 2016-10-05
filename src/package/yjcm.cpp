#include "yjcm.h"
#include "wind.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"
#include "exppattern.h"

JieyueCard::JieyueCard()
{

}

void JieyueCard::onEffect(const CardEffectStruct &effect) const
{
    if (!effect.to->isNude()) {
        Room *room = effect.to->getRoom();
        const Card *card = room->askForExchange(effect.to, "jieyue", 1, 1, true, QString("@jieyue_put:%1").arg(effect.from->objectName()), true);

        if (card != NULL)
            effect.from->addToPile("jieyue_pile", card);
    }
}

bool JieyueCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

class JieyueVS : public OneCardViewAsSkill
{
public:
    JieyueVS() : OneCardViewAsSkill("jieyue")
    {
        response_or_use = true;
    }

    bool viewFilter(const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            return false;
        }

        if (pattern == "jink")
            return to_select->isRed();
        else if (pattern == "nullification")
            return to_select->isBlack();
        return false;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (!player->getPile("jieyue_pile").isEmpty() && (pattern == "jink" || pattern == "nullification")) || (pattern == "@@jieyue" && !player->isKongcheng());
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->getPile("jieyue_pile").isEmpty();
    }

    const Card *viewAs(const Card *card) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            JieyueCard *jy = new JieyueCard;
            return jy;
        }

        if (card->isRed()) {
            Jink *jink = new Jink(Card::SuitToBeDecided, 0);
            jink->addSubcard(card);
            jink->setSkillName(objectName());
            return jink;
        } else if (card->isBlack()) {
            Nullification *nulli = new Nullification(Card::SuitToBeDecided, 0);
            nulli->addSubcard(card);
            nulli->setSkillName(objectName());
            return nulli;
        }
        return NULL;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Nullification"))
            return 3;
        else if (card->isKindOf("Jink"))
            return 2;

        return 1;
    }
};

class Jieyue : public TriggerSkill
{
public:
    Jieyue() : TriggerSkill("jieyue")
    {
        events << EventPhaseStart;
        view_as_skill = new JieyueVS;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Start && !player->getPile("jieyue_pile").isEmpty()) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            DummyCard *dummy = new DummyCard(player->getPile("jieyue_pile"));
            player->obtainCard(dummy);
            delete dummy;
        } else if (player->getPhase() == Player::Finish) {
            room->askForUseCard(player, "@@jieyue", "@jieyue", -1, Card::MethodDiscard, false);
        }
        return false;
    }
};

class JieyueClear : public DetachEffectSkill
{
public:
    JieyueClear() : DetachEffectSkill("jieyue", "jieyue_pile")
    {
    }
};

class Luoying : public TriggerSkill
{
public:
    Luoying() : TriggerSkill("luoying")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        //frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (caozhi->getPhase() == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0)
                        p->setMark(objectName(), 0);
                }
            }
        }
        else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!TriggerSkill::triggerable(caozhi)) return false;
            if (move.from == caozhi || move.from == NULL || caozhi->getMark(objectName()) > 0)
                return false;
            if (move.to_place == Player::DiscardPile
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                || move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)) {
                QList<int> card_ids;
                int i = 0;
                foreach(int card_id, move.card_ids) {
                    if (Sanguosha->getCard(card_id)->getSuit() == Card::Club
                        && room->getCardPlace(card_id) == Player::DiscardPile
                        && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                        && move.from_places[i] == Player::PlaceJudge)
                        || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                        && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                        card_ids << card_id;
                    i++;
                }
                if (card_ids.isEmpty())
                    return false;
                else if (caozhi->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke("luoying");
                    room->fillAG(card_ids, caozhi);
                    int id = room->askForAG(caozhi, card_ids, false, objectName());
                    room->clearAG(caozhi);
                    Card *card = Sanguosha->getCard(id);
                    caozhi->obtainCard(card);
                    caozhi->setMark(objectName(), 1);
                }
            }
        }
        return false;
    }
};

class Jiushi : public ZeroCardViewAsSkill
{
public:
    Jiushi() : ZeroCardViewAsSkill("jiushi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("analeptic") && player->faceUp();
    }

    const Card *viewAs() const
    {
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName(objectName());
        return analeptic;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return qrand() % 2 + 1;
    }
};

class JiushiFlip : public TriggerSkill
{
public:
    JiushiFlip() : TriggerSkill("#jiushi-flip")
    {
        events << PreCardUsed << PreDamageDone << DamageComplete;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "jiushi")
                player->turnOver();
        } else if (triggerEvent == PreDamageDone) {
            player->tag["PredamagedFace"] = !player->faceUp();
        } else if (triggerEvent == DamageComplete) {
            bool facedown = player->tag.value("PredamagedFace").toBool();
            player->tag.remove("PredamagedFace");
            if (facedown && !player->faceUp() && player->askForSkillInvoke("jiushi", data)) {
                room->broadcastSkillInvoke("jiushi", 3);
                player->turnOver();
            }
        }

        return false;
    }
};

class Enyuan : public TriggerSkill
{
public:
    Enyuan() : TriggerSkill("enyuan")
    {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpRecover) {
            if (player->getPhase() == Player::NotActive) {
                RecoverStruct recover = data.value<RecoverStruct>();
                for (int i = 0; i<recover.recover; i++) {
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@enyuan-en", false);
                    room->broadcastSkillInvoke("enyuan", qrand() % 2 + 1);
                    room->sendCompulsoryTriggerLog(player, objectName());
                    target->drawCards(1, objectName());
                }
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *current = room->getCurrent();    
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                for (int i = 0; i < damage.damage; i++) {
                    QList<ServerPlayer *> targets;
                    if (source)
                        targets << source;
                    if (current)
                        targets << current;
                    if (!targets.isEmpty()) {
                        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@enyuan-yuan", false);
                        room->broadcastSkillInvoke("enyuan", 3);
                        room->sendCompulsoryTriggerLog(player, objectName());

                        const Card *card = room->askForCard(target, ".|heart|.|hand", "@nosenyuan-heart", data, Card::MethodNone);
                        if (card) {
                            player->obtainCard(card);
                            room->broadcastSkillInvoke("enyuan", 4);
                        }
                        else
                            room->loseHp(target);
                    }
                }
            }
        }

        return false;
    }
};

XuanhuoCard::XuanhuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void XuanhuoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "xuanhuo");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "xuanhuo", "@xuanhuo-give:" + effect.to->objectName());
    if (target != effect.from) {
        CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "xuanhuo", QString());
        room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
    }
}

class Xuanhuo : public OneCardViewAsSkill
{
public:
    Xuanhuo() :OneCardViewAsSkill("xuanhuo")
    {
        filter_pattern = ".|heart|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("XuanhuoCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XuanhuoCard *xuanhuoCard = new XuanhuoCard;
        xuanhuoCard->addSubcard(originalCard);
        return xuanhuoCard;
    }
};

class Huilei : public TriggerSkill
{
public:
    Huilei() :TriggerSkill("huilei")
    {
        events << Death;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && killer != player) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            QString killer_name = killer->getGeneralName();
            if (killer_name.contains("zhugeliang") || killer_name == "wolong")
                room->broadcastSkillInvoke(objectName(), 1);
            else
                room->broadcastSkillInvoke(objectName(), 2);

            killer->throwAllHandCardsAndEquips();
            room->setPlayerFlag(killer, "huilei");
        }

        return false;
    }
};

class Xuanfeng : public TriggerSkill
{
public:
    Xuanfeng() : TriggerSkill("xuanfeng")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    void perform(Room *room, ServerPlayer *lingtong) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
            if (lingtong->canDiscard(target, "he"))
                targets << target;
        }
        if (targets.isEmpty())
            return;

        if (lingtong->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());

            ServerPlayer *first = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            ServerPlayer *second = NULL;
            int first_id = -1;
            int second_id = -1;
            if (first != NULL) {
                first_id = room->askForCardChosen(lingtong, first, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(first_id, first, lingtong);
            }
            if (!lingtong->isAlive())
                return;
            targets.clear();
            foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
                if (lingtong->canDiscard(target, "he"))
                    targets << target;
            }
            if (!targets.isEmpty())
                second = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            if (second != NULL) {
                second_id = room->askForCardChosen(lingtong, second, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(second_id, second, lingtong);
            }
        }
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            lingtong->setMark("xuanfeng", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != lingtong)
                return false;

            if (lingtong->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                lingtong->addMark("xuanfeng", move.card_ids.length());

            if (move.from_places.contains(Player::PlaceEquip) && TriggerSkill::triggerable(lingtong))
                perform(room, lingtong);
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(lingtong)
            && lingtong->getPhase() == Player::Discard && lingtong->getMark("xuanfeng") >= 2) {
            perform(room, lingtong);
        }

        return false;
    }
};

MingceCard::MingceCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    QList<ServerPlayer *> targets;
    if (Slash::IsAvailable(effect.to)) {
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->canSlash(p))
                targets << p;
        }
    }

    ServerPlayer *target = NULL;
    QStringList choicelist;
    choicelist << "draw";
    if (!targets.isEmpty() && effect.from->isAlive()) {
        target = room->askForPlayerChosen(effect.from, targets, "mingce", "@dummy-slash2:" + effect.to->objectName());
        target->setFlags("MingceTarget"); // For AI

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        choicelist << "use";
    }

    try {
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "mingce", QString());
        room->obtainCard(effect.to, this, reason);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");
        throw triggerEvent;
    }

    QString choice = room->askForChoice(effect.to, "mingce", choicelist.join("+"));
    if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");

    if (choice == "use") {
        if (effect.to->canSlash(target, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mingce");
            room->useCard(CardUseStruct(slash, effect.to, target));
        }
    } else if (choice == "draw") {
        effect.to->drawCards(1, "mingce");
    }
}

class Mingce : public OneCardViewAsSkill
{
public:
    Mingce() : OneCardViewAsSkill("mingce")
    {
        filter_pattern = "EquipCard,Slash";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MingceCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);

        return mingceCard;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;
        else
            return -1;
    }
};

class Zhichi : public TriggerSkill
{
public:
    Zhichi() : TriggerSkill("zhichi")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::NotActive)
            return false;

        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
            if (player->getMark("@late") == 0)
                room->addPlayerMark(player, "@late");

            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);
        }

        return false;
    }
};

class ZhichiProtect : public TriggerSkill
{
public:
    ZhichiProtect() : TriggerSkill("#zhichi-protect")
    {
        events << CardEffected;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Slash") || effect.card->isNDTrick()) && effect.to->getMark("@late") > 0) {
            room->broadcastSkillInvoke("zhichi", 2);
            room->notifySkillInvoked(effect.to, "zhichi");
            LogMessage log;
            log.type = "#ZhichiAvoid";
            log.from = effect.to;
            log.arg = "zhichi";
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class ZhichiClear : public TriggerSkill
{
public:
    ZhichiClear() : TriggerSkill("#zhichi-clear")
    {
        events << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || player != room->getCurrent())
                return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@late") > 0)
                room->setPlayerMark(p, "@late", 0);
        }

        return false;
    }
};

GanluCard::GanluCard()
{
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const
{
    Room *room = first->getRoom();

    QList<int> equips1, equips2;
    foreach(const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach(const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equips1, second, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_SWAP, first->objectName(), second->objectName(), "ganlu", QString()));
    CardsMoveStruct move2(equips2, first, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_SWAP, second->objectName(), first->objectName(), "ganlu", QString()));
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCardsAtomic(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    switch (targets.length()) {
    case 0: return true;
    case 1: {
        int n1 = targets.first()->getEquips().length();
        int n2 = to_select->getEquips().length();
        return qAbs(n1 - n2) <= Self->getLostHp();
    }
    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);

    swapEquip(targets.first(), targets[1]);
}

class Ganlu : public ZeroCardViewAsSkill
{
public:
    Ganlu() : ZeroCardViewAsSkill("ganlu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GanluCard");
    }

    const Card *viewAs() const
    {
        return new GanluCard;
    }
};

class Buyi : public TriggerSkill
{
public:
    Buyi() : TriggerSkill("buyi")
    {
        events << Dying;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *wuguotai, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (player->isKongcheng()) return false;
        if (player->getHp() < 1 && wuguotai->askForSkillInvoke(this, data)) {
            const Card *card = NULL;
            if (player == wuguotai)
                card = room->askForCardShow(player, wuguotai, objectName());
            else {
                int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
                card = Sanguosha->getCard(card_id);
            }

            room->showCard(player, card->getEffectiveId());

            if (card->getTypeId() != Card::TypeBasic) {
                if (!player->isJilei(card))
                    room->throwCard(card, player);

                room->broadcastSkillInvoke(objectName());
                room->recover(player, RecoverStruct(wuguotai));
            }
        }
        return false;
    }
};

XinzhanCard::XinzhanCard()
{
    target_fixed = true;
}

void XinzhanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> cards = room->getNCards(3), left;

    LogMessage log;
    log.type = "$ViewDrawPile";
    log.from = source;
    log.card_str = IntList2StringList(cards).join("+");
    room->sendLog(log, source);

    left = cards;

    QList<int> hearts, non_hearts;
    foreach (int card_id, cards) {
        const Card *card = Sanguosha->getCard(card_id);
        if (card->getSuit() == Card::Heart)
            hearts << card_id;
        else
            non_hearts << card_id;
    }

    if (!hearts.isEmpty()) {
        DummyCard *dummy = new DummyCard;
        do {
            room->fillAG(left, source, non_hearts);
            int card_id = room->askForAG(source, hearts, true, "xinzhan");
            if (card_id == -1) {
                room->clearAG(source);
                break;
            }

            hearts.removeOne(card_id);
            left.removeOne(card_id);

            dummy->addSubcard(card_id);
            room->clearAG(source);
        } while (!hearts.isEmpty());

        if (dummy->subcardsLength() > 0) {
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length() + dummy->subcardsLength()));
            source->obtainCard(dummy);
            foreach(int id, dummy->getSubcards())
                room->showCard(source, id);
        }
        delete dummy;
    }

    if (!left.isEmpty())
        room->askForGuanxing(source, left, Room::GuanxingUpOnly);
}

class Xinzhan : public ZeroCardViewAsSkill
{
public:
    Xinzhan() : ZeroCardViewAsSkill("xinzhan")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XinzhanCard");
    }

    const Card *viewAs() const
    {
        return new XinzhanCard;
    }
};

class Quanji : public TriggerSkill
{
public:
    Quanji() : TriggerSkill("quanji")
    {
        events << Damaged;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isDead())
            return false;

        foreach (ServerPlayer *zhonghui, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(zhonghui)) continue;
            if (zhonghui->distanceTo(player) <= 1) {
                int x = damage.damage;
                for (int i = 0; i < x; i++) {
                    if (player->askForSkillInvoke(objectName(), QString("quanji_invoke:%1").arg(zhonghui->objectName()))) {
                        room->broadcastSkillInvoke("quanji");
                        if (player->askForSkillInvoke(objectName(), "quanji_draw"))
                            room->drawCards(player, 1, objectName());
                        if (!player->isKongcheng()) {
                            int card_id;
                            if (player->getHandcardNum() == 1) {
                                room->getThread()->delay();
                                card_id = player->handCards().first();
                            } else {
                                const Card *card = room->askForExchange(player, "quanji", 1, 1, false, "QuanjiPush");
                                card_id = card->getEffectiveId();
                                delete card;
                            }
                            zhonghui->addToPile("power", card_id);
                        }
                    }
                }
            }
        }
        return false;
    }
};

class QuanjiKeep : public MaxCardsSkill
{
public:
    QuanjiKeep() : MaxCardsSkill("#quanji")
    {
        frequency = Frequent;
    }

    int getExtra(const Player *target) const
    {
        if (target->hasSkill(objectName()) && target->getMark("zili") == 0)
            return target->getPile("power").length();
        else
            return 0;
    }
};

class QuanjiClear : public DetachEffectSkill
{
public:
    QuanjiClear() : DetachEffectSkill("quanji", "power")
    {
    }
};

class Zili : public PhaseChangeSkill
{
public:
    Zili() : PhaseChangeSkill("zili")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("zili") == 0
            && target->getPile("power").length() >= 4;
    }

    bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZiliAnimate", 4000);

        room->doSuperLightbox("zhonghui", "zili");

        room->setPlayerMark(zhonghui, "zili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui) && zhonghui->getMark("zili") == 1)
            room->acquireSkill(zhonghui, "paiyi");

        return false;
    }
};

PaiyiCard::PaiyiCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() < 2;
}

void PaiyiCard::use(Room *room, ServerPlayer *zhonghui, QList<ServerPlayer *> &targets) const
{
    QList<int> powers = zhonghui->getPile("power");
    if (powers.length() < 2) return;

    room->broadcastSkillInvoke("paiyi", targets.contains(zhonghui) ? 1 : 2);

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), zhonghui->objectName(), "paiyi", QString());
    room->throwCard(this, reason, NULL);
    foreach(ServerPlayer *target, targets) {
        room->drawCards(target, 1, "paiyi");
        if (target->getHandcardNum() > zhonghui->getHandcardNum())
            room->damage(DamageStruct("paiyi", zhonghui, target));
    }
}

class Paiyi : public ViewAsSkill
{
public:
    Paiyi() : ViewAsSkill("paiyi")
    {
        expand_pile = "power";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getPile("power").length() > 1 && !player->hasUsed("PaiyiCard");
    }
    
    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;

        if (Self->getPile("power").length() > 1) {
            return Self->getPile("power").contains(to_select->getId());
        }

        return false;
    }
    
    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            PaiyiCard *py = new PaiyiCard;
            py->addSubcards(cards);
            return py;
        } else
            return NULL;
    }
};

//YJCM2012

class Zhenlie : public TriggerSkill
{
public:
    Zhenlie() : TriggerSkill("zhenlie")
    {
        events << TargetConfirming;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        use.to.removeOne(player);
                        if (player->canDiscard(use.from, "he")) {
                            int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                            room->throwCard(id, use.from, player);
                        }
                        room->loseHp(player);
                        data = QVariant::fromValue(use);
                    }
                }
            }
        }
        return false;
    }
};

class Miji : public TriggerSkill
{
public:
    Miji() : TriggerSkill("miji")
    {
        events << EventPhaseStart << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());
            int num = target->getLostHp();
            target->drawCards(num, objectName());
            target->setMark(objectName(), 0);
            if (!target->isKongcheng()) {
                forever {
                    int n = target->getMark(objectName());
                    if (n < num && !target->isKongcheng()) {
                        QList<int> handcards = target->handCards();
                        if (!room->askForYiji(target, handcards, objectName(), false, false, true, num - n)) {
                            if (n == 0)
                                return false; // User select cancel at the first time of askForYiji, it can be treated as canceling the distribution of the cards

                            break;
                        }
                    } else {
                        break;
                    }
                }
                // give the rest cards randomly
                /*if (target->getMark(objectName()) < num && !target->isKongcheng()) {
                    int rest_num = num - target->getMark(objectName());
                    forever {
                        QList<int> handcard_list = target->handCards();
                        qShuffle(handcard_list);
                        int give = qrand() % rest_num + 1;
                        rest_num -= give;
                        QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                        ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
                        DummyCard *dummy = new DummyCard(to_give);
                        room->obtainCard(receiver, dummy, false);
                        delete dummy;
                        if (rest_num == 0 || target->isKongcheng())
                            break;
                    }
                }*/
            }
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                target->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};

QiceCard::QiceCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool QiceCard::targetFixed() const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *QiceCard::validate(CardUseStruct &card_use) const
{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("qice");
    use_card->addSubcards(this->subcards);
    bool available = true;
    foreach(ServerPlayer *to, card_use.to)
        if (card_use.from->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(card_use.from);
    use_card->deleteLater();
    if (!available) return NULL;
    return use_card;
}

class Qice : public ViewAsSkill
{
public:
    Qice() : ViewAsSkill("qice")
    {
    }

    QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("qice", false);
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getHandcardNum())
            return NULL;

        const Card *c = Self->tag.value("qice").value<const Card *>();
        if (c) {
            QiceCard *card = new QiceCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->isKongcheng())
            return false;
        else
            return !player->hasUsed("QiceCard");
    }
};

class Zhiyu : public MasochismSkill
{
public:
    Zhiyu() : MasochismSkill("zhiyu")
    {
    }

    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        if (target->askForSkillInvoke(this, QVariant::fromValue(damage))) {
            target->drawCards(1, objectName());

            Room *room = target->getRoom();
            room->broadcastSkillInvoke(objectName());

            if (target->isKongcheng())
                return;
            room->showAllCards(target);

            QList<const Card *> cards = target->getHandcards();
            Card::Color color = cards.first()->getColor();
            bool same_color = true;
            foreach (const Card *card, cards) {
                if (card->getColor() != color) {
                    same_color = false;
                    break;
                }
            }

            if (same_color && damage.from && damage.from->canDiscard(damage.from, "h"))
                room->askForDiscard(damage.from, objectName(), 1, 1);
        }
    }
};

DangxianCard::DangxianCard()
{
    mute = true;
}

bool DangxianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shensu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void DangxianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (!source->isKongcheng())
        source->throwAllHandCards();
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_dangxian");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}


class DangxianViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DangxianViewAsSkill() : ZeroCardViewAsSkill("dangxian")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@dangxian");
    }

    const Card *viewAs() const
    {
        return new DangxianCard;
    }
};

class Dangxian : public TriggerSkill
{
public:
    Dangxian() : TriggerSkill("dangxian")
    {
        events << EventPhaseStart;
        view_as_skill = new DangxianViewAsSkill;
    }
    
    bool triggerable(const ServerPlayer *liaohua) const
    {
        return TriggerSkill::triggerable(liaohua) && (!liaohua->isKongcheng() || liaohua->getMark("fuli")>0);
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &) const
    {
        if (liaohua->getPhase() == Player::Start) {
            foreach (const Card *card, liaohua->getHandcards()) {
                if (liaohua->isJilei(card))
                    return false;
            }
            room->askForUseCard(liaohua, "@@dangxian", "@dangxian");
        }
        return false;
    }
};

class Fuli : public TriggerSkill
{
public:
    Fuli() : TriggerSkill("fuli")
    {
        frequency = Wake;
        events << EnterDying;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }


    bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &) const
    {
        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$FuliAnimate", 3000);

        room->doSuperLightbox("liaohua", "fuli");

        room->addPlayerMark(liaohua, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(liaohua) && liaohua->getMark(objectName()) > 0) {
            int recover = 2 - liaohua->getHp();
            room->recover(liaohua, RecoverStruct(NULL, NULL, recover));
        }
        
        return false;
    }
};

class Zishou : public DrawCardsSkill
{
public:
    Zishou() : DrawCardsSkill("zishou")
    {

    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->askForSkillInvoke(objectName())) {
            Room *room = player->getRoom();
            room->broadcastSkillInvoke(objectName());

            room->setPlayerFlag(player, "zishou");

            QSet<QString> kingdomSet;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                kingdomSet.insert(p->getKingdom());

            return n + kingdomSet.count();
        }

        return n;
    }
};

class ZishouProhibit : public ProhibitSkill
{
public:
    ZishouProhibit() : ProhibitSkill("#zishou")
    {

    }

    bool isProhibited(const Player *from, const Player *, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard"))
            return false;

        if (from->hasFlag("zishou"))
            return !card->isKindOf("Peach") && !card->isKindOf("GodSalvation");

        return false;
    }
};

class Zongshi : public MaxCardsSkill
{
public:
    Zongshi() : MaxCardsSkill("zongshi")
    {
    }

    int getExtra(const Player *target) const
    {
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach(const Player *player, target->parent()->findChildren<const Player *>())
            {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(this))
            return extra;
        else
            return 0;
    }
};

GongqiCard::GongqiCard()
{
    mute = true;
    target_fixed = true;
}

void GongqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerFlag(source, "InfinityAttackRange");
    const Card *cd = Sanguosha->getCard(subcards.first());
    if (cd->isKindOf("EquipCard")) {
        room->broadcastSkillInvoke("gongqi", 2);
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(source))
            if (source->canDiscard(p, "he")) targets << p;
        if (!targets.isEmpty()) {
            ServerPlayer *to_discard = room->askForPlayerChosen(source, targets, "gongqi", "@gongqi-discard", true);
            if (to_discard)
                room->throwCard(room->askForCardChosen(source, to_discard, "he", "gongqi", false, Card::MethodDiscard), to_discard, source);
        }
    } else {
        room->broadcastSkillInvoke("gongqi", 1);
    }
}

class Gongqi : public OneCardViewAsSkill
{
public:
    Gongqi() : OneCardViewAsSkill("gongqi")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GongqiCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        GongqiCard *card = new GongqiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

JiefanCard::JiefanCard()
{
    mute = true;
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@rescue");
    ServerPlayer *target = targets.first();
    source->tag["JiefanTarget"] = QVariant::fromValue(target);
    room->broadcastSkillInvoke("jiefan");
    //room->doLightbox("$JiefanAnimate", 2500);
    room->doSuperLightbox("handang", "jiefan");

    foreach (ServerPlayer *player, room->getAllPlayers()) {
        if (player->isAlive() && player->inMyAttackRange(target))
            room->cardEffect(this, source, player);
    }
    source->tag.remove("JiefanTarget");
}

void JiefanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    ServerPlayer *target = effect.from->tag["JiefanTarget"].value<ServerPlayer *>();
    QVariant data = effect.from->tag["JiefanTarget"];
    if (target && !room->askForCard(effect.to, ".Weapon", "@jiefan-discard::" + target->objectName(), data))
        target->drawCards(1, "jiefan");
}

class Jiefan : public ZeroCardViewAsSkill
{
public:
    Jiefan() : ZeroCardViewAsSkill("jiefan")
    {
        frequency = Limited;
        limit_mark = "@rescue";
    }

    const Card *viewAs() const
    {
        return new JiefanCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@rescue") >= 1;
    }
};

AnxuCard::AnxuCard()
{
    mute = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() >= selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    QString resp  = QString("resp:%1").arg(to->objectName());
    if (room->askForSkillInvoke(from, "anxu-resp", resp)) {
        source->drawCards(1, "anxu");
        if (from->getGeneralName().contains("sunquan"))
            room->broadcastSkillInvoke("anxu", 2);
        else
            room->broadcastSkillInvoke("anxu", 1);
        int id = room->askForCardChosen(to, from, "h", "anxu", true);
        const Card *cd = Sanguosha->getCard(id);
        to->obtainCard(cd);
    }
}

class Anxu : public ZeroCardViewAsSkill
{
public:
    Anxu() : ZeroCardViewAsSkill("anxu")
    {
    }

    const Card *viewAs() const
    {
        return new AnxuCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("AnxuCard");
    }
};

class Zhuiyi : public TriggerSkill
{
public:
    Zhuiyi() : TriggerSkill("zhuiyi")
    {
        events << Death;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        QList<ServerPlayer *> targets = (death.damage && death.damage->from) ? room->getOtherPlayers(death.damage->from) :
            room->getAlivePlayers();

        if (targets.isEmpty())
            return false;

        QString prompt = "zhuiyi-invoke";
        if (death.damage && death.damage->from && death.damage->from != player)
            prompt = QString("%1x:%2").arg(prompt).arg(death.damage->from->objectName());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, false, true);

        if (target->getGeneralName().contains("sunquan"))
            room->broadcastSkillInvoke(objectName(), 2);
        else
            room->broadcastSkillInvoke(objectName(), 1);

        target->drawCards(3, objectName());
        room->recover(target, RecoverStruct(player), true);
        return false;
    }
};

//YJCM2013

class Jingce : public TriggerSkill
{
public:
    Jingce() : TriggerSkill("jingce")
    {
        events << EventPhaseEnd;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Play && player->getMark(objectName()) >= player->getHp()) {
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName());
                player->drawCards(2, objectName());
                room->setPlayerFlag(player, "JingceMaxcards");
            }
        }
        return false;
    }
};

class JingceRecord : public TriggerSkill
{
public:
    JingceRecord() : TriggerSkill("#jingce-record")
    {
        events << PreCardUsed << CardResponded << EventPhaseStart;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == PreCardUsed || triggerEvent == CardResponded) && player->getPhase() <= Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill)
                player->addMark("jingce");
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            player->setMark("jingce", 0);
        }
        return false;
    }
};

class JingceMaxCards : public MaxCardsSkill
{
public:
    JingceMaxCards() : MaxCardsSkill("#jingce-maxcard")
    {
    }

    int getExtra(const Player *target) const
    {
        if (target->hasFlag("JingceMaxcards"))
            return 1;
        else
            return 0;
    }
};

class Longyin : public TriggerSkill
{
public:
    Longyin() : TriggerSkill("longyin")
    {
        events << CardUsed;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target->getPhase() == Player::Play;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            ServerPlayer *guanping = room->findPlayerBySkillName(objectName());
            if (guanping && guanping->canDiscard(guanping, "he")
                && room->askForCard(guanping, "..", "@longyin", data, objectName())) {
                room->broadcastSkillInvoke(objectName(), use.card->isRed() ? 2 : 1);
                if (use.m_addHistory) {
                    room->addPlayerHistory(player, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
                if (use.card->isRed())
                    guanping->drawCards(1, objectName());
            }
        }
        return false;
    }
};

ExtraCollateralCard::ExtraCollateralCard()
{
}

bool ExtraCollateralCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
    if (!coll) return false;
    QStringList tos = Self->property("extra_collateral_current_targets").toString().split("+");

    if (targets.isEmpty())
        return !tos.contains(to_select->objectName())
        && !Self->isProhibited(to_select, coll) && coll->targetFilter(targets, to_select, Self);
    else
        return coll->targetFilter(targets, to_select, Self);
}

void ExtraCollateralCard::onUse(Room *, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.first();
    ServerPlayer *victim = card_use.to.last();

    killer->setFlags("ExtraCollateralTarget");
    killer->tag["collateralVictim"] = QVariant::fromValue(victim);
}

QiaoshuiCard::QiaoshuiCard()
{
}

bool QiaoshuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void QiaoshuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    bool success = source->pindian(targets.first(), "qiaoshui", NULL);
    if (success)
        source->setFlags("QiaoshuiSuccess");
    else
        room->setPlayerFlag(source, "qiaoshui");
}

class QiaoshuiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QiaoshuiViewAsSkill() : ZeroCardViewAsSkill("qiaoshui")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@qiaoshui");
    }

    const Card *viewAs() const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern.endsWith("!"))
            return new ExtraCollateralCard;
        else
            return new QiaoshuiCard;
    }
};

class QiaoshuiProhibit : public ProhibitSkill
{
public:
    QiaoshuiProhibit() : ProhibitSkill("#qiaoshui")
    {

    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard"))
            return false;

        if (from->hasFlag("qiaoshui"))
            return card->isKindOf("TrickCard") && from != to;

        return false;
    }
};

class Qiaoshui : public PhaseChangeSkill
{
public:
    Qiaoshui() : PhaseChangeSkill("qiaoshui")
    {
        view_as_skill = new QiaoshuiViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *jianyong) const
    {
        if (jianyong->getPhase() == Player::Play && !jianyong->isKongcheng()) {
            Room *room = jianyong->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(jianyong);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke)
                room->askForUseCard(jianyong, "@@qiaoshui", "@qiaoshui-card", 1);
        }

        return false;
    }
};

class QiaoshuiUse : public TriggerSkill
{
public:
    QiaoshuiUse() : TriggerSkill("#qiaoshui-use")
    {
        events << PreCardUsed;
        view_as_skill = new QiaoshuiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *jianyong, QVariant &data) const
    {
        if (!jianyong->hasFlag("QiaoshuiSuccess")) return false;

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() || use.card->isKindOf("BasicCard")) {
            if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
                return false;

            QList<ServerPlayer *> available_targets;
            if (!use.card->isKindOf("AOE") && !use.card->isKindOf("GlobalEffect")) {
                room->setPlayerFlag(jianyong, "QiaoshuiExtraTarget");
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(jianyong, p, use.card)) continue;
                    if (use.card->targetFixed()) {
                        if (!use.card->isKindOf("Peach") || p->isWounded())
                            available_targets << p;
                    } else {
                        if (use.card->targetFilter(QList<const Player *>(), p, jianyong))
                            available_targets << p;
                    }
                }
                room->setPlayerFlag(jianyong, "-QiaoshuiExtraTarget");
            }
            QStringList choices;
            choices << "cancel";
            if (use.to.length() > 1) choices.prepend("remove");
            if (!available_targets.isEmpty()) choices.prepend("add");
            if (choices.length() == 1) return false;

            QString choice = room->askForChoice(jianyong, "qiaoshui", choices.join("+"), data);
            if (choice == "cancel") {
                return false;
            }
            jianyong->setFlags("-QiaoshuiSuccess");
            if (choice == "add") {
                ServerPlayer *extra = NULL;
                if (!use.card->isKindOf("Collateral"))
                    extra = room->askForPlayerChosen(jianyong, available_targets, "qiaoshui", "@qiaoshui-add:::" + use.card->objectName());
                else {
                    QStringList tos;
                    foreach(ServerPlayer *t, use.to)
                        tos.append(t->objectName());
                    room->setPlayerProperty(jianyong, "extra_collateral", use.card->toString());
                    room->setPlayerProperty(jianyong, "extra_collateral_current_targets", tos.join("+"));
                    room->askForUseCard(jianyong, "@@qiaoshui!", "@qiaoshui-add:::collateral");
                    room->setPlayerProperty(jianyong, "extra_collateral", QString());
                    room->setPlayerProperty(jianyong, "extra_collateral_current_targets", QString("+"));
                    foreach (ServerPlayer *p, room->getOtherPlayers(jianyong)) {
                        if (p->hasFlag("ExtraCollateralTarget")) {
                            p->setFlags("-ExtraCollateralTarget");
                            extra = p;
                            break;
                        }
                    }
                    if (extra == NULL) {
                        extra = available_targets.at(qrand() % available_targets.length() - 1);
                        QList<ServerPlayer *> victims;
                        foreach (ServerPlayer *p, room->getOtherPlayers(extra)) {
                            if (extra->canSlash(p)
                                && (!(p == jianyong && p->hasSkill("kongcheng") && p->isLastHandCard(use.card, true)))) {
                                victims << p;
                            }
                        }
                        Q_ASSERT(!victims.isEmpty());
                        extra->tag["collateralVictim"] = QVariant::fromValue((victims.at(qrand() % victims.length() - 1)));
                    }
                }
                use.to.append(extra);
                room->sortByActionOrder(use.to);

                LogMessage log;
                log.type = "#QiaoshuiAdd";
                log.from = jianyong;
                log.to << extra;
                log.card_str = use.card->toString();
                log.arg = "qiaoshui";
                room->sendLog(log);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, jianyong->objectName(), extra->objectName());

                if (use.card->isKindOf("Collateral")) {
                    ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                    if (victim) {
                        LogMessage log;
                        log.type = "#CollateralSlash";
                        log.from = jianyong;
                        log.to << victim;
                        room->sendLog(log);
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                    }
                }
            } else {
                ServerPlayer *removed = room->askForPlayerChosen(jianyong, use.to, "qiaoshui", "@qiaoshui-remove:::" + use.card->objectName());
                use.to.removeOne(removed);

                LogMessage log;
                log.type = "#QiaoshuiRemove";
                log.from = jianyong;
                log.to << removed;
                log.card_str = use.card->toString();
                log.arg = "qiaoshui";
                room->sendLog(log);
            }
        }
        data = QVariant::fromValue(use);

        return false;
    }
};

class QiaoshuiTargetMod : public TargetModSkill
{
public:
    QiaoshuiTargetMod() : TargetModSkill("#qiaoshui-target")
    {
        frequency = NotFrequent;
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("QiaoshuiExtraTarget"))
            return 1000;
        else
            return 0;
    }
};

class Zongshih : public TriggerSkill
{
public:
    Zongshih() : TriggerSkill("zongshih")
    {
        events << Pindian;
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        const Card *to_obtain = NULL;
        ServerPlayer *jianyong = NULL;
        if (TriggerSkill::triggerable(pindian->from)) {
            jianyong = pindian->from;
            if (pindian->from_number > pindian->to_number)
                to_obtain = pindian->to_card;
            else
                to_obtain = pindian->from_card;
        } else if (TriggerSkill::triggerable(pindian->to)) {
            jianyong = pindian->to;
            if (pindian->from_number < pindian->to_number)
                to_obtain = pindian->from_card;
            else
                to_obtain = pindian->to_card;
        }
        if (jianyong && to_obtain && room->getCardPlace(to_obtain->getEffectiveId()) == Player::PlaceTable
            && room->askForSkillInvoke(jianyong, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            jianyong->obtainCard(to_obtain);
        }

        return false;
    }
};

XiansiCard::XiansiCard()
{
    will_throw = false;
}

bool XiansiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.length() < 2 && !to_select->isNude();
}

void XiansiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    source->addToPile("counter", subcards.first());
    foreach(ServerPlayer *target, targets) {
        int id = room->askForCardChosen(source, target, "he", "xiansi", false, Card::MethodDiscard);
        room->throwCard(id, target, source);
    }
}


/*void XiansiCard::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isNude()) return;
    Room *room = effect.from->getRoom();
    int id = room->askForCardChosen(effect.from, effect.to, "he", "xiansi", false, Card::MethodDiscard);
    room->throwCard(id, effect.to, effect.from);
}*/

class XiansiViewAsSkill : public OneCardViewAsSkill
{
public:
    XiansiViewAsSkill() : OneCardViewAsSkill("xiansi")
    {
        filter_pattern = ".!";
        response_pattern = "@@xiansi";
    }
    
    /*bool isEnabledAtResponse(const QString &pattern) const
    {
        return pattern == "@@xiansi";
    }*/
    
    const Card *viewAs(const Card *originalcard) const
    {
        XiansiCard *card = new XiansiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

class Xiansi : public TriggerSkill
{
public:
    Xiansi() : TriggerSkill("xiansi")
    {
        events << EventPhaseStart;
        view_as_skill = new XiansiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Play && !player->isNude())
            room->askForUseCard(player, "@@xiansi", "@xiansi-card");
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        int index = qrand() % 2 + 1;
        if (card->isKindOf("Slash"))
            index += 2;
        return index;
    }
};

class XiansiAttach : public TriggerSkill
{
public:
    XiansiAttach() : TriggerSkill("#xiansi-attach")
    {
        events << GameStart << EventAcquireSkill << Debut;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == GameStart && TriggerSkill::triggerable(player))
            || (triggerEvent == EventAcquireSkill && data.toString() == "xiansi")) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->hasSkill("xiansi_slash"))
                    room->attachSkillToPlayer(p, "xiansi_slash");
            }
        } else if (triggerEvent == Debut) {
            QList<ServerPlayer *> liufengs = room->findPlayersBySkillName("xiansi");
            foreach (ServerPlayer *liufeng, liufengs) {
                if (player != liufeng && !player->hasSkill("xiansi_attach")) {
                    room->attachSkillToPlayer(player, "xiansi_attach");
                    break;
                }
            }
        }
        return false;
    }
};

XiansiSlashCard::XiansiSlashCard()
{
    m_skillName = "xiansi_slash";
}

bool XiansiSlashCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Player *liufeng = NULL;
    foreach (const Player *p, targets) {
        if (!p->getPile("counter").isEmpty()) {
            liufeng = p;
            break;
        }
    }

    if (liufeng == NULL)
        return false;

    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->addSpecificAssignee(liufeng);
    bool feasible = slash->targetsFeasible(targets, Self);
    delete slash;
    return feasible;
}

bool XiansiSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(Card::NoSuit, 0);
    if (targets.isEmpty()) {
        bool filter = to_select->hasSkill("xiansi") && to_select->getPile("counter").length() >= 1
            && slash->targetFilter(QList<const Player *>(), to_select, Self);
        delete slash;
        return filter;
    } else {
        slash->addSpecificAssignee(targets.first());
        bool filter = slash->targetFilter(targets, to_select, Self);
        delete slash;
        return filter;
    }
    return false;
}

const Card *XiansiSlashCard::validate(CardUseStruct &cardUse) const
{
    Room *room = cardUse.from->getRoom();

    ServerPlayer *source = cardUse.from;

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "xiansi", QString());
    room->throwCard(this, reason, NULL);

    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->setSkillName("_xiansi");

    QList<ServerPlayer *> targets = cardUse.to;
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, slash, false))
            cardUse.to.removeOne(target);
    }
    if (cardUse.to.length() > 0)
        return slash;
    else {
        delete slash;
        return NULL;
    }
}

class XiansiSlashViewAsSkill : public ViewAsSkill
{
public:
    XiansiSlashViewAsSkill() : ViewAsSkill("xiansi_slash")
    {
        attached_lord_skill = true;
        expand_pile = "%counter";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && canSlashLiufeng(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash"
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && canSlashLiufeng(player);
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 1)
            return false;

        foreach (const Player *p, Self->getAliveSiblings()) {
            if (!p->getPile("counter").isEmpty()) {
                return p->getPile("counter").contains(to_select->getId());
            }
        }

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 1) {
            XiansiSlashCard *xs = new XiansiSlashCard;
            xs->addSubcards(cards);
            return xs;
        }

        return NULL;
    }

private:
    static bool canSlashLiufeng(const Player *player)
    {
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->getPile("counter").isEmpty()) {
                if (slash->targetFilter(QList<const Player *>(), p, player)) {
                    delete slash;
                    return true;
                }
            }
        }
        delete slash;
        return false;
    }
};

ZongxuanCard::ZongxuanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    target_fixed = true;
}

void ZongxuanCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QVariantList subcardsList;
    foreach(int id, subcards)
        subcardsList << id;
    source->tag["zongxuan"] = QVariant::fromValue(subcardsList);
}

class ZongxuanViewAsSkill : public ViewAsSkill
{
public:
    ZongxuanViewAsSkill() : ViewAsSkill("zongxuan")
    {
        response_pattern = "@@zongxuan";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QStringList zongxuan = Self->property("zongxuan").toString().split("+");
        foreach (QString id, zongxuan) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return true;
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;

        ZongxuanCard *card = new ZongxuanCard;
        card->addSubcards(cards);
        return card;
    }
};

class Zongxuan : public TriggerSkill
{
public:
    Zongxuan() : TriggerSkill("zongxuan")
    {
        events << BeforeCardsMove;
        view_as_skill = new ZongxuanViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {

            int i = 0;
            QList<int> zongxuan_card;
            foreach (int card_id, move.card_ids) {
                if (room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)) {
                    zongxuan_card << card_id;
                }
                i++;
            }
            if (zongxuan_card.isEmpty())
                return false;

            room->setPlayerProperty(player, "zongxuan", IntList2StringList(zongxuan_card).join("+"));
            do {
                if (!room->askForUseCard(player, "@@zongxuan", "@zongxuan-put")) break;

                QList<int> subcards;
                QVariantList subcards_variant = player->tag["zongxuan"].toList();
                if (!subcards_variant.isEmpty()) {
                    subcards = VariantList2IntList(subcards_variant);
                    QStringList zongxuan = player->property("zongxuan").toString().split("+");
                    foreach (int id, subcards) {
                        zongxuan_card.removeOne(id);
                        zongxuan.removeOne(QString::number(id));
                        room->setPlayerProperty(player, "zongxuan", zongxuan.join("+"));
                        QList<int> _id;
                        _id << id;
                        move.removeCardIds(_id);
                        data = QVariant::fromValue(move);
                        room->setPlayerProperty(player, "zongxuan_move", QString::number(id)); // For UI to translate the move reason
                        room->moveCardTo(Sanguosha->getCard(id), player, NULL, Player::DrawPile, move.reason, true);
                        if (!player->isAlive())
                            break;
                    }
                }
                player->tag.remove("zongxuan");
            } while (!zongxuan_card.isEmpty());
        }
        return false;
    }
};

class Zhiyan : public PhaseChangeSkill
{
public:
    Zhiyan() : PhaseChangeSkill("zhiyan")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish)
            return false;

        Room *room = target->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(target, room->getAlivePlayers(), objectName(), "zhiyan-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            const Card *card = Sanguosha->getCard(ids.first());
            room->obtainCard(to, card, false);
            if (!to->isAlive())
                return false;
            room->showCard(to, ids.first());

            if (card->isKindOf("EquipCard")) {
                room->recover(to, RecoverStruct(target));
                if (to->isAlive() && !to->isCardLimited(card, Card::MethodUse))
                    room->useCard(CardUseStruct(card, to, to));
            }
        }
        return false;
    }
};

class Juece : public PhaseChangeSkill
{
public:
    Juece() : PhaseChangeSkill("juece")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> kongcheng_players;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->isKongcheng())
                kongcheng_players << p;
        }
        if (kongcheng_players.isEmpty()) return false;

        ServerPlayer *to_damage = room->askForPlayerChosen(target, kongcheng_players, objectName(),
            "@juece", true, true);
        if (to_damage) {
            room->broadcastSkillInvoke(objectName());
            room->damage(DamageStruct(objectName(), target, to_damage));
        }
        return false;
    }
};

MiejiCard::MiejiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool MiejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void MiejiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "mieji", QString());
    room->moveCardTo(this, effect.from, NULL, Player::DrawPile, reason, true);
    
    /*
    int trick_num = 0, nontrick_num = 0;
    foreach (const Card *c, effect.to->getCards("he")) {
        if (effect.to->canDiscard(effect.to, c->getId())) {
            if (c->isKindOf("TrickCard"))
                trick_num++;
            else
                nontrick_num++;
        }
    }
    bool discarded = room->askForDiscard(effect.to, "mieji", 1, qMin(1, trick_num), nontrick_num > 1, true, "@mieji-trick", "TrickCard");
    if (trick_num == 0 || !discarded)
        room->askForDiscard(effect.to, "mieji", 2, 2, false, true, "@mieji-nontrick", "^TrickCard");
        */

    QList<const Card *> cards = effect.to->getCards("he");
    QList<const Card *> cardsCopy = cards;

    foreach (const Card *c, cardsCopy) {
        if (effect.to->isJilei(c))
            cards.removeOne(c);
    }

    if (cards.length() == 0)
        return;

    bool instanceDiscard = false;
    int instanceDiscardId = -1;

    if (cards.length() == 1)
        instanceDiscard = true;
    else if (cards.length() == 2) {
        bool bothTrick = true;
        int trickId = -1;
        
        foreach (const Card *c, cards) {
            if (c->getTypeId() != Card::TypeTrick)
                bothTrick = false;
            else
                trickId = c->getId();
        }
        
        instanceDiscard = !bothTrick;
        instanceDiscardId = trickId;
    }

    if (instanceDiscard) {
        DummyCard d;
        if (instanceDiscardId == -1)
            d.addSubcards(cards);
        else
            d.addSubcard(instanceDiscardId);
        room->throwCard(&d, effect.to);
    } else if (!room->askForCard(effect.to, "@@miejidiscard!", "@mieji-discard")) {
        DummyCard d;
        qShuffle(cards);
        int trickId = -1;
        foreach (const Card *c, cards) {
            if (c->getTypeId() == Card::TypeTrick) {
                trickId = c->getId();
                break;
            }
        }
        if (trickId != -1)
            d.addSubcard(trickId);
        else {
            d.addSubcard(cards.first());
            d.addSubcard(cards.last());
        }

        room->throwCard(&d, effect.to);
    }
}

class Mieji : public OneCardViewAsSkill
{
public:
    Mieji() : OneCardViewAsSkill("mieji")
    {
        filter_pattern = "TrickCard|black";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MiejiCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        MiejiCard *card = new MiejiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class MiejiDiscard : public ViewAsSkill
{
public:
    MiejiDiscard() : ViewAsSkill("miejidiscard")
    {

    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@miejidiscard!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->isJilei(to_select))
            return false;

        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (selected.first()->getTypeId() == Card::TypeTrick)
                return false;
            else
                return to_select->getTypeId() != Card::TypeTrick;
        } else
            return false;

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        bool ok = false;
        if (cards.length() == 1)
            ok = cards.first()->getTypeId() == Card::TypeTrick;
        else if (cards.length() == 2) {
            ok = true;
            foreach (const Card *c, cards) {
                if (c->getTypeId() == Card::TypeTrick) {
                    ok = false;
                    break;
                }
            }
        }

        if (!ok)
            return NULL;

        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(cards);
        return dummy;
    }
};

class Fencheng : public ZeroCardViewAsSkill
{
public:
    Fencheng() : ZeroCardViewAsSkill("fencheng")
    {
        frequency = Limited;
        limit_mark = "@burn";
    }

    const Card *viewAs() const
    {
        return new FenchengCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@burn") >= 1;
    }
};

class FenchengMark : public TriggerSkill
{
public:
    FenchengMark() : TriggerSkill("#fencheng")
    {
        events << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        QStringList data_str = data.toString().split(":");
        if (data_str.length() != 3 || data_str.first() != "cardDiscard" || data_str.at(1) != "fencheng")
            return false;
        room->setTag("FenchengDiscard", data_str.last().split("+").length());
        return false;
    }
};

FenchengCard::FenchengCard()
{
    mute = true;
    target_fixed = true;
}

void FenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->removePlayerMark(source, "@burn");
    room->broadcastSkillInvoke("fencheng");
    //room->doLightbox("$FenchengAnimate", 3000);
    room->doSuperLightbox("liru", "fencheng");
    room->setTag("FenchengDiscard", 0);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("FenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive()) {
                room->cardEffect(this, source, player);
                room->getThread()->delay();
            }
        }
        source->setFlags("-FenchengUsing");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-FenchengUsing");
        throw triggerEvent;
    }
}

void FenchengCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    int length = room->getTag("FenchengDiscard").toInt() + 1;
    if (!effect.to->canDiscard(effect.to, "he") || effect.to->getCardCount(true) < length
        || !room->askForDiscard(effect.to, "fencheng", 1000, length, true, true, "@fencheng:::" + QString::number(length))) {
        room->setTag("FenchengDiscard", 0);
        room->damage(DamageStruct("fencheng", effect.from, effect.to, 2, DamageStruct::Fire));
    }
}

/*ZhuikongCard::ZhuikongCard()
{
    
}

bool ZhuikongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && to_select->getPhase() == Player::NotActive && !to_select->isKongcheng();
}

void ZhuikongCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    ServerPlayer *current = room->getCurrent();
    if (current && !current->isKongcheng()) {
        if (room->askForSkillInvoke(effect.to, "zhuikong-pindian", "pindian")) {
            room->broadcastSkillInvoke("zhuikong", 2);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.to->objectName(), current->objectName());
            current->setFlags("ZhuikongPindianTarget");
            effect.to->pindian(current, "zhuikong", NULL);
            current->setFlags("-ZhuikongPindianTarget");
        }
        else {
            room->broadcastSkillInvoke("zhuikong",3);
        }
    }
}

class ZhuikongViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ZhuikongViewAsSkill() : ZeroCardViewAsSkill("zhuikong")
    {
        //filter_pattern = ".!";
        response_pattern == "@@zhuikong";
    }

    /*const Card *viewAs(const Card *originalcard) const
    {
        ZhuikongCard *card = new ZhuikongCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }

    const Card *viewAs() const
    {
        return new ZhuikongCard;;
    }
};*/

class Zhuikong : public TriggerSkill
{
public:
    Zhuikong() : TriggerSkill("zhuikong")
    {
        events << EventPhaseStart ;
        //view_as_skill = new ZhuikongViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                && player != fuhuanghou && fuhuanghou->isWounded() && !fuhuanghou->isNude()) {
                if (room->askForCard(fuhuanghou, ".", "@zhuikong", QVariant(), objectName())) {
                    room->setPlayerFlag(fuhuanghou, "zhuikong");
                    QList<ServerPlayer *> targets;
                    foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (p != fuhuanghou && !p->isKongcheng())
                            targets << p;
                    }
                    ServerPlayer *target = room->askForPlayerChosen(fuhuanghou, targets, "zhuikong", "@zhuikong", true);
                    if (room->askForSkillInvoke(target, "zhuikong-pindian", "pindian")) {
                        room->broadcastSkillInvoke("zhuikong", 2);
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), player->objectName());
                        player->setFlags("ZhuikongPindianTarget");
                        target->pindian(player, "zhuikong", NULL);
                        player->setFlags("-ZhuikongPindianTarget");
                    }
                    else {
                        room->broadcastSkillInvoke("zhuikong", 3);
                    }

                }
                //room->askForUseCard(fuhuanghou, "@@zhuikong", "@zhuikong-card", -1, Card::MethodDiscard);
            }
        }
        return false;
    }
    
    /*&int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }*/
};

class ZhuikongResult : public TriggerSkill
{
public:
    ZhuikongResult() : TriggerSkill("#zhuikong-result")
    {
        events << Pindian;
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
        if (pindian->reason != "zhuikong" || pindian->from_number == pindian->to_number)
            return false;

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        if (winner->canSlash(loser, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_zhuikong");
            ServerPlayer *current = room->getCurrent();
            if (winner == current && room->askForSkillInvoke(winner, "zhuikong-extra", "extra")) {
                QList <ServerPlayer *> targets = { loser };
                foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
                    if (fuhuanghou->hasFlag("zhuikong")) {
                        targets << fuhuanghou;
                        room->setPlayerFlag(fuhuanghou, "-zhuikong");
                    }
                }
                room->useCard(CardUseStruct(slash, winner, targets));
            }
            else {
                room->useCard(CardUseStruct(slash, winner, loser));
            }
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

class Qiuyuan : public TriggerSkill
{
public:
    Qiuyuan() : TriggerSkill("qiuyuan")
    {
        events << SlashProceed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.slash->hasFlag("qiuyuan"))
            effect.slash->setFlags("-qiuyuan");
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            room->setPlayerMark(p, "qiuyuan", 0);
            if (p != effect.from)
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "qiuyuan-invoke", true, true);
        if (target) {
            effect.slash->setFlags("qiuyuan");
            if (target->getGeneralName().contains("fuwan") || target->getGeneral2Name().contains("fuwan"))
                room->broadcastSkillInvoke("qiuyuan", 2);
            else
                room->broadcastSkillInvoke("qiuyuan", 1);
            const Card *card = NULL;
            if (!target->isKongcheng())
                card = room->askForExchange(target, objectName(), 1, 1, false, "@qiuyuan-give:" + player->objectName(), true);
            
            if (!card) {
                room->setPlayerMark(target, "qiuyuan", 2);                
            } else {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                reason.m_playerId = player->objectName();
                room->moveCardTo(card, target, player, Player::PlaceHand, reason);
                delete card;
                room->setPlayerMark(target, "qiuyuan", 1);
            }
        }
        return false;
    }
};

class QiuyuanResult : public TriggerSkill
{
public:
    QiuyuanResult() : TriggerSkill("#qiuyuan-result")
    {
        events << SlashMissed << SlashHit;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("qiuyuan")) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getMark("qiuyuan") == 1)
                        p->drawCards(2, "qiuyuan");
                }
            }
        }
        else if (triggerEvent == SlashHit) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("qiuyuan")) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getMark("qiuyuan") == 2)
                        room->loseHp(p, 1);
                }
            }
        }
        return false;
    }
};

LishouCard::LishouCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LishouCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "lishou", QString());
    room->obtainCard(effect.to, this, reason);
    room->damage(DamageStruct("lishou", effect.from, effect.to, 1));
    if (effect.to->isAlive()) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (!p->isNude())
                targets << p;
        }

        QStringList choicelist;
        if (!effect.to->faceUp() || effect.to->isChained())
            choicelist << "reset";
        if (!targets.isEmpty()) 
            choicelist << "obtain";
        if (!choicelist.isEmpty()) {
            QString choice = room->askForChoice(effect.to, "lishou", choicelist.join("+"));

            if (choice == "obtain") {
                ServerPlayer *target = room->askForPlayerChosen(effect.to, targets, "lishou");
                int card_id = room->askForCardChosen(effect.to, target, "he", "lishou");
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.to->objectName());
                Card *card = Sanguosha->getCard(card_id);
                room->obtainCard(effect.to, card, reason, room->getCardPlace(card_id) != Player::PlaceHand);
            } else if (choice == "reset") {
                if (effect.to->isChained())
                    room->setPlayerProperty(effect.to, "chained", false);
                if (!effect.to->faceUp())
                    effect.to->turnOver();
            }
        }
    }
}

class Lishou : public OneCardViewAsSkill
{
public:
    Lishou() : OneCardViewAsSkill("lishou")
    {
        filter_pattern = "^BasicCard|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LishouCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        LishouCard *lishouCard = new LishouCard;
        lishouCard->addSubcard(originalCard);

        return lishouCard;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->getTypeId() == Card::TypeTrick)
            return -2;
        else
            return -1;
    }
};

//YJCM2014

SidiCard::SidiCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void SidiCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    room->throwCard(this, NULL);
}

class SidiVS : public OneCardViewAsSkill
{
public:
    SidiVS() : OneCardViewAsSkill("sidi")
    {
        response_pattern = "@@sidi";
        filter_pattern = ".|.|.|sidi_pile";
        expand_pile = "sidi_pile";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        SidiCard *sd = new SidiCard;
        sd->addSubcard(originalCard);
        return sd;
    }
};

class Sidi : public TriggerSkill
{
public:
    Sidi() : TriggerSkill("sidi")
    {
        events << CardResponded << EventPhaseStart << EventPhaseChanging;
        //frequency = Frequent;
        view_as_skill = new SidiVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerMark(player, "sidi", 0);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (TriggerSkill::triggerable(p) && (p == player || p->getPhase() != Player::NotActive)
                        && room->askForSkillInvoke(p, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName(), 1);
                        QList<int> ids = room->getNCards(1, false); // For UI
                        CardsMoveStruct move(ids, p, Player::PlaceTable,
                            CardMoveReason(CardMoveReason::S_REASON_TURNOVER, p->objectName(), "sidi", QString()));
                        room->moveCardsAtomic(move, true);
                        p->addToPile("sidi_pile", ids);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->getPhase() != Player::Play) return false;
                if (TriggerSkill::triggerable(p) && p->getPile("sidi_pile").length() > 0 && room->askForUseCard(p, "@@sidi", "sidi_remove:remove", -1, Card::MethodNone))
                    room->addPlayerMark(player, "sidi");
            }
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class SidiTargetMod : public TargetModSkill
{
public:
    SidiTargetMod() : TargetModSkill("#sidi-target")
    {
    }

    int getResidueNum(const Player *from, const Card *card) const
    {
        return card->isKindOf("Slash") ? -from->getMark("sidi") : 0;
    }
};

class SidiClear : public DetachEffectSkill
{
public:
    SidiClear() : DetachEffectSkill("sidi", "sidi_pile")
    {
    }
};

class Benxi : public TriggerSkill
{
public:
    Benxi() : TriggerSkill("benxi")
    {
        events << EventPhaseChanging << PreCardUsed << CardUsed << CardResponded << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
        // global = true; // forgotten? @para
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(player, "@benxi", 0);
                room->setPlayerMark(player, "benxi", 0);
            }
        } else if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed) 
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill) {
                player->addMark("benxi");
                if (TriggerSkill::triggerable(player))
                    room->setPlayerMark(player, "@benxi", player->getMark("benxi"));
            }
        } else if (triggerEvent == CardUsed) { 
            const Card *card = data.value<CardUseStruct>().card;
            if (TriggerSkill::triggerable(player) && card->isKindOf("Slash") && isAllAdjacent(player, card)) {
                CardUseStruct use = data.value<CardUseStruct>();
                QList<ServerPlayer *> available_targets;
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (room->isProhibited(player, p, card)) continue;
                    if (card->targetFixed()) {
                        available_targets << p;
                    }
                    else {
                        if (card->targetFilter(QList<const Player *>(), p, player))
                            available_targets << p;
                    }
                }
                ServerPlayer *extra = room->askForPlayerChosen(player, available_targets, "benxi", "@benxi-add", true);
                if (extra){
                    use.to.append(extra);
                    room->sortByActionOrder(use.to);
                    data = QVariant::fromValue(use);
                }
            }
        } else if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
            QString name = data.toString();
            if (name != objectName()) return false;
            int num = (triggerEvent == EventAcquireSkill) ? player->getMark("benxi") : 0;
            room->setPlayerMark(player, "@benxi", num);
        }
        return false;
    }

private:
    bool isAllAdjacent(const Player *from, const Card *card) const
    {
        int rangefix = 0;
        if (card->isVirtualCard() && from->getOffensiveHorse()
            && card->getSubcards().contains(from->getOffensiveHorse()->getEffectiveId()))
            rangefix = 1;
        foreach(const Player *p, from->getAliveSiblings()) {
            if (from->distanceTo(p, rangefix) != 1)
                return false;
        }
        return true;
    }
};

// the part of Armor ignorance is coupled in Player::hasArmorEffect

/*class BenxiTargetMod : public TargetModSkill
{
public:
    BenxiTargetMod() : TargetModSkill("#benxi-target")
    {
    }

    int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("benxi") && isAllAdjacent(from, card))
            return 1;
        else
            return 0;
    }

private:
    bool isAllAdjacent(const Player *from, const Card *card) const
    {
        int rangefix = 0;
        if (card->isVirtualCard() && from->getOffensiveHorse()
            && card->getSubcards().contains(from->getOffensiveHorse()->getEffectiveId()))
            rangefix = 1;
        int aboveonenum = 0;
        foreach (const Player *p, from->getAliveSiblings()) {
            if (from->distanceTo(p, rangefix) != 1)
                aboveonenum++;
        }
        return aboveonenum <= 1;
    }
};*/

class BenxiDistance : public DistanceSkill
{
public:
    BenxiDistance() : DistanceSkill("#benxi-dist")
    {
    }

    int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill("benxi") && from->getPhase() != Player::NotActive)
            return -from->getMark("@benxi");         // bug will be caused if written as "benxi" 
        return 0;
    }
};

class Qiangzhi : public TriggerSkill
{
public:
    Qiangzhi() : TriggerSkill("qiangzhi")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::Play;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            player->setMark(objectName(), 0);
            if (TriggerSkill::triggerable(player)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isKongcheng())
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "qiangzhi-invoke", true, true);
                if (target) {
                    room->broadcastSkillInvoke(objectName(), 1);
                    int id = room->askForCardChosen(player, target, "h", objectName());
                    room->showCard(target, id);
                    player->setMark(objectName(), static_cast<int>(Sanguosha->getCard(id)->getTypeId()));
                }
            }
        } else if (player->getMark(objectName()) > 0) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && static_cast<int>(card->getTypeId()) == player->getMark(objectName())
                && room->askForSkillInvoke(player, objectName(), data)) {
                if (!player->hasSkill(this)) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                }
                room->broadcastSkillInvoke(objectName(), 2);
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class Xiantu : public TriggerSkill
{
public:
    Xiantu() : TriggerSkill("xiantu")
    {
        events << EventPhaseStart << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                p->setFlags("-XiantuInvoked");
                if (!player->isAlive()) return false;
                if (TriggerSkill::triggerable(p) && room->askForSkillInvoke(p, objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    p->setFlags("XiantuInvoked");
                    p->drawCards(2, objectName());
                    if (p->isAlive() && player->isAlive()) {
                        if (!p->isNude()) {
                            int num = qMin(2, p->getCardCount(true));
                            const Card *to_give = room->askForExchange(p, objectName(), num, num, true,
                                QString("@xiantu-give::%1:%2").arg(player->objectName()).arg(num));
                            player->obtainCard(to_give, false);
                            delete to_give;
                        }
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                QList<ServerPlayer *> zhangsongs;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("XiantuInvoked")) {
                        p->setFlags("-XiantuInvoked");
                        zhangsongs << p;
                    }
                }
                if (player->getMark("XiantuKill") > 0) {
                    player->setMark("XiantuKill", 0);
                    return false;
                }
                foreach (ServerPlayer *zs, zhangsongs) {
                    LogMessage log;
                    log.type = "#Xiantu";
                    log.from = player;
                    log.to << zs;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->loseHp(zs);
                }
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage && death.damage->from && death.damage->from->getPhase() == Player::Play)
                death.damage->from->addMark("XiantuKill");
        }
        return false;
    }
};

ShenxingCard::ShenxingCard()
{
    target_fixed = true;
}

void ShenxingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (source->isAlive())
        room->drawCards(source, 1, "shenxing");
}

class Shenxing : public ViewAsSkill
{
public:
    Shenxing() : ViewAsSkill("shenxing")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        ShenxingCard *card = new ShenxingCard;
        card->addSubcards(cards);
        return card;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getCardCount(true) >= 2 && player->canDiscard(player, "he");
    }
};

BingyiCard::BingyiCard()
{
}

bool BingyiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card::Color color = Card::Colorless;
    foreach (const Card *c, Self->getHandcards()) {
        if (color == Card::Colorless)
            color = c->getColor();
        else if (c->getColor() != color)
            return targets.isEmpty();
    }
    return targets.length() <= Self->getHandcardNum();
}

bool BingyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    Card::Color color = Card::Colorless;
    foreach (const Card *c, Self->getHandcards()) {
        if (color == Card::Colorless)
            color = c->getColor();
        else if (c->getColor() != color)
            return false;
    }
    return targets.length() < Self->getHandcardNum();
}

void BingyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->showAllCards(source);
    foreach(ServerPlayer *p, targets)
        room->drawCards(p, 1, "bingyi");
}

class BingyiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    BingyiViewAsSkill() : ZeroCardViewAsSkill("bingyi")
    {
        response_pattern = "@@bingyi";
    }

    const Card *viewAs() const
    {
        return new BingyiCard;
    }
};

class Bingyi : public PhaseChangeSkill
{
public:
    Bingyi() : PhaseChangeSkill("bingyi")
    {
        view_as_skill = new BingyiViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish || target->isKongcheng()) return false;
        target->getRoom()->askForUseCard(target, "@@bingyi", "@bingyi-card");
        return false;
    }
};

class Youdi : public PhaseChangeSkill
{
public:
    Youdi() : PhaseChangeSkill("youdi")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish || target->isNude()) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->canDiscard(target, "he")) players << p;
        }
        if (players.isEmpty()) return false;
        ServerPlayer *player = room->askForPlayerChosen(target, players, objectName(), "youdi-invoke", true, true);
        if (player) {
            room->broadcastSkillInvoke(objectName());
            int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            if (!Sanguosha->getCard(id)->isKindOf("Slash") && player->isAlive() && !player->isNude()) {
                QStringList choices;
                for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
                    if (player->getEquip(i) && !target->getEquip(i))
                        choices << QString::number(i);
                }
                choices << "obtain";
                QString choice = room->askForChoice(target, objectName(), choices.join("+"), QVariant::fromValue(target));
                if (choice == "obtain") {
                    int id2 = room->askForCardChosen(target, player, "he", "youdi_obtain");
                    room->obtainCard(target, id2, false);
                }
                else {
                    int index = choice.toInt();
                    const Card *card = player->getEquip(index);
                    room->moveCardTo(card, target, Player::PlaceEquip);
                }
            }
        }
        return false;
    }
};

HuaiyiCard::HuaiyiCard()
{
    target_fixed = true;
}

void HuaiyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->showAllCards(source);

    QList<int> blacks;
    QList<int> reds;
    foreach (const Card *c, source->getHandcards()) {
        if (c->isRed())
            reds << c->getId();
        else
            blacks << c->getId();
    }
    
    foreach (const Card *c, source->getEquips()) {
        if (c->isRed())
            reds << c->getId();
        else
            blacks << c->getId();
    }

    if (reds.isEmpty() || blacks.isEmpty())
        return;

    QString to_discard = room->askForChoice(source, "huaiyi", "black+red");
    QList<int> *pile = NULL;
    if (to_discard == "black")
        pile = &blacks;
    else
        pile = &reds;

    int n = pile->length();

    room->setPlayerMark(source, "huaiyi_num", n);

    DummyCard dm(*pile);
    room->throwCard(&dm, source);

    room->askForUseCard(source, "@@huaiyi", "@huaiyi:::" + QString::number(n), -1, Card::MethodNone);
}

HuaiyiSnatchCard::HuaiyiSnatchCard()
{
    handling_method = Card::MethodNone;
    m_skillName = "_huaiyi";
}

bool HuaiyiSnatchCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int n = Self->getMark("huaiyi_num");
    if (targets.length() >= n)
        return false;

    if (to_select == Self)
        return false;

    if (to_select->isNude())
        return false;
    
    QSet<QString> kingdoms;
    foreach(const Player *p, targets)
        kingdoms << p->getKingdom();
    return !kingdoms.contains(to_select->getKingdom());
}

void HuaiyiSnatchCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    QList<ServerPlayer *> to = card_use.to;

    room->sortByActionOrder(to);

    foreach (ServerPlayer *p, to) {
        int id = room->askForCardChosen(player, p, "he", "huaiyi");
        player->obtainCard(Sanguosha->getCard(id), false);
    }

    if (to.length() >= 2)
        room->loseHp(player);
}

class Huaiyi : public ZeroCardViewAsSkill
{
public:
    Huaiyi() : ZeroCardViewAsSkill("huaiyi")
    {

    }

    const Card *viewAs() const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@huaiyi")
            return new HuaiyiSnatchCard;
        else
            return new HuaiyiCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("HuaiyiCard");
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@huaiyi";
    }
};

YJCMPackage::YJCMPackage()
    : Package("YJCM")
{
    General *caozhi = new General(this, "caozhi", "wei", 3); // YJ 001
    caozhi->addSkill(new Luoying);
    caozhi->addSkill(new Jiushi);
    caozhi->addSkill(new JiushiFlip);
    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *chengong = new General(this, "chengong", "qun", 3); // YJ 002
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiProtect);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);
    related_skills.insertMulti("zhichi", "#zhichi-protect");
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *fazheng = new General(this, "fazheng", "shu", 3); // YJ 003
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);

    General *lingtong = new General(this, "lingtong", "wu"); // YJ 005
    lingtong->addSkill(new Xuanfeng);

    General *masu = new General(this, "masu", "shu", 3); // YJ 006
    masu->addSkill(new Xinzhan);
    masu->addSkill(new Huilei);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false); // YJ 007
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *yujin = new General(this, "yujin", "wei"); // YJ 010
    yujin->addSkill(new Jieyue);
    yujin->addSkill(new JieyueClear);
    related_skills.insertMulti("jieyue", "#jieyue-clear");
    
    General *zhonghui = new General(this, "zhonghui", "wei"); // YJ 012
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new QuanjiClear);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");
    related_skills.insertMulti("quanji", "#quanji");
    related_skills.insertMulti("quanji", "#quanji-clear");
    
    General *bulianshi = new General(this, "bulianshi", "wu", 3, false); // YJ 101
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *handang = new General(this, "handang", "wu"); // YJ 105
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *liaohua = new General(this, "liaohua", "shu"); // YJ 107
    liaohua->addSkill(new Dangxian);
    liaohua->addSkill(new Fuli);

    General *liubiao = new General(this, "liubiao", "qun", 3); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);
    liubiao->addSkill(new ZishouProhibit);
    related_skills.insertMulti("zishou", "#zishou");

    General *wangyi = new General(this, "wangyi", "wei", 3, false); // YJ 110
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3); // YJ 111
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    General *fuhuanghou = new General(this, "fuhuanghou", "qun", 3, false); // YJ 202
    fuhuanghou->addSkill(new Zhuikong);
    fuhuanghou->addSkill(new ZhuikongResult);
    fuhuanghou->addSkill(new Qiuyuan);
    fuhuanghou->addSkill(new QiuyuanResult);
    related_skills.insertMulti("zhuikong", "#zhuikong-result");
    related_skills.insertMulti("qiuyuan", "#qiuyuan-result");

    General *guohuai = new General(this, "guohuai", "wei"); // YJ 203
    guohuai->addSkill(new Jingce);
    guohuai->addSkill(new JingceRecord);
    guohuai->addSkill(new JingceMaxCards);
    related_skills.insertMulti("jingce", "#jingce-record");
    related_skills.insertMulti("jingce", "#jingce-maxcard");

    General *guanping = new General(this, "guanping", "shu", 4); // YJ 204
    guanping->addSkill(new Longyin);

    General *jianyong = new General(this, "jianyong", "shu", 3); // YJ 205
    jianyong->addSkill(new Qiaoshui);
    jianyong->addSkill(new QiaoshuiUse);
    jianyong->addSkill(new QiaoshuiTargetMod);
    jianyong->addSkill(new QiaoshuiProhibit);
    jianyong->addSkill(new Zongshih);
    related_skills.insertMulti("qiaoshui", "#qiaoshui-use");
    related_skills.insertMulti("qiaoshui", "#qiaoshui-target");
    related_skills.insertMulti("qiaoshui", "#qiaoshui");

    General *liru = new General(this, "liru", "qun", 3); // YJ 206
    liru->addSkill(new Juece);
    liru->addSkill(new Mieji);
    liru->addSkill(new Fencheng);
    liru->addSkill(new FenchengMark);
    related_skills.insertMulti("fencheng", "#fencheng");

    General *liufeng = new General(this, "liufeng", "shu"); // YJ 207
    liufeng->addSkill(new Xiansi);
    liufeng->addSkill(new XiansiAttach);
    related_skills.insertMulti("xiansi", "#xiansi-attach");

    General *manchong = new General(this, "manchong", "wei", 4); // YJ 208
    manchong->addSkill(new Lishou);

    General *yufan = new General(this, "yufan", "wu", 3); // YJ 210
    yufan->addSkill(new Zongxuan);
    yufan->addSkill(new Zhiyan);

    General *caozhen = new General(this, "caozhen", "wei"); // YJ 302
    caozhen->addSkill(new Sidi);
    caozhen->addSkill(new SidiTargetMod);
    caozhen->addSkill(new SidiClear);
    related_skills.insertMulti("sidi", "#sidi-target");
    related_skills.insertMulti("sidi", "#sidi-clear");
    
    General *guyong = new General(this, "guyong", "wu", 3); // YJ 304
    guyong->addSkill(new Shenxing);
    guyong->addSkill(new Bingyi);

    General *wuyi = new General(this, "wuyi", "shu"); // YJ 308
    wuyi->addSkill(new Benxi);
    //wuyi->addSkill(new BenxiTargetMod);
    wuyi->addSkill(new BenxiDistance);
    //related_skills.insertMulti("benxi", "#benxi-target");
    related_skills.insertMulti("benxi", "#benxi-dist");

    General *zhangsong = new General(this, "zhangsong", "shu", 3); // YJ 309
    zhangsong->addSkill(new Qiangzhi);
    zhangsong->addSkill(new Xiantu);

    General *zhuhuan = new General(this, "zhuhuan", "wu"); // YJ 311
    zhuhuan->addSkill(new Youdi);
    
    General *gongsunyuan = new General(this, "gongsunyuan", "qun"); // YJ 403
    gongsunyuan->addSkill(new Huaiyi);

    addMetaObject<JieyueCard>();
    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XuanhuoCard>();
    addMetaObject<XinzhanCard>();
    addMetaObject<PaiyiCard>();
    addMetaObject<QiceCard>();
    addMetaObject<DangxianCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
    addMetaObject<AnxuCard>();
    addMetaObject<QiaoshuiCard>();
    addMetaObject<XiansiCard>();
    addMetaObject<XiansiSlashCard>();
    addMetaObject<ZongxuanCard>();
    addMetaObject<MiejiCard>();
    addMetaObject<FenchengCard>();
    //addMetaObject<ZhuikongCard>();
    addMetaObject<LishouCard>();
    addMetaObject<ExtraCollateralCard>();
    addMetaObject<ShenxingCard>();
    addMetaObject<BingyiCard>();
    addMetaObject<SidiCard>();
    addMetaObject<HuaiyiCard>();
    addMetaObject<HuaiyiSnatchCard>();

    skills << new XiansiSlashViewAsSkill << new MiejiDiscard << new Paiyi;

}

ADD_PACKAGE(YJCM)

