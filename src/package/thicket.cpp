#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "json.h"
#include "roomthread.h"

class Xingshang : public TriggerSkill
{
public:
    Xingshang() : TriggerSkill("xingshang")
    {
        events << Death;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.who;
        if (player->isNude() || caopi == player)
            return false;
        if (caopi->isAlive() && room->askForSkillInvoke(caopi, objectName(), data)) {
            bool isCaoCao = player->getGeneralName().contains("caocao");
            room->broadcastSkillInvoke(objectName(), (isCaoCao ? 3 : (player->isMale() ? 1 : 2)));

            DummyCard *dummy = new DummyCard(player->handCards());
            QList <const Card *> equips = player->getEquips();
            foreach(const Card *card, equips)
                dummy->addSubcard(card);

            if (dummy->subcardsLength() > 0) {
                CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, caopi->objectName());
                room->obtainCard(caopi, dummy, reason, false);
            }
            delete dummy;
        }

        return false;
    }
};

class Fangzhu : public MasochismSkill
{
public:
    Fangzhu() : MasochismSkill("fangzhu")
    {
    }

    void onDamaged(ServerPlayer *caopi, const DamageStruct &) const
    {
        Room *room = caopi->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(caopi, room->getOtherPlayers(caopi), objectName(),
            "fangzhu-invoke", caopi->getMark("JilveEvent") != int(Damaged), true);
        if (to) {
            if (caopi->hasInnateSkill("fangzhu") || !caopi->hasSkill("jilve")) {
                int index = to->faceUp() ? 1 : 2;
                if (to->getGeneralName().contains("caozhi") || (to->getGeneral2() && to->getGeneral2Name().contains("caozhi")))
                    index = 3;
                room->broadcastSkillInvoke("fangzhu", index);
            } else
                room->broadcastSkillInvoke("jilve", 2);

            to->drawCards(caopi->getLostHp(), objectName());
            to->turnOver();
        }
    }
};

class Songwei : public TriggerSkill
{
public:
    Songwei() : TriggerSkill("songwei$")
    {
        events << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getKingdom() == "wei";
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        const Card *card = judge->card;

        if (card->isBlack()) {
            QList<ServerPlayer *> caopis;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(this))
                    caopis << p;
            }

            while (!caopis.isEmpty()) {
                ServerPlayer *caopi = room->askForPlayerChosen(player, caopis, objectName(), "@songwei-to", true);
                if (caopi) {                   
                    room->broadcastSkillInvoke(objectName(), player->isMale() ? 1 : 2);
                    room->notifySkillInvoked(caopi, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << caopi;
                    log.arg = objectName();
                    room->sendLog(log);

                    caopi->drawCards(1, objectName());
                    caopis.removeOne(caopi);
                } else
                    break;
            }
        }

        return false;
    }
};

Jushou::Jushou() : PhaseChangeSkill("jushou")
{
}

int Jushou::getJushouDrawNum(ServerPlayer *) const
{
    return 1;
}

bool Jushou::onPhaseChange(ServerPlayer *target) const
{
    if (target->getPhase() == Player::Finish) {
        Room *room = target->getRoom();
        if (room->askForSkillInvoke(target, objectName())) {
            room->broadcastSkillInvoke("jushou");
            target->drawCards(getJushouDrawNum(target), objectName());
            target->turnOver();
        }
    }

    return false;
}


class Jiewei : public TriggerSkill
{
public:
    Jiewei() : TriggerSkill("jiewei")
    {
        events << TurnedOver;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!room->askForSkillInvoke(player, objectName())) return false;
        room->broadcastSkillInvoke(objectName());
        player->drawCards(1, objectName());
        player->gainMark("@solve", 1);    
        return false;
    }
};

class JieweiUse : public TriggerSkill
{
public:
    JieweiUse() : TriggerSkill("#jiewei-use")
    {
        events << EventPhaseChanging;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        ServerPlayer *caoren = room->findPlayerBySkillName("jiewei");
        if (!caoren || !caoren->isAlive())
            return false;
        int n = caoren->getMark("@solve");
        room->removePlayerMark(caoren, "@solve", n);
        for (; n > 0; n--) {
            const Card *card = room->askForUseCard(caoren, "TrickCard+^Nullification,EquipCard|.|.|hand", QString("@jiewei:::%1").arg(n));
            if (!card) continue;
            QList<ServerPlayer *> targets;
            if (card->getTypeId() == Card::TypeTrick) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    bool can_discard = false;
                    foreach (const Card *judge, p->getJudgingArea()) {
                        if (judge->getTypeId() == Card::TypeTrick && caoren->canDiscard(p, judge->getEffectiveId())) {
                            can_discard = true;
                            break;
                        } else if (judge->getTypeId() == Card::TypeSkill) {
                            const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                            if (real_card->getTypeId() == Card::TypeTrick && caoren->canDiscard(p, real_card->getEffectiveId())) {
                                can_discard = true;
                                break;
                            }
                        }
                    }
                    if (can_discard) targets << p;
                }
            } else if (card->getTypeId() == Card::TypeEquip) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!p->getEquips().isEmpty() && caoren->canDiscard(p, "e"))
                        targets << p;
                    else {
                        foreach (const Card *judge, p->getJudgingArea()) {
                            if (judge->getTypeId() == Card::TypeSkill) {
                                const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                                if (real_card->getTypeId() == Card::TypeEquip && caoren->canDiscard(p, real_card->getEffectiveId())) {
                                    targets << p;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (targets.isEmpty()) continue;
            ServerPlayer *to_discard = room->askForPlayerChosen(caoren, targets, objectName(), "@jiewei-discard", true);
            if (to_discard) {
                QList<int> disabled_ids;
                foreach (const Card *c, to_discard->getCards("ej")) {
                    const Card *pcard = c;
                    if (pcard->getTypeId() == Card::TypeSkill)
                        pcard = Sanguosha->getEngineCard(c->getEffectiveId());
                    if (pcard->getTypeId() != card->getTypeId())
                        disabled_ids << pcard->getEffectiveId();
                }
                int id = room->askForCardChosen(caoren, to_discard, "ej", objectName(), false, Card::MethodDiscard, disabled_ids);
                room->throwCard(id, to_discard, caoren);
            }
        }
        return false;
    }
};

class SavageAssaultAvoid : public TriggerSkill
{
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        : TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill)
    {
        events << CardEffected;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("SavageAssault") && room->askForSkillInvoke(player, avoid_skill, "avoid")) {
            room->broadcastSkillInvoke(avoid_skill);

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = avoid_skill;
            log.arg2 = "savage_assault";
            room->sendLog(log);

            return true;
        } else
            return false;
    }

private:
    QString avoid_skill;
};

class Huoshou : public TriggerSkill
{
public:
    Huoshou() : TriggerSkill("huoshou")
    {
        events << SlashMissed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!player->isKongcheng()) {
            SavageAssault *sa = new SavageAssault(Card::NoSuit, 0);
            sa->addSubcards(player->getHandcards());
            if (player->isCardLimited(sa, Card::MethodUse)) {
                delete(sa);
                return false;
            }
            if (room->askForSkillInvoke(player, objectName(), "use")) {
                sa->setSkillName("_huoshou");
                QList<ServerPlayer *> targets, other_players = room->getOtherPlayers(player);
                foreach(ServerPlayer *target, other_players) {
                    const ProhibitSkill *skill = room->isProhibited(player, target, sa);
                    if (skill) {
                        if (skill->isVisible()) {
                            LogMessage log;
                            log.type = "#SkillAvoid";
                            log.from = target;
                            log.arg = skill->objectName();
                            log.arg2 = objectName();
                            room->sendLog(log);

                            room->broadcastSkillInvoke(skill->objectName());
                        }
                    }
                    else
                        targets << target;
                }
                room->useCard(CardUseStruct(sa, player, targets));
            }
            else {
                delete(sa);
            }
        }
        return false;
    }
};

class Lieren : public TriggerSkill
{
public:
    Lieren() : TriggerSkill("lieren")
    {
        events << Damage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && damage.card->isKindOf("Slash") && !zhurong->isKongcheng()
            && !target->isKongcheng() && !target->hasFlag("Global_DebutFlag")
            && room->askForSkillInvoke(zhurong, objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), 1);

            bool success = zhurong->pindian(target, "lieren", NULL);
            if (!success) return false;

            room->broadcastSkillInvoke(objectName(), 2);
            if (!target->isNude()) {
                int card_id = room->askForCardChosen(zhurong, target, "he", objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhurong->objectName());
                room->obtainCard(zhurong, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
            }
        }

        return false;
    }
};

class Zaiqi : public TriggerSkill
{
public:
    Zaiqi() : TriggerSkill("zaiqi")
    {
        events << Damage;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("damage_point_round") >= 3) {
            int n = player->getMark("damage_point_round");
            int i = n - n % 3;
            DamageStruct damage = data.value<DamageStruct>();
            for (; i > n - damage.damage; i = i - 3) {
                player->drawCards(player->getLostHp(), objectName());
                if (player->isWounded())
                    room->recover(player, RecoverStruct(player, NULL, 1));
            }
        }
        return false;
    }
};

class Juxiang : public TriggerSkill
{
public:
    Juxiang() : TriggerSkill("juxiang")
    {
        events << BeforeCardsMove;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.card_ids.length() == 1 && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (!card || !card->isKindOf("SavageAssault"))
                return false;
            if (card->isVirtualCard()) {
                if (card->subcardsLength() == 0)
                    return false;
            }
            if (player != move.from && room->askForSkillInvoke(player, objectName(), "get")) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());

                player->obtainCard(card);
                move.removeCardIds(move.card_ids);
                data = QVariant::fromValue(move);
            }
        }
        return false;
    }
};

HaoshiCard::HaoshiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "_haoshi";
}

bool HaoshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return to_select->getHandcardNum() <= to_select->getHp();
}

void HaoshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
        targets.first()->objectName(), "haoshi", QString());
    room->moveCardTo(this, targets.first(), Player::PlaceHand, reason);
    source->drawCards(getSubcards().length(), "haoshi");
}

class HaoshiViewAsSkill : public ViewAsSkill
{
public:
    HaoshiViewAsSkill() : ViewAsSkill("haoshi")
    {
        response_pattern = "@@haoshi";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < 2;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;

        HaoshiCard *card = new HaoshiCard;
        card->addSubcards(cards);
        return card;
    }
};

class Polu : public TargetModSkill
{
public:
    Polu() : TargetModSkill("polu")
    {
    }

    int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(this) && !from->isWounded())
            return 1;
        else
            return 0;
    }
};


class Haoshi : public TriggerSkill
{
public:
    Haoshi() : TriggerSkill("haoshi")
    {
        events << EventPhaseStart;
        view_as_skill = new HaoshiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Draw ) {
            room->askForUseCard(player, "@@haoshi", "@haoshi", -1, Card::MethodNone);            
        }
        return false;
    }
};

DimengCard::DimengCard()
{
    mute = true;
}

bool DimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.isEmpty())
        return true;

    if (targets.length() == 1) {
        return (to_select->getHandcardNum() != targets.first()->getHandcardNum()) && (to_select->getKingdom() != targets.first()->getKingdom());
    }

    return false;
}

bool DimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void DimengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() > selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    int n = source->getMark("dimeng");
    from->drawCards(n, "dimeng");
    int num = qMin(n, from->getCardCount(true));
    const Card *to_give = room->askForExchange(from, "dimeng", num, num, true, QString("@dimeng-give::%1:%2").arg(to->objectName()).arg(num));
    to->obtainCard(to_give, false);
    delete to_give;   
}

PreDimengCard::PreDimengCard()
{
    mute = true;
    target_fixed = true;
}

void PreDimengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerMark(source, "dimeng", getSubcards().length());
    room->askForUseCard(source, "@@dimeng", "@dimeng-card", -1, Card::MethodNone);
}

class Dimeng : public ViewAsSkill
{
public:
    Dimeng() : ViewAsSkill("dimeng")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@dimeng") 
            return false;
        else
            return !Self->isJilei(to_select) && selected.length() < 2;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@dimeng") {
            return new DimengCard;
        }
        else{
            if (cards.length() == 0)
                return NULL;
            PreDimengCard *card = new PreDimengCard;
            foreach(const Card *c, cards)
                card->addSubcard(c);
            return card;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("PreDimengCard");
    }
    
    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern=="@@dimeng";
    }
};

class Yaowu : public TriggerSkill
{
public:
    Yaowu() : TriggerSkill("yaowu")
    {
        events << Damaged;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
            && damage.from && damage.from->isAlive() && room->askForSkillInvoke(damage.to, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->loseMaxHp(damage.to);
            
            if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover")
                room->recover(damage.from, RecoverStruct(damage.to));
            else
                damage.from->drawCards(1, objectName());
        }
        return false;
    }
};

class Jiuchi : public OneCardViewAsSkill
{
public:
    Jiuchi() : OneCardViewAsSkill("jiuchi")
    {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern.contains("analeptic");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class Roulin : public TriggerSkill
{
public:
    Roulin() : TriggerSkill("roulin")
    {
        events << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            bool play_effect = false;
            if (triggerEvent == TargetSpecified) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->isFemale()) {
                        play_effect = true;
                        if (jink_list.at(index).toInt() == 1)
                            jink_list.replace(index, QVariant(2));
                    }
                    index++;
                }
                use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
                if (play_effect) {
                    room->broadcastSkillInvoke(objectName(), 1);
                    room->sendCompulsoryTriggerLog(use.from, objectName());
                }
            } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
                foreach (ServerPlayer *p, use.to) {
                    if (p == player) {
                        if (jink_list.at(index).toInt() == 1) {
                            play_effect = true;
                            jink_list.replace(index, QVariant(2));
                        }
                    }
                    index++;
                }
                use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);

                if (play_effect) {
                    bool drunk = (use.card->tag.value("drunk", 0).toInt() > 0);
                    int index = drunk ? 3 : 2;
                    room->broadcastSkillInvoke(objectName(), index);
                    room->sendCompulsoryTriggerLog(player, objectName());
                }
            }
        }

        return false;
    }
};

class Benghuai : public PhaseChangeSkill
{
public:
    Benghuai() : PhaseChangeSkill("benghuai")
    {
        frequency = Compulsory;
    }

    bool onPhaseChange(ServerPlayer *dongzhuo) const
    {
        bool trigger_this = false;
        Room *room = dongzhuo->getRoom();

        if (dongzhuo->getPhase() == Player::Finish) {
            QList<ServerPlayer *> players = room->getOtherPlayers(dongzhuo);
            foreach (ServerPlayer *player, players) {
                if (dongzhuo->getHp() > player->getHp()) {
                    trigger_this = true;
                    break;
                }
            }
        }

        if (trigger_this) {
            room->sendCompulsoryTriggerLog(dongzhuo, objectName());

            QString result = room->askForChoice(dongzhuo, "benghuai", "hp+maxhp");
            int index = (dongzhuo->isFemale()) ? 2 : 1;

            if (!dongzhuo->hasInnateSkill(this) && dongzhuo->getMark("juyi") > 0)
                index = 3;

            if (!dongzhuo->hasInnateSkill(this) && dongzhuo->getMark("baoling") > 0)
                index = result == "hp" ? 4 : 5;

            room->broadcastSkillInvoke(objectName(), index);
            if (result == "hp")
                room->loseHp(dongzhuo);
            else
                room->loseMaxHp(dongzhuo);
        }

        return false;
    }
};

class LuanshiAttach : public ViewAsSkill
{
public:
    LuanshiAttach() : ViewAsSkill("luanshi_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return shouldBeVisible(player) && !player->hasUsed("LuanshiCard") && !(player->isNude() && player->isChained());
    }

    bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "qun";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.isEmpty();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty() && Self->isChained()) return NULL;
        LuanshiCard *luanshi_card = new LuanshiCard;
        luanshi_card->addSubcards(cards);
        luanshi_card->setSkillName("luanshi");
        return luanshi_card;
    }
};

class LuanshiViewAsSkill : public ViewAsSkill
{
public:
    LuanshiViewAsSkill() : ViewAsSkill("luanshi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return shouldBeVisible(player) && !player->hasUsed("LuanshiCard") && !(player->isNude() && player->isChained());
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.isEmpty();
    }
    
    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty() && Self->isChained()) return NULL;
        LuanshiCard *luanshi_card = new LuanshiCard;
        luanshi_card->addSubcards(cards);
        luanshi_card->setSkillName("luanshi");
        return luanshi_card;
    }
};

LuanshiCard::LuanshiCard()
{
}

bool LuanshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void LuanshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (getSubcards().isEmpty()) {
        source->setChained(true);
        room->broadcastProperty(source, "chained");
        room->setEmotion(source, "chain");
        room->getThread()->trigger(ChainStateChanged, room, source);
    }
    ServerPlayer *target = targets.first();
    target->addMark("luanshi");
    room->addPlayerMark(target, "@luanshi_invalidity");

    foreach(ServerPlayer *p, room->getAllPlayers())
        room->filterCards(p, p->getCards("he"), true);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

class Luanshi : public TriggerSkill
{
public:
    Luanshi() : TriggerSkill("luanshi$")
    {
        events << EventPhaseChanging << Death << GameStart << EventAcquireSkill << EventLoseSkill;
        global = true;
        view_as_skill = new LuanshiViewAsSkill;
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
        if ((triggerEvent == GameStart && target->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "luanshi")) {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.isEmpty()) return false;
            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach(ServerPlayer *p, players) {
                if (!p->hasSkill("luanshi_attach"))
                    room->attachSkillToPlayer(p, "luanshi_attach");
            }
        }
        else if (triggerEvent == EventLoseSkill && data.toString() == "luanshi") {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach(ServerPlayer *p, players) {
                if (p->hasSkill("luanshi_attach"))
                    room->detachSkillFromPlayer(p, "luanshi_attach", true);
            }
        }
        else {
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return false;
            }
            else if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != target || target != room->getCurrent())
                    return false;
            }
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach(ServerPlayer *player, players) {
                if (player->getMark("luanshi") == 0) continue;
                player->removeMark("luanshi");
                room->removePlayerMark(player, "@luanshi_invalidity");

                foreach(ServerPlayer *p, room->getAllPlayers())
                    room->filterCards(p, p->getCards("he"), false);

                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
        }
        return false;
    }
};

class LuanshiInvalidity : public InvaliditySkill
{
public:
    LuanshiInvalidity() : InvaliditySkill("#luanshi-invalidity")
    {
    }

    bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return player->getMark("@luanshi_invalidity") == 0 || skill->isAttachedLordSkill();
    }
};

ThicketPackage::ThicketPackage()
    : Package("thicket")
{
    General *caoren = new General(this, "caoren", "wei"); // WEI 011
    caoren->addSkill(new Jushou);
    caoren->addSkill(new Jiewei);
    caoren->addSkill(new JieweiUse);
    related_skills.insertMulti("jiewei", "#jiewei-use");
    
    General *caopi = new General(this, "caopi$", "wei", 3); // WEI 014
    caopi->addSkill(new Xingshang);
    caopi->addSkill(new Fangzhu);
    caopi->addSkill(new Songwei);

    General *menghuo = new General(this, "menghuo", "shu"); // SHU 014
    menghuo->addSkill(new SavageAssaultAvoid("huoshou"));
    menghuo->addSkill(new Huoshou);
    menghuo->addSkill(new Zaiqi);
    related_skills.insertMulti("huoshou", "#sa_avoid_huoshou");

    General *zhurong = new General(this, "zhurong", "shu", 4, false); // SHU 015
    zhurong->addSkill(new SavageAssaultAvoid("juxiang"));
    zhurong->addSkill(new Juxiang);
    zhurong->addSkill(new Lieren);
    related_skills.insertMulti("juxiang", "#sa_avoid_juxiang");

    General *sunjian = new General(this, "sunjian", "wu"); // WU 009
    sunjian->addSkill("yinghun");
    sunjian->addSkill(new Polu);

    General *lusu = new General(this, "lusu", "wu", 3); // WU 014
    lusu->addSkill(new Haoshi);
    lusu->addSkill(new Dimeng);

    General *dongzhuo = new General(this, "dongzhuo$", "qun", 8); // QUN 006
    dongzhuo->addSkill(new Jiuchi);
    dongzhuo->addSkill(new Roulin);
    dongzhuo->addSkill(new Benghuai);
    dongzhuo->addSkill(new Luanshi);
    dongzhuo->addSkill(new LuanshiInvalidity);
    related_skills.insertMulti("luanshi", "#luanshi-invalidity");

    General *huaxiong = new General(this, "huaxiong", "qun", 6); // QUN 019
    huaxiong->addSkill(new Yaowu);

    addMetaObject<HaoshiCard>();
    addMetaObject<PreDimengCard>();
    addMetaObject<DimengCard>();
    addMetaObject<LuanshiCard>();

    skills << new LuanshiAttach;
}

ADD_PACKAGE(Thicket)


