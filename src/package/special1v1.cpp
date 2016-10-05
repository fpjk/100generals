#include "special1v1.h"
#include "general.h"
#include "standard.h"
#include "standard-equips.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "ai.h"
#include "settings.h"
#include "maneuvering.h"
#include "util.h"
#include "roomthread.h"
#include "thicket.h"
#include "clientplayer.h"

class KOFTuxi : public DrawCardsSkill
{
public:
    KOFTuxi() : DrawCardsSkill("koftuxi")
    {
    }

    int getDrawNum(ServerPlayer *zhangliao, int n) const
    {
        Room *room = zhangliao->getRoom();
        bool can_invoke = false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao)) {
            if (p->getHandcardNum() > zhangliao->getHandcardNum())
                targets << p;
        }
        if (!targets.isEmpty())
            can_invoke = true;

        if (can_invoke) {
            ServerPlayer *target = room->askForPlayerChosen(zhangliao, targets, objectName(), "koftuxi-invoke", true, true);
            if (target) {
                target->setFlags("KOFTuxiTarget");
                zhangliao->setFlags("koftuxi");
                return n - 1;
            } else {
                return n;
            }
        } else
            return n;
    }
};

class KOFTuxiAct : public TriggerSkill
{
public:
    KOFTuxiAct() : TriggerSkill("#koftuxi")
    {
        events << AfterDrawNCards;
    }

    bool triggerable(const ServerPlayer *player) const
    {
        return player != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &) const
    {
        if (!zhangliao->hasFlag("koftuxi")) return false;
        zhangliao->setFlags("-koftuxi");

        ServerPlayer *target = NULL;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao)) {
            if (p->hasFlag("KOFTuxiTarget")) {
                p->setFlags("-KOFTuxiTarget");
                target = p;
                break;
            }
        }
        if (!target) return false;

        int card_id = room->askForCardChosen(zhangliao, target, "h", "koftuxi");
        room->broadcastSkillInvoke("tuxi");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhangliao->objectName());
        room->obtainCard(zhangliao, Sanguosha->getCard(card_id), reason, false);

        return false;
    }
};

XiechanCard::XiechanCard()
{
}

bool XiechanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void XiechanCard::use(Room *room, ServerPlayer *xuchu, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(xuchu, "@twine");
    //room->doLightbox("$XiechanAnimate");
    room->doSuperLightbox("kof_xuchu", "xiechan");

    bool success = xuchu->pindian(targets.first(), "xiechan", NULL);
    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setSkillName("_xiechan");
    ServerPlayer *from = NULL, *to = NULL;
    if (success) {
        from = xuchu;
        to = targets.first();
    } else {
        from = targets.first();
        to = xuchu;
    }
    if (!from->isLocked(duel) && !from->isProhibited(to, duel))
        room->useCard(CardUseStruct(duel, from, to));
}

class Xiechan : public ZeroCardViewAsSkill
{
public:
    Xiechan() : ZeroCardViewAsSkill("xiechan")
    {
        frequency = Limited;
        limit_mark = "@twine";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@twine") > 0;
    }

    const Card *viewAs() const
    {
        return new XiechanCard;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Duel"))
            return -2;
        else
            return -1;
    }
};

class KOFQingguo : public OneCardViewAsSkill
{
public:
    KOFQingguo() : OneCardViewAsSkill("kofqingguo")
    {
        filter_pattern = ".|.|.|equipped";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "jink" && !player->getEquips().isEmpty();
    }
};

class KOFSuzi : public TriggerSkill
{
public:
    KOFSuzi() : TriggerSkill("kofsuzi")
    {
        events << BuryVictim;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isNude())
            return false;
        ServerPlayer *xiahouyuan = room->findPlayerBySkillName(objectName());
        DeathStruct death = data.value<DeathStruct>();
        if (!xiahouyuan || xiahouyuan == death.who)
            return false;
        if (room->askForSkillInvoke(xiahouyuan, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());

            DummyCard *dummy = new DummyCard(player->handCards());
            QList <const Card *> equips = player->getEquips();
            foreach(const Card *card, equips)
                dummy->addSubcard(card);

            if (dummy->subcardsLength() > 0) {
                CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, xiahouyuan->objectName());
                room->obtainCard(xiahouyuan, dummy, reason, false);
            }
            delete dummy;
        }
        return false;
    }
};

class Huwei : public TriggerSkill
{
public:
    Huwei() : TriggerSkill("huwei")
    {
        events << Debut;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        Drowning *drowning = new Drowning(Card::NoSuit, 0);
        drowning->setSkillName("_huwei");
        if (!drowning->isAvailable(player) || player->isProhibited(opponent, drowning)) {
            delete drowning;
            return false;
        }
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(drowning, player, opponent));
        return false;
    }
};

KOFCangjiCard::KOFCangjiCard()
{
    will_throw = false;
}

bool KOFCangjiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    QList<int> equip_loc;
    foreach (int id, subcards) {
        const Card *card = Sanguosha->getCard(id);
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        if (equip)
            equip_loc << equip->location();
    }
    foreach (int loc, equip_loc) {
        if (to_select->getEquip(loc))
            return false;
    }
    return true;
}

void KOFCangjiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    CardsMoveStruct move(subcards, effect.from, effect.to, Player::PlaceUnknown, Player::PlaceEquip, CardMoveReason());
    room->moveCardsAtomic(move, true);

    if (effect.from->getEquips().isEmpty())
        return;
    bool loop = false;
    for (int i = 0; i <= 3; i++) {
        if (effect.from->getEquip(i)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(effect.from)) {
                if (!p->getEquip(i)) {
                    loop = true;
                    break;
                }
            }
            if (loop) break;
        }
    }
    if (loop)
        room->askForUseCard(effect.from, "@@kofcangji", "@kofcangji-install", -1, Card::MethodNone);
}

class KOFCangjiViewAsSkill : public ViewAsSkill
{
public:
    KOFCangjiViewAsSkill() : ViewAsSkill("kofcangji")
    {
        response_pattern = "@@kofcangji";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        KOFCangjiCard *card = new KOFCangjiCard;
        card->addSubcards(cards);
        return card;
    }
};

class KOFCangji : public TriggerSkill
{
public:
    KOFCangji() : TriggerSkill("kofcangji")
    {
        events << Death;
        view_as_skill = new KOFCangjiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player || !player->hasSkill(objectName()) || player->getEquips().isEmpty())
            return false;
        if (room->getMode() == "02_1v1") {
            if (room->askForSkillInvoke(player, objectName())) {
                QVariantList equip_list;
                CardsMoveStruct move;
                move.from = player;
                move.to = NULL;
                move.to_place = Player::PlaceTable;

                foreach (const Card *equip, player->getEquips()) {
                    equip_list.append(QVariant(equip->getEffectiveId()));
                    move.card_ids.append(equip->getEffectiveId());
                }
                player->tag[objectName()] = QVariant::fromValue(equip_list);
                room->moveCardsAtomic(move, true);
            }
        } else {
            room->askForUseCard(player, "@@kofcangji", "@kofcangji-install", -1, Card::MethodNone);
        }
        return false;
    }
};

class KOFCangjiInstall : public TriggerSkill
{
public:
    KOFCangjiInstall() : TriggerSkill("#kofcangji-install")
    {
        events << Debut;
    }

    int getPriority(TriggerEvent) const
    {
        return 5;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return !target->tag["kofcangji"].isNull();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<int> equip_list;
        foreach (QVariant id, player->tag["kofcangji"].toList()) {
            int card_id = id.toInt();
            if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeEquip)
                equip_list << card_id;
        }
        player->tag.remove("kofcangji");
        if (equip_list.isEmpty())
            return false;

        LogMessage log;
        log.from = player;
        log.type = "$Install";
        log.card_str = IntList2StringList(equip_list).join("+");
        room->sendLog(log);

        CardsMoveStruct move(equip_list, player, Player::PlaceEquip, CardMoveReason());
        room->moveCardsAtomic(move, true);
        return false;
    }
};

class KOFLiegong : public TriggerSkill
{
public:
    KOFLiegong() : TriggerSkill("kofliegong")
    {
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            int handcardnum = p->getHandcardNum();
            if (player->getHp() <= handcardnum && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke("liegong");

                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class Manyi : public TriggerSkill
{
public:
    Manyi() : TriggerSkill("manyi")
    {
        events << Debut;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        SavageAssault *savage_assault = new SavageAssault(Card::NoSuit, 0);
        savage_assault->setSkillName("_manyi");
        if (!savage_assault->isAvailable(player)) {
            delete savage_assault;
            return false;
        }
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(savage_assault, player, QList<ServerPlayer *>()));
        return false;
    }
};

class ManyiAvoid : public TriggerSkill
{
public:
    ManyiAvoid() : TriggerSkill("#manyi-avoid")
    {
        events << CardEffected;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("SavageAssault")) {
            room->broadcastSkillInvoke(player->isFemale() ? "juxiang" : "huoshou");

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = "manyi";
            log.arg2 = "savage_assault";
            room->sendLog(log);

            return true;
        } else
            return false;
    }
};

class KOFXiaoji : public TriggerSkill
{
public:
    KOFXiaoji() : TriggerSkill("kofxiaoji")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == sunshangxiang && move.from_places.contains(Player::PlaceEquip)) {
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (!sunshangxiang->isAlive())
                    return false;
                if (move.from_places[i] == Player::PlaceEquip) {
                    QStringList choicelist;
                    choicelist << "draw" << "cancel";
                    if (sunshangxiang->isWounded())
                        choicelist.prepend("recover");
                    QString choice = room->askForChoice(sunshangxiang, objectName(), choicelist.join("+"));
                    if (choice == "cancel")
                        return false;
                    room->broadcastSkillInvoke("xiaoji");
                    room->notifySkillInvoked(sunshangxiang, objectName());

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = sunshangxiang;
                    log.arg = objectName();
                    room->sendLog(log);
                    if (choice == "draw")
                        sunshangxiang->drawCards(2, objectName());
                    else
                        room->recover(sunshangxiang, RecoverStruct(sunshangxiang));
                }
            }
        }
        return false;
    }
};

class Yinli : public TriggerSkill
{
public:
    Yinli() : TriggerSkill("yinli")
    {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == sunshangxiang || move.from == NULL)
            return false;
        if (move.from->getPhase() != Player::NotActive && move.to_place == Player::DiscardPile) {
            QList<int> card_ids;
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeEquip
                    && room->getCardOwner(card_id) == move.from
                    && (room->getCardPlace(card_id) == Player::PlaceHand || room->getCardPlace(card_id) == Player::PlaceEquip))
                    card_ids << card_id;
                i++;
            }
            if (card_ids.isEmpty())
                return false;
            else if (sunshangxiang->askForSkillInvoke(objectName(), data)) {
                int ai_delay = Config.AIDelay;
                Config.AIDelay = 0;
                while (!card_ids.isEmpty()) {
                    room->fillAG(card_ids, sunshangxiang);
                    int id = room->askForAG(sunshangxiang, card_ids, true, objectName());
                    if (id == -1) {
                        room->clearAG(sunshangxiang);
                        break;
                    }
                    card_ids.removeOne(id);
                    room->clearAG(sunshangxiang);
                }
                Config.AIDelay = ai_delay;

                if (!card_ids.isEmpty()) {
                    room->broadcastSkillInvoke("yinli");
                    move.removeCardIds(card_ids);
                    data = QVariant::fromValue(move);
                    DummyCard *dummy = new DummyCard(card_ids);
                    room->moveCardTo(dummy, sunshangxiang, Player::PlaceHand, move.reason, true);
                    delete dummy;
                }
            }
        }
        return false;
    }
};

class Pianyi : public TriggerSkill
{
public:
    Pianyi() : TriggerSkill("pianyi")
    {
        events << Debut;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        ServerPlayer *opponent = room->getOtherPlayers(player).first();
        if (opponent->getPhase() != Player::NotActive) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());

            LogMessage log2;
            log2.type = "#TurnBroken";
            log2.from = opponent;
            room->sendLog(log2);

            throw TurnBroken;
        }
        return false;
    }
};

MouzhuCard::MouzhuCard()
{
}

bool MouzhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void MouzhuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ServerPlayer *hejin = effect.from, *target = effect.to;
    if (target->isKongcheng()) return;

    const Card *card = NULL;
    if (target->getHandcardNum() > 1) {
        card = room->askForCard(target, ".!", "@mouzhu-give:" + hejin->objectName(), QVariant(), Card::MethodNone);
        if (!card)
            card = target->getHandcards().at(qrand() % target->getHandcardNum());
    } else {
        card = target->getHandcards().first();
    }
    Q_ASSERT(card != NULL);
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), hejin->objectName(), "mouzhu", QString());
    room->obtainCard(hejin, card, reason, false);
    if (!hejin->isAlive() || !target->isAlive()) return;
    if (hejin->getHandcardNum() > target->getHandcardNum()) {
        QStringList choicelist;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_mouzhu");
        Duel *duel = new Duel(Card::NoSuit, 0);
        duel->setSkillName("_mouzhu");
        if (!target->isLocked(slash) && target->canSlash(hejin, slash, false))
            choicelist.append("slash");
        if (!target->isLocked(duel) && !target->isProhibited(hejin, duel))
            choicelist.append("duel");
        if (choicelist.isEmpty()) {
            delete slash;
            delete duel;
            return;
        }
        QString choice = room->askForChoice(target, "mouzhu", choicelist.join("+"));
        CardUseStruct use;
        use.from = target;
        use.to << hejin;
        if (choice == "slash") {
            delete duel;
            use.card = slash;
        } else {
            delete slash;
            use.card = duel;
        }
        room->useCard(use);
    }
}

class Mouzhu : public ZeroCardViewAsSkill
{
public:
    Mouzhu() : ZeroCardViewAsSkill("mouzhu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MouzhuCard");
    }

    const Card *viewAs() const
    {
        return new MouzhuCard;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("MouzhuCard"))
            return -1;
        else
            return -2;
    }
};

class Yanhuo : public TriggerSkill
{
public:
    Yanhuo() : TriggerSkill("yanhuo")
    {
        events << BeforeGameOverJudge << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target && !target->isAlive() && target->hasSkill(objectName());
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == BeforeGameOverJudge) {
            player->setMark(objectName(), player->getCardCount());
        } else {
            int n = player->getMark(objectName());
            if (n == 0) return false;
            bool normal = false;
            ServerPlayer *killer = NULL;
            if (room->getMode() == "02_1v1")
                killer = room->getOtherPlayers(player).first();
            else {
                normal = true;
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (player->canDiscard(p, "he"))
                        targets << p;
                }
                if (!targets.isEmpty())
                    killer = room->askForPlayerChosen(player, targets, objectName(), "yanhuo-invoke", true, true);
            }
            if (killer && killer->isAlive() && player->canDiscard(killer, "he")
                && (normal || room->askForSkillInvoke(player, objectName()))) {
                for (int i = 0; i < n; i++) {
                    if (player->canDiscard(killer, "he")) {
                        int card_id = room->askForCardChosen(player, killer, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(Sanguosha->getCard(card_id), killer, player);
                    } else {
                        break;
                    }
                }
            }
        }
        return false;
    }
};

class Renwang : public TriggerSkill
{
public:
    Renwang() : TriggerSkill("renwang")
    {
        events << CardUsed << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash") && !use.card->isNDTrick()) return false;
            QList<ServerPlayer *> first;
            foreach (ServerPlayer *to, use.to) {
                if (to != player && !to->hasFlag("RenwangEffect")) {
                    first << to;
                    to->setFlags("RenwangEffect");
                }
            }
            foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                if (use.to.contains(p) && !first.contains(p) && p->canDiscard(use.from, "he")
                    && p->hasFlag("RenwangEffect") && TriggerSkill::triggerable(p)
                    && room->askForSkillInvoke(p, objectName(), data)) {
                    room->throwCard(room->askForCardChosen(p, use.from, "he", objectName(), false, Card::MethodDiscard), use.from, p);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *to, room->getAlivePlayers()) {
                    if (to->hasFlag("RenwangEffect"))
                        to->setFlags("-RenwangEffect");
                }
            }
        }
        return false;
    }
};

class KOFKuanggu : public TriggerSkill
{
public:
    KOFKuanggu() : TriggerSkill("kofkuanggu")
    {
        events << Damage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->askForSkillInvoke(player, objectName(), data)) {
            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.who = player;
            judge.reason = objectName();

            room->judge(judge);
            if (judge.isGood() && player->isWounded()) {
                room->broadcastSkillInvoke("kuanggu");
                room->recover(player, RecoverStruct(player));
            }
        }

        return false;
    }
};

class Shenju : public MaxCardsSkill
{
public:
    Shenju() : MaxCardsSkill("shenju")
    {
    }

    int getExtra(const Player *target) const
    {
        if (target->hasSkill(objectName())) {
            int max = 0;
            foreach(const Player *p, target->getAliveSiblings())
                if (p->getHp() > max) max = p->getHp();
            return max;
        } else
            return 0;
    }
};

class BotuCount : public TriggerSkill
{
public:
    BotuCount() : TriggerSkill("#botu-count")
    {
        events << PreCardUsed << CardResponded << TurnStart;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Play && (triggerEvent == PreCardUsed || triggerEvent == CardResponded)) {
            const Card *c = NULL;
            if (triggerEvent == PreCardUsed)
                c = data.value<CardUseStruct>().card;
            else
                c = data.value<CardResponseStruct>().m_card;
            if (c && int(c->getSuit()) <= 3) {
                player->setMark("botu", player->getMark("botu") | (1 << int(c->getSuit())));
            }
        } else if (triggerEvent == TurnStart) {
            player->setMark("botu", 0);
        }

        return false;
    }
};

class Botu : public PhaseChangeSkill
{
public:
    Botu() : PhaseChangeSkill("botu")
    {
        frequency = Frequent;
    }

    int getPriority(TriggerEvent) const
    {
        return 1;
    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::NotActive) {
            if (player->getMark("botu") != 0xF || !player->askForSkillInvoke("botu"))
                return false;
            room->broadcastSkillInvoke(objectName());
            player->gainAnExtraTurn();
        }
        return false;
    }
};

class Wanrong : public TriggerSkill
{
public:
    Wanrong() : TriggerSkill("wanrong")
    {
        events << TargetConfirmed;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.to.contains(player)
            && room->askForSkillInvoke(player, objectName(), data))
            player->drawCards(1, objectName());
        return false;
    }
};

class Cuorui : public TriggerSkill
{
public:
    Cuorui() : TriggerSkill("cuorui")
    {
        events << DrawInitialCards << EventPhaseChanging;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DrawInitialCards) {
            int n = 3;
            if (room->getMode() == "02_1v1") {
                n = player->tag["1v1Arrange"].toStringList().length();
                if (Config.value("1v1/Rule", "2013").toString() != "2013")
                    n += 3;
                int origin = (Config.value("1v1/Rule", "2013").toString() == "Classical") ? 4 : player->getMaxHp();
                n += (2 - origin);
            }

            room->broadcastSkillInvoke("cuorui");
            room->sendCompulsoryTriggerLog(player, "cuorui");

            data = data.toInt() + n;
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Judge && player->getMark("CuoruiSkipJudge") == 0) {
                room->broadcastSkillInvoke("cuorui");
                room->sendCompulsoryTriggerLog(player, "cuorui");

                player->skip(Player::Judge);
                player->addMark("CuoruiSkipJudge");
            }
        }
        return false;
    }
};

class Liewei : public TriggerSkill
{
public:
    Liewei() : TriggerSkill("liewei")
    {
        events << Death;
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (TriggerSkill::triggerable(player)
            && room->askForSkillInvoke(player, objectName(), data))
            player->drawCards(3, objectName());
        return false;
    }
};

class NiluanViewAsSkill : public OneCardViewAsSkill
{
public:
    NiluanViewAsSkill() : OneCardViewAsSkill("niluan")
    {
        filter_pattern = ".|black";
        response_pattern = "@@niluan";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(originalCard);
        slash->setSkillName("niluan");
        return slash;
    }
};

class Niluan : public TriggerSkill
{
public:
    Niluan() : TriggerSkill("niluan")
    {
        events << EventPhaseStart;
        view_as_skill = new NiluanViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Finish) {
            ServerPlayer *hansui = room->findPlayerBySkillName(objectName());
            if (hansui && hansui != player && hansui->canSlash(player, false)
                && (player->getHp() > hansui->getHp() || hansui->hasFlag("NiluanSlashTarget"))) {
                if (hansui->isKongcheng()) {
                    bool has_black = false;
                    for (int i = 0; i < 4; i++) {
                        const EquipCard *equip = hansui->getEquip(i);
                        if (equip && equip->isBlack()) {
                            has_black = true;
                            break;
                        }
                    }
                    if (!has_black) return false;
                }

                room->setPlayerFlag(hansui, "slashTargetFix");
                room->setPlayerFlag(hansui, "slashNoDistanceLimit");
                room->setPlayerFlag(hansui, "slashTargetFixToOne");
                room->setPlayerFlag(player, "SlashAssignee");

                const Card *slash = room->askForUseCard(hansui, "@@niluan", "@niluan-slash:" + player->objectName());
                if (slash == NULL) {
                    room->setPlayerFlag(hansui, "-slashTargetFix");
                    room->setPlayerFlag(hansui, "-slashNoDistanceLimit");
                    room->setPlayerFlag(hansui, "-slashTargetFixToOne");
                    room->setPlayerFlag(player, "-SlashAssignee");
                }
            }
        }
        return false;
    }
};

class NiluanRecord : public TriggerSkill
{
public:
    NiluanRecord() : TriggerSkill("#niluan-record")
    {
        events << TargetSpecified << EventPhaseStart;
        global = true;
    }

    int getPriority(TriggerEvent) const
    {
        return 4;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *to, use.to) {
                    if (!to->hasFlag("NiluanSlashTarget"))
                        to->setFlags("NiluanSlashTarget");
                }
            }
        } else if (player->getPhase() == Player::RoundStart) {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                p->setFlags("-NiluanSlashTarget");
        }
        return false;
    }
};

class KOFGanglie : public MasochismSkill
{
public:
    KOFGanglie() : MasochismSkill("kofganglie")
    {
    }

    void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(xiahou, "kofganglie", data)) {
            room->broadcastSkillInvoke("ganglie");

            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead()) return;
            if (judge.isGood()) {
                if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                    room->damage(DamageStruct(objectName(), xiahou, from));
            }
        }
    }
};

class KOFLuoyiBuff : public TriggerSkill
{
public:
    KOFLuoyiBuff() : TriggerSkill("#kofluoyi")
    {
        events << DamageCaused;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasFlag("kofluoyi") && target->isAlive();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
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

class KOFLuoyi : public DrawCardsSkill
{
public:
    KOFLuoyi() : DrawCardsSkill("kofluoyi")
    {
    }

    int getDrawNum(ServerPlayer *xuchu, int n) const
    {
        Room *room = xuchu->getRoom();
        if (room->askForSkillInvoke(xuchu, objectName())) {
            room->broadcastSkillInvoke("luoyi");
            xuchu->setFlags(objectName());
            return n - 1;
        } else
            return n;
    }
};

class Longdan : public OneCardViewAsSkill
{
public:
    Longdan() : OneCardViewAsSkill("longdan")
    {
        response_or_use = true;
    }

    bool viewFilter(const Card *to_select) const
    {
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            return card->isKindOf("Jink");
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "slash")
                return card->isKindOf("Jink");
            else if (pattern == "jink")
                return card->isKindOf("Slash");
        }
        default:
            return false;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "jink" || pattern == "slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }
};

class KOFQianxun : public ProhibitSkill
{
public:
    KOFQianxun() : ProhibitSkill("kofqianxun")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && (card->isKindOf("Snatch") || card->isKindOf("Indulgence"));
    }
};

class KOFLianying : public TriggerSkill
{
public:
    KOFLianying() : TriggerSkill("koflianying")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            if (room->askForSkillInvoke(luxun, objectName(), data)) {
                room->broadcastSkillInvoke("lianying");
                luxun->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class KOFJushou : public Jushou
{
public:
    KOFJushou() : Jushou()
    {
        setObjectName("kofjushou");
    }

    int getJushouDrawNum(ServerPlayer *) const
    {
        return 3;
    }
};

class KOFBuquRemove : public TriggerSkill
{
public:
    KOFBuquRemove() : TriggerSkill("#kofbuqu-remove")
    {
        events << HpRecover;
    }

    static void Remove(ServerPlayer *zhoutai)
    {
        Room *room = zhoutai->getRoom();
        QList<int> kofbuqu(zhoutai->getPile("kofbuqu"));

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "kofbuqu", QString());
        int need = 1 - zhoutai->getHp();
        if (need <= 0) {
            // clear all the buqu cards
            foreach (int card_id, kofbuqu) {
                LogMessage log;
                log.type = "$KOFBuquRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            }
        } else {
            int to_remove = kofbuqu.length() - need;
            for (int i = 0; i < to_remove; i++) {
                room->fillAG(kofbuqu);
                int card_id = room->askForAG(zhoutai, kofbuqu, false, "kofbuqu");

                LogMessage log;
                log.type = "$KOFBuquRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                kofbuqu.removeOne(card_id);
                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
                room->clearAG();
            }
        }
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *zhoutai, QVariant &) const
    {
        if (zhoutai->getPile("kofbuqu").length() > 0)
            Remove(zhoutai);

        return false;
    }
};

class KOFBuqu : public TriggerSkill
{
public:
    KOFBuqu() : TriggerSkill("kofbuqu")
    {
        events << HpChanged << AskForPeachesDone;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == HpChanged)
            return 1;
        else
            return TriggerSkill::getPriority(triggerEvent);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data) const
    {
        if (triggerEvent == HpChanged && !data.isNull() && !data.canConvert<RecoverStruct>() && zhoutai->getHp() < 1) {
            if (room->askForSkillInvoke(zhoutai, objectName(), data)) {
                room->setTag("KOFBuqu", zhoutai->objectName());
                room->broadcastSkillInvoke("buqu");
                const QList<int> &kofbuqu = zhoutai->getPile("kofbuqu");

                int need = 1 - zhoutai->getHp(); // the buqu cards that should be turned over
                int n = need - kofbuqu.length();
                if (n > 0) {
                    QList<int> card_ids = room->getNCards(n, false);
                    zhoutai->addToPile("kofbuqu", card_ids);
                }
                const QList<int> &kofbuqunew = zhoutai->getPile("kofbuqu");
                QList<int> duplicate_numbers;

                QSet<int> numbers;
                foreach (int card_id, kofbuqunew) {
                    const Card *card = Sanguosha->getCard(card_id);
                    int number = card->getNumber();

                    if (numbers.contains(number))
                        duplicate_numbers << number;
                    else
                        numbers << number;
                }

                if (duplicate_numbers.isEmpty()) {
                    room->setTag("KOFBuqu", QVariant());
                    return true;
                }
            }
        } else if (triggerEvent == AskForPeachesDone) {
            const QList<int> &kofbuqu = zhoutai->getPile("kofbuqu");

            if (zhoutai->getHp() > 0)
                return false;
            if (room->getTag("KOFBuqu").toString() != zhoutai->objectName())
                return false;
            room->setTag("KOFBuqu", QVariant());

            QList<int> duplicate_numbers;
            QSet<int> numbers;
            foreach (int card_id, kofbuqu) {
                const Card *card = Sanguosha->getCard(card_id);
                int number = card->getNumber();

                if (numbers.contains(number) && !duplicate_numbers.contains(number))
                    duplicate_numbers << number;
                else
                    numbers << number;
            }

            if (duplicate_numbers.isEmpty()) {
                room->broadcastSkillInvoke("buqu");
                room->setPlayerFlag(zhoutai, "-Global_Dying");
                return true;
            } else {
                LogMessage log;
                log.type = "#KOFBuquDuplicate";
                log.from = zhoutai;
                log.arg = QString::number(duplicate_numbers.length());
                room->sendLog(log);

                for (int i = 0; i < duplicate_numbers.length(); i++) {
                    int number = duplicate_numbers.at(i);

                    LogMessage log;
                    log.type = "#KOFBuquDuplicateGroup";
                    log.from = zhoutai;
                    log.arg = QString::number(i + 1);
                    if (number == 10)
                        log.arg2 = "10";
                    else {
                        const char *number_string = "-A23456789-JQK";
                        log.arg2 = QString(number_string[number]);
                    }
                    room->sendLog(log);

                    foreach (int card_id, kofbuqu) {
                        const Card *card = Sanguosha->getCard(card_id);
                        if (card->getNumber() == number) {
                            LogMessage log;
                            log.type = "$KOFBuquDuplicateItem";
                            log.from = zhoutai;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }
                    }
                }
            }
        }
        return false;
    }
};

class KOFBuquClear : public DetachEffectSkill
{
public:
    KOFBuquClear() : DetachEffectSkill("kofbuqu")
    {
    }

    void onSkillDetached(Room *room, ServerPlayer *player) const
    {
        if (player->getHp() <= 0)
            room->enterDying(player, NULL);
    }
};

KOFQiangxiCard::KOFQiangxiCard()
{
}

bool KOFQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return true;
}

void KOFQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    if (subcards.isEmpty())
        room->loseHp(effect.from);

    room->damage(DamageStruct("kofqiangxi", effect.from, effect.to));
}

class KOFQiangxi : public ViewAsSkill
{
public:
    KOFQiangxi() : ViewAsSkill("kofqiangxi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KOFQiangxiCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new KOFQiangxiCard;
        else if (cards.length() == 1) {
            KOFQiangxiCard *card = new KOFQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

KOFDrowning::KOFDrowning(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("kofdrowning");
}

bool KOFDrowning::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self;
}

void KOFDrowning::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (!effect.to->getEquips().isEmpty()
        && room->askForChoice(effect.to, objectName(), "throw+damage", QVariant::fromValue(effect)) == "throw")
        effect.to->throwAllEquips();
    else
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
}

class KOFZaiqi : public PhaseChangeSkill
{
public:
    KOFZaiqi() : PhaseChangeSkill("kofzaiqi")
    {
    }

    bool onPhaseChange(ServerPlayer *menghuo) const
    {
        if (menghuo->getPhase() == Player::Draw && menghuo->isWounded()) {
            Room *room = menghuo->getRoom();
            if (room->askForSkillInvoke(menghuo, objectName())) {
                room->broadcastSkillInvoke(objectName(), 1);

                bool has_heart = false;
                int x = menghuo->getLostHp();
                QList<int> ids = room->getNCards(x, false);
                CardsMoveStruct move(ids, menghuo, Player::PlaceTable,
                    CardMoveReason(CardMoveReason::S_REASON_TURNOVER, menghuo->objectName(), "kofzaiqi", QString()));
                room->moveCardsAtomic(move, true);

                room->getThread()->delay();
                room->getThread()->delay();

                QList<int> card_to_throw;
                QList<int> card_to_gotback;
                for (int i = 0; i < x; i++) {
                    if (Sanguosha->getCard(ids[i])->getSuit() == Card::Heart)
                        card_to_throw << ids[i];
                    else
                        card_to_gotback << ids[i];
                }
                if (!card_to_throw.isEmpty()) {
                    room->recover(menghuo, RecoverStruct(menghuo, NULL, card_to_throw.length()));

                    DummyCard *dummy = new DummyCard(card_to_throw);
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, menghuo->objectName(), "zaiqi", QString());
                    room->throwCard(dummy, reason, NULL);
                    delete dummy;
                    has_heart = true;
                }
                if (!card_to_gotback.isEmpty()) {
                    DummyCard *dummy2 = new DummyCard(card_to_gotback);
                    room->obtainCard(menghuo, dummy2);
                    delete dummy2;
                }

                if (has_heart)
                    room->broadcastSkillInvoke(objectName(), 2);
                else
                    room->broadcastSkillInvoke(objectName(), 3);

                return true;
            }
        }

        return false;
    }
};


Special1v1Package::Special1v1Package()
    : Package("Special1v1")
{
    General *kof_xiahoudun = new General(this, "kof_xiahoudun", "wei", 4, true, true, true);
    kof_xiahoudun->addSkill(new KOFGanglie);
    
    General *kof_zhangliao = new General(this, "kof_zhangliao", "wei", 4, true, true, true);
    kof_zhangliao->addSkill(new KOFTuxi);
    kof_zhangliao->addSkill(new KOFTuxiAct);
    related_skills.insertMulti("koftuxi", "#koftuxi");

    General *kof_xuchu = new General(this, "kof_xuchu", "wei", 4, true, true, true);
    kof_xuchu->addSkill("kofluoyi");
    kof_xuchu->addSkill(new Xiechan);

    General *kof_zhenji = new General(this, "kof_zhenji", "wei", 3, false, true, true);
    kof_zhenji->addSkill(new KOFQingguo);
    kof_zhenji->addSkill("luoshen");
    
    General *kof_caoren = new General(this, "kof_caoren", "wei", 4, true, true, true);
    kof_caoren->addSkill(new KOFJushou);

    General *kof_xiahouyuan = new General(this, "kof_xiahouyuan", "wei", 4, true, true, true);
    kof_xiahouyuan->addSkill("shensu");
    kof_xiahouyuan->addSkill(new KOFSuzi);
    
    General *kof_dianwei = new General(this, "kof_dianwei", "wei", 4, true, true, true); // WEI 012
    kof_dianwei->addSkill(new KOFQiangxi);

    General *kof_liubei = new General(this, "kof_liubei$", "shu", 4, true, true, true);
    kof_liubei->addSkill(new Renwang);

    General *kof_guanyu = new General(this, "kof_guanyu", "shu", 4, true, true, true);
    kof_guanyu->addSkill("wusheng");
    kof_guanyu->addSkill(new Huwei);
    
    General *kof_zhaoyun = new General(this, "kof_zhaoyun", "shu", 4, true, true, true);
    kof_zhaoyun->addSkill(new Longdan);

    General *kof_huangyueying = new General(this, "kof_huangyueying", "shu", 3, false, true, true);
    kof_huangyueying->addSkill("jizhi");
    kof_huangyueying->addSkill(new KOFCangji);
    kof_huangyueying->addSkill(new KOFCangjiInstall);
    related_skills.insertMulti("kofcangji", "#kofcangji-install");

    General *kof_huangzhong = new General(this, "kof_huangzhong", "shu", 4, true, true, true);
    kof_huangzhong->addSkill(new KOFLiegong);

    General *kof_weiyan = new General(this, "kof_weiyan", "shu", 4, true, true, true);
    kof_weiyan->addSkill(new KOFKuanggu);

    General *kof_jiangwei = new General(this, "kof_jiangwei", "shu", 4, true, true, true);
    kof_jiangwei->addSkill("tiaoxin");

    General *kof_menghuo = new General(this, "kof_menghuo", "shu", 4, true, true, true);
    kof_menghuo->addSkill(new Manyi);
    kof_menghuo->addSkill(new ManyiAvoid);
    kof_menghuo->addSkill(new KOFZaiqi);
    related_skills.insertMulti("manyi", "#manyi-avoid");

    General *kof_zhurong = new General(this, "kof_zhurong", "shu", 4, false, true, true);
    kof_zhurong->addSkill("manyi");
    kof_zhurong->addSkill("lieren");

    General *kof_lvmeng = new General(this, "kof_lvmeng", "wu", 4, true, true, true);
    kof_lvmeng->addSkill(new Shenju);
    kof_lvmeng->addSkill(new Botu);
    kof_lvmeng->addSkill(new BotuCount);
    related_skills.insertMulti("botu", "#botu-count");

    General *kof_daqiao = new General(this, "kof_daqiao", "wu", 3, false, true, true);
    kof_daqiao->addSkill("nosguose");
    kof_daqiao->addSkill(new Wanrong);
    
    General *kof_luxun = new General(this, "kof_luxun", "wu", 3, true, true, true);
    kof_luxun->addSkill(new KOFQianxun);
    kof_luxun->addSkill(new KOFLianying);

    General *kof_sunshangxiang = new General(this, "kof_sunshangxiang", "wu", 3, false, true, true);
    kof_sunshangxiang->addSkill(new Yinli);
    kof_sunshangxiang->addSkill(new KOFXiaoji);
    
    General *kof_zhoutai = new General(this, "kof_zhoutai", "wu", 4, true, true, true);
    kof_zhoutai->addSkill(new KOFBuqu);
    kof_zhoutai->addSkill(new KOFBuquRemove);
    kof_zhoutai->addSkill(new KOFBuquClear);
    related_skills.insertMulti("kofbuqu", "#kofbuqu-remove");
    related_skills.insertMulti("kofbuqu", "#kofbuqu-clear");
    
    General *kof_lvbu = new General(this, "kof_lvbu", "qun", 4, true, true, true);
    kof_lvbu->addSkill("wushuang");

    General *kof_diaochan = new General(this, "kof_diaochan", "qun", 3, false, true, true);
    kof_diaochan->addSkill(new Pianyi);
    kof_diaochan->addSkill("biyue");

    addMetaObject<XiechanCard>();
    addMetaObject<KOFCangjiCard>();
    addMetaObject<KOFQiangxiCard>();
}

ADD_PACKAGE(Special1v1)

Special1v1ExtPackage::Special1v1ExtPackage()
: Package("Special1v1Ext")
{
    General *hejin = new General(this, "hejin", "qun", 4, true, true, true); // QUN 025
    hejin->addSkill(new Mouzhu);
    hejin->addSkill(new Yanhuo);

    General *niujin = new General(this, "niujin", "wei", 4, true, true, true); // WEI 025
    niujin->addSkill(new Cuorui);
    niujin->addSkill(new Liewei);

    General *hansui = new General(this, "hansui", "qun", 4, true, true, true); // QUN 027
    hansui->addSkill("mashu");
    hansui->addSkill(new Niluan);
    hansui->addSkill(new NiluanRecord);
    related_skills.insertMulti("niluan", "#niluan-record");

    addMetaObject<MouzhuCard>();
}

ADD_PACKAGE(Special1v1Ext)

New1v1CardPackage::New1v1CardPackage()
: Package("New1v1Card")
{
    QList<Card *> cards;
    cards << new Duel(Card::Spade, 1)
        << new EightDiagram(Card::Spade, 2)
        << new Dismantlement(Card::Spade, 3)
        << new Snatch(Card::Spade, 4)
        << new Slash(Card::Spade, 5)
        << new QinggangSword(Card::Spade, 6)
        << new Slash(Card::Spade, 7)
        << new Slash(Card::Spade, 8)
        << new IceSword(Card::Spade, 9)
        << new Slash(Card::Spade, 10)
        << new Snatch(Card::Spade, 11)
        << new Spear(Card::Spade, 12)
        << new SavageAssault(Card::Spade, 13);

    cards << new ArcheryAttack(Card::Heart, 1)
        << new Jink(Card::Heart, 2)
        << new Peach(Card::Heart, 3)
        << new Peach(Card::Heart, 4)
        << new Jink(Card::Heart, 5)
        << new Indulgence(Card::Heart, 6)
        << new ExNihilo(Card::Heart, 7)
        << new ExNihilo(Card::Heart, 8)
        << new Peach(Card::Heart, 9)
        << new Slash(Card::Heart, 10)
        << new Slash(Card::Heart, 11)
        << new Dismantlement(Card::Heart, 12)
        << new Nullification(Card::Heart, 13);

    cards << new Duel(Card::Club, 1)
        << new RenwangShield(Card::Club, 2)
        << new Dismantlement(Card::Club, 3)
        << new Slash(Card::Club, 4)
        << new Slash(Card::Club, 5)
        << new Slash(Card::Club, 6)
        << new KOFDrowning(Card::Club, 7)
        << new Slash(Card::Club, 8)
        << new Slash(Card::Club, 9)
        << new Slash(Card::Club, 10)
        << new Slash(Card::Club, 11)
        << new SupplyShortage(Card::Club, 12)
        << new Nullification(Card::Club, 13);

    cards << new Crossbow(Card::Diamond, 1)
        << new Jink(Card::Diamond, 2)
        << new Jink(Card::Diamond, 3)
        << new Snatch(Card::Diamond, 4)
        << new Axe(Card::Diamond, 5)
        << new Slash(Card::Diamond, 6)
        << new Jink(Card::Diamond, 7)
        << new Jink(Card::Diamond, 8)
        << new Slash(Card::Diamond, 9)
        << new Jink(Card::Diamond, 10)
        << new Jink(Card::Diamond, 11)
        << new Peach(Card::Diamond, 12)
        << new Slash(Card::Diamond, 13);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New1v1Card)
