#include "sp.h"
#include "ai.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "json.h"
#include "settings.h"
#include "clientplayer.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

/*class Moukui : public TriggerSkill
{
public:
    Moukui() : TriggerSkill("moukui")
    {
        events << TargetSpecified << SlashMissed << CardFinished;
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
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                    QString choice;
                    if (!player->canDiscard(p, "he"))
                        choice = "draw";
                    else
                        choice = room->askForChoice(player, objectName(), "draw+discard", QVariant::fromValue(p));
                    if (choice == "draw") {
                        room->broadcastSkillInvoke(objectName(), 1);
                        player->drawCards(1, objectName());
                    } else {
                        room->broadcastSkillInvoke(objectName(), 2);
                        room->setTag("MoukuiDiscard", data);
                        int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                        room->removeTag("MoukuiDiscard");
                        room->throwCard(disc, p, player);
                    }
                    room->addPlayerMark(p, objectName() + use.card->toString());
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isDead() || effect.to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (!effect.from->isAlive() || !effect.to->isAlive() || !effect.to->canDiscard(effect.from, "he"))
                return false;
            if (effect.to->canDiscard(effect.to, "he") && room->askForCard(effect.to, ".|black|.|.", QString("@moukui::%1").arg(effect.from->objectName()), QVariant(), objectName())) {
                room->broadcastSkillInvoke(objectName(), 3);
                room->damage(DamageStruct("moukui", effect.to, effect.from));
                room->removePlayerMark(effect.to, objectName() + effect.slash->toString());
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};

class Fengyin : public MaxCardsSkill
{
public:
    Fengyin() : MaxCardsSkill("fengyin")
    {
    }

    int getFixed(const Player *target) const
    {
        if (target->hasSkill("fengyin"))
            return target->getLostHp() + 2;
        else
            return -1;
    }
};*/

class Baobian : public TriggerSkill
{
public:
    Baobian() : TriggerSkill("baobian")
    {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
                QStringList detachList;
                foreach(QString skill_name, baobian_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList);
                player->tag["BaobianSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
        }

        if (!player->isAlive() || !player->hasSkill(this, true)) return false;

        acquired_skills.clear();
        detached_skills.clear();
        BaobianChange(room, player, 1, "shensu");
        BaobianChange(room, player, 2, "mangzhi");
        BaobianChange(room, player, 3, "wuju");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty())
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
        return false;
    }

private:
    void BaobianChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) const
    {
        QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
        if (player->getHp() <= hp) {
            if (!baobian_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "baobian");
                if (player->getHp() == hp)
                    room->broadcastSkillInvoke("baobian", 4 - hp);
                acquired_skills.append(skill_name);
                baobian_skills << skill_name;
            }
        } else {
            if (baobian_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                baobian_skills.removeOne(skill_name);
            }
        }
        player->tag["BaobianSkills"] = QVariant::fromValue(baobian_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

/*XiaoguoCard::XiaoguoCard()
{
    target_fixed = true;
}

void XiaoguoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    //Room *room = effect.from->getRoom();
    ServerPlayer *current = room->getCurrent();
    if (!room->askForCard(current, "@@xiaoguo-discard!", "@xiaoguo-discard")) {
        room->broadcastSkillInvoke("xiaoguo", 2);
        room->damage(DamageStruct("xiaoguo", source, current));
    }
}

class XiaoguoViewAsSkill : public OneCardViewAsSkill
{
public:
    XiaoguoViewAsSkill() : OneCardViewAsSkill("xiaoguo")
    {
        filter_pattern = "BasicCard";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@xiaoguo";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XiaoguoCard *card = new XiaoguoCard;
        card->addSubcard(originalCard);
        return card;
    }
};*/

class Xiaoguo : public TriggerSkill
{
public:
    Xiaoguo() : TriggerSkill("xiaoguo")
    {
        events << EventPhaseStart;
        //view_as_skill = new XiaoguoViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Finish)
            return false;
        ServerPlayer *yuejin = room->findPlayerBySkillName(objectName());
        if (!yuejin || yuejin == player)
            return false;
        if (yuejin->canDiscard(yuejin, "h") && room->askForCard(yuejin, ".Basic", "@xiaoguo", QVariant(), objectName())) {
            room->broadcastSkillInvoke(objectName(), 1);
            if (!room->askForCard(player, "@@xiaoguodiscard", "@xiaoguo-discard")) {
                room->broadcastSkillInvoke(objectName(), 2);
                room->damage(DamageStruct("xiaoguo", yuejin, player));
            }
        }
        return false;
    }
};


class XiaoguoDiscard : public ViewAsSkill
{
public:
    XiaoguoDiscard() : ViewAsSkill("xiaoguodiscard")
    {

    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@xiaoguodiscard";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->isJilei(to_select))
            return false;

        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (selected.first()->getTypeId() == Card::TypeEquip)
                return true;
            else
                return to_select->getTypeId() == Card::TypeEquip;
        }
        else
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {

        if (cards.length() != 2)
            return NULL;

        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(cards);
        return dummy;
    }
};

/*class XiaoguoDiscard : public ViewAsSkill
{
public:
    XiaoguoDiscard() : ViewAsSkill("xiaoguo-discard")
    {
        response_pattern = "@@xiaoguodiscard!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        /*if (Self->isJilei(to_select))
            return false;

        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (selected.first()->getTypeId() == Card::TypeEquip)
                return true;
            else
                return to_select->getTypeId() == Card::TypeEquip;
        } else
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(cards);
        return dummy;
    }
};*/

ZhoufuCard::ZhoufuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && to_select->getPile("incantation").isEmpty();
}

void ZhoufuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->tag["ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    target->addToPile("incantation", this);
}

class ZhoufuViewAsSkill : public OneCardViewAsSkill
{
public:
    ZhoufuViewAsSkill() : OneCardViewAsSkill("zhoufu")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZhoufuCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        Card *card = new ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Zhoufu : public TriggerSkill
{
public:
    Zhoufu() : TriggerSkill("zhoufu")
    {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new ZhoufuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPile("incantation").length() > 0;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantation").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                judge->who->objectName(),
                QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
            ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(card_id)].value<ServerPlayer *>();
            zhangbao->drawCards(2, objectName());
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("incantation").first();
                ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
                if (zhangbao && zhangbao->isAlive())
                    zhangbao->obtainCard(Sanguosha->getCard(id));
            }
        }
        return false;
    }
};

class Zhuji : public TriggerSkill
{
public:
    Zhuji() : TriggerSkill("zhuji")
    {
        events << DamageCaused << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder || !damage.from)
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (TriggerSkill::triggerable(p) && room->askForSkillInvoke(p, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName());
                    JudgeStruct judge;
                    judge.good = true;
                    judge.play_animation = false;
                    judge.reason = objectName();
                    judge.pattern = ".";
                    judge.who = damage.from;

                    room->judge(judge);
                    if (judge.pattern == "black") {
                        LogMessage log;
                        log.type = "#ZhujiBuff";
                        log.from = p;
                        log.to << damage.to;
                        log.arg = QString::number(damage.damage);
                        log.arg2 = QString::number(++damage.damage);
                        room->sendLog(log);

                        data = QVariant::fromValue(damage);
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && judge->card->isRed())
                    player->obtainCard(judge->card);
            }
        }
        return false;
    }
};

class Kangkai : public TriggerSkill
{
public:
    Kangkai() : TriggerSkill("kangkai")
    {
        events << TargetConfirmed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (!player->isAlive()) break;
                if (TriggerSkill::triggerable(player)) {
                    player->tag["KangkaiSlash"] = data;
                    bool will_use =  player->distanceTo(to) <= 2 && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                    player->tag.remove("KangkaiSlash");
                    if (!will_use) continue;

                    room->broadcastSkillInvoke(objectName());
                    QStringList choices;
                    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
                        if (player->getEquip(i) && !to->getEquip(i))
                            choices << QString::number(i);
                    }
                    choices << "draw";
                    QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(player));
                    if (choice == "draw") {
                        room->broadcastSkillInvoke(objectName(), 2);
                        to->drawCards(1, objectName());
                        if (to != player && (player->isNude() || !room->askForDiscard(player, objectName(), 1, 1, true)))
                            room->loseHp(player);
                    } else {
                        room->broadcastSkillInvoke(objectName(), 1);
                        int index = choice.toInt();
                        const Card *card = player->getEquip(index);
                        room->moveCardTo(card, to, Player::PlaceEquip);
                    }
                }
            }
        }
        return false;
    }
};

class Xianzhu : public TriggerSkill
{
public:
    Xianzhu() : TriggerSkill("xianzhu")
    {
        events << CardsMoveOneTime << EventPhaseStart;                                                                                                                                                                                                                                                                
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0)
                        p->setMark(objectName(), 0);
                }
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive && player->getMark(objectName()) == 0 && move.from && move.from->isAlive()
                && move.from->objectName() != player->objectName() && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QList<int> card_ids;
                foreach (int card_id, move.card_ids) {
                    if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeBasic && room->getCardPlace(card_id) == Player::DiscardPile)
                        card_ids << card_id;
                }
                if (card_ids.isEmpty())
                    return false;
                else if (player->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke("xianzhu");
                    room->fillAG(card_ids, player);
                    int id = room->askForAG(player, card_ids, false, objectName());
                    room->clearAG(player);
                    Card *card = Sanguosha->getCard(id);
                    player->obtainCard(card);
                    player->setMark(objectName(), 1);
                }
            }
        }
        return false;
    }
};

QiangwuCard::QiangwuCard()
{
    target_fixed = true;
}

void QiangwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (subcardsLength() <= source->getHp())
        room->setPlayerFlag(source, "qiangwu");
}

class QiangwuViewAsSkill : public ViewAsSkill
{
public:
    QiangwuViewAsSkill() : ViewAsSkill("qiangwu")
    {
        response_pattern = "@@qiangwu";
    }
    
    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;

        int length = Self->getHandcardNum() / 2;
        return selected.length() < length;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != Self->getHandcardNum() / 2)
            return NULL;
        
        QiangwuCard *card = new QiangwuCard;
        card->addSubcards(cards);
        return card;
    }
};

class Qiangwu : public TriggerSkill
{
public:
    Qiangwu() : TriggerSkill("qiangwu")
    {
        events << EventPhaseChanging;
        view_as_skill = new QiangwuViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->setPlayerMark(player, "qiangwu", 0);
        else if (change.to == Player::Start)
            room->askForUseCard(player, "@@qiangwu", "@qiangwu:::" + QString::number(player->getHandcardNum()/2), -1, Card::MethodDiscard);
        return false;
    }
};

class QiangwuTargetMod : public TargetModSkill
{
public:
    QiangwuTargetMod() : TargetModSkill("#qiangwu-target")
    {
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("qiangwu"))
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("qiangwu"))
            return 1;
        else
            return 0;
    }
};

YinbingCard::YinbingCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void YinbingCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("turban", this);
}

class YinbingViewAsSkill : public ViewAsSkill
{
public:
    YinbingViewAsSkill() : ViewAsSkill("yinbing")
    {
        response_pattern = "@@yinbing";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->getTypeId() != Card::TypeBasic;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0) return NULL;

        Card *acard = new YinbingCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Yinbing : public TriggerSkill
{
public:
    Yinbing() : TriggerSkill("yinbing")
    {
        events << EventPhaseStart << Damaged;
        view_as_skill = new YinbingViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && !player->isNude()) {
            room->askForUseCard(player, "@@yinbing", "@yinbing", -1, Card::MethodNone);
        } else if (triggerEvent == Damaged && !player->getPile("turban").isEmpty()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))) {
                room->sendCompulsoryTriggerLog(player, objectName());

                QList<int> ids = player->getPile("turban");
                room->fillAG(ids, player);
                int id = room->askForAG(player, ids, false, objectName());
                room->clearAG(player);
                CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName(), "turban", QString());
                room->obtainCard(player, Sanguosha->getCard(id), reason, true);
            }
        }

        return false;
    }
};

class YinbingClear : public DetachEffectSkill
{
public:
    YinbingClear() : DetachEffectSkill("yinbing", "turban")
    {
    }
};

class Juedi : public PhaseChangeSkill
{
public:
    Juedi() : PhaseChangeSkill("juedi")
    {
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
            && !target->getPile("turban").isEmpty();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (!room->askForSkillInvoke(target, objectName())) return false;
        room->broadcastSkillInvoke(objectName());
        ServerPlayer *to_give = room->askForPlayerChosen(target, room->getOtherPlayers(target), objectName(), "@juedi", true);
        if (to_give) {
            room->recover(to_give, RecoverStruct(target));
            DummyCard *dummy = new DummyCard(target->getPile("turban"));
            room->obtainCard(to_give, dummy);
            delete dummy;
        } else {
            int len = target->getPile("turban").length();
            target->clearOnePrivatePile("turban");
            if (target->isAlive())
                room->drawCards(target, len, objectName());
        }
        return false;
    }
};

class AocaiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    AocaiViewAsSkill() : ZeroCardViewAsSkill("aocai")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
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
        AocaiCard *aocai_card = new AocaiCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        aocai_card->setUserString(pattern);
        return aocai_card;
    }
};

class Aocai : public TriggerSkill
{
public:
    Aocai() : TriggerSkill("aocai")
    {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        if (player->getPhase() == Player::NotActive
            && (pattern == "slash" || pattern == "jink")
            && room->askForSkillInvoke(player, objectName(), data)) {
            QList<int> ids = room->getNCards(2, false);
            QList<int> enabled, disabled;
            foreach (int id, ids) {
                if (Sanguosha->getCard(id)->objectName().contains(pattern))
                    enabled << id;
                else
                    disabled << id;
            }
            int id = Aocai::view(room, player, ids, enabled, disabled);
            if (id != -1) {
                const Card *card = Sanguosha->getCard(id);
                room->provide(card);
                return true;
            }
        }
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled, QList<int> &disabled)
    {
        int result = -1, index = -1;
        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        room->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");
        if (enabled.isEmpty()) {
            JsonArray arg;
            arg << "." << false << JsonUtils::toJsonArray(ids);
            room->doNotify(player, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, arg);
        } else {
            room->fillAG(ids, player, disabled);
            int id = room->askForAG(player, enabled, true, "aocai");
            if (id != -1) {
                index = ids.indexOf(id);
                ids.removeOne(id);
                result = id;
            }
            room->clearAG(player);
        }

        room->askForGuanxing(player, ids);
        const QList<int> &drawPile = room->getDrawPile();
        /*QList<int> &drawPile = room->getDrawPile();
        for (int i = ids.length() - 1; i >= 0; i--)
            drawPile.prepend(ids.at(i));*/
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(drawPile.length()));
        if (result == -1)
            room->setPlayerFlag(player, "Global_AocaiFailed");
        /*else {
            LogMessage log;
            log.type = "#AocaiUse";
            log.from = player;
            log.arg = "aocai";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }*/
        return result;
    }
};


AocaiCard::AocaiCard()
{
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const
{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

DuwuCard::DuwuCard()
{
    mute = true;
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() /*|| qMax(0, to_select->getHp()) > subcardsLength()*/)
        return false;

    return Self->inMyAttackRange(to_select);
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    int num = effect.from->getHandcardNum();
    effect.from->throwAllHandCards();
    if (num >= effect.to->getHp()) {

        if (effect.to->getHp() <= 1)
            room->broadcastSkillInvoke("duwu", 2);
        else
            room->broadcastSkillInvoke("duwu", 1);

        room->damage(DamageStruct("duwu", effect.from, effect.to));
    }
}

class DuwuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DuwuViewAsSkill() : ZeroCardViewAsSkill("duwu")
    {
        response_pattern = "@@duwu";
    }

    /*bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@duwu";
    }*/

    /*bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }*/

    const Card *viewAs(/*const QList<const Card *> &cards*/) const
    {
        /*(if (cards.length() < Self->getHandcardNum())
            return NULL;

        DuwuCard *card = new DuwuCard;
            card->addSubcards(cards);*/

        return new DuwuCard;
    }
};

class Duwu : public TriggerSkill
{
public:
    Duwu() : TriggerSkill("duwu")
    {
        events << EventPhaseEnd;
        view_as_skill = new DuwuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhugeke, QVariant &) const
    {
        if (zhugeke->getPhase() == Player::Play && TriggerSkill::triggerable(zhugeke)) {
            room->askForUseCard(zhugeke, "@@duwu", "@duwu", -1, Card::MethodDiscard);
        }
        return false;
    }
};

HongyuanCard::HongyuanCard()
{
    mute = true;
}

bool HongyuanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.length() <= 2 && !targets.contains(Self);
}

bool HongyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return to_select != Self && targets.length() < 2;
}

void HongyuanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->setFlags("HongyuanTarget");
}

class HongyuanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    HongyuanViewAsSkill() : ZeroCardViewAsSkill("hongyuan")
    {
        response_pattern = "@@hongyuan";
    }

    const Card *viewAs() const
    {
        return new HongyuanCard;
    }
};

class Hongyuan : public DrawCardsSkill
{
public:
    Hongyuan() : DrawCardsSkill("hongyuan")
    {
        frequency = NotFrequent;
        view_as_skill = new HongyuanViewAsSkill;
    }

    int getDrawNum(ServerPlayer *zhugejin, int n) const
    {
        Room *room = zhugejin->getRoom();
        bool invoke = false;
        if (room->getMode().startsWith("06_"))
            invoke = room->askForSkillInvoke(zhugejin, objectName());
        else
            invoke = room->askForUseCard(zhugejin, "@@hongyuan", "@hongyuan");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            zhugejin->setFlags("hongyuan");
            return n - 1;
        } else
            return n;
    }
};

class HongyuanDraw : public TriggerSkill
{
public:
    HongyuanDraw() : TriggerSkill("#hongyuan")
    {
        events << AfterDrawNCards;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!player->hasFlag("hongyuan"))
            return false;
        player->setFlags("-hongyuan");

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
                if (AI::GetRelation3v3(player, p) == AI::Friend)
                    targets << p;
            } else if (p->hasFlag("HongyuanTarget")) {
                p->setFlags("-HongyuanTarget");
                targets << p;
            }
        }

        if (targets.isEmpty()) return false;
        room->drawCards(targets, 1, "hongyuan");
        return false;
    }
};

class Huanshi : public RetrialSkill
{
public:
    Huanshi() : RetrialSkill("huanshi")
    {

    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->isNude();
    }

    const Card *onRetrial(ServerPlayer *player, JudgeStruct *judge) const
    {
        const Card *card = NULL;
        Room *room = player->getRoom();
        if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
            if (AI::GetRelation3v3(player, judge->who) != AI::Friend) return false;
            QStringList prompt_list;
            prompt_list << "@huanshi-card" << judge->who->objectName()
                << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
            QString prompt = prompt_list.join(":");

            card = room->askForCard(player, "..", prompt, QVariant::fromValue(judge), Card::MethodResponse, judge->who, true);
        } else if (!player->isNude()) {
            QList<int> ids, disabled_ids;
            foreach (const Card *card, player->getCards("he")) {
                if (player->isCardLimited(card, Card::MethodResponse))
                    disabled_ids << card->getEffectiveId();
                else
                    ids << card->getEffectiveId();
            }
            if (!ids.isEmpty() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(judge))) {
                if (judge->who != player && !player->isKongcheng()) {
                    LogMessage log;
                    log.type = "$ViewAllCards";
                    log.from = judge->who;
                    log.to << player;
                    log.card_str = IntList2StringList(player->handCards()).join("+");
                    room->sendLog(log, judge->who);
                }
                judge->who->tag["HuanshiJudge"] = QVariant::fromValue(judge);
                room->fillAG(ids + disabled_ids, judge->who, disabled_ids);
                int card_id = room->askForAG(judge->who, ids, false, objectName());
                room->clearAG(judge->who);
                judge->who->tag.remove("HuanshiJudge");
                card = Sanguosha->getCard(card_id);
            }
        }
        if (card != NULL)
            room->broadcastSkillInvoke(objectName());

        return card;
    }
};

class Mingzhe : public TriggerSkill
{
public:
    Mingzhe() : TriggerSkill("mingzhe")
    {
        events << BeforeCardsMove << CardsMoveOneTime << CardUsed << CardResponded;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() != Player::NotActive) return false;
        if (triggerEvent == BeforeCardsMove || triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player) return false;

            if (triggerEvent == BeforeCardsMove) {
                CardMoveReason reason = move.reason;
                if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    const Card *card;
                    int i = 0;
                    foreach (int card_id, move.card_ids) {
                        card = Sanguosha->getCard(card_id);
                        if (room->getCardOwner(card_id) == player && card->isRed()
                            && (move.from_places[i] == Player::PlaceHand
                            || move.from_places[i] == Player::PlaceEquip)) {
                            player->addMark(objectName());
                        }
                        i++;
                    }
                }
            } else {
                int n = player->getMark(objectName());
                try {
                    for (int i = 0; i < n; i++) {
                        player->removeMark(objectName());
                        if (player->isAlive() && player->askForSkillInvoke(objectName(), data)) {
                            room->broadcastSkillInvoke(objectName());
                            player->drawCards(1, objectName());
                        } else {
                            break;
                        }
                    }
                    player->setMark(objectName(), 0);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        player->setMark(objectName(), 0);
                    throw triggerEvent;
                }
            }
        } else {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                card = resp.m_card;
            }
            if (card && card->isRed() && player->askForSkillInvoke(objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

SPPackage::SPPackage()
: Package("sp")
{
    /*General *fuwan = new General(this, "fuwan", "qun", 4);
    fuwan->addSkill(new Moukui);*/

    General *xiahouba = new General(this, "xiahouba", "shu"); // SP 019
    xiahouba->addSkill(new Baobian);

    General *yuejin = new General(this, "yuejin", "wei", 4, true); // SP 024
    yuejin->addSkill(new Xiaoguo);

    General *zhangbaozhangliang = new General(this, "zhangbaozhangliang", "qun", 3); // SP 025
    zhangbaozhangliang->addSkill(new Zhoufu);
    zhangbaozhangliang->addSkill(new Zhuji);

    General *caoang = new General(this, "caoang", "wei"); // SP 026
    caoang->addSkill(new Kangkai);

    General *zhangshi = new General(this, "zhangshi", "shu", 3, false); // SP 028
    zhangshi->addSkill(new Xianzhu);
    zhangshi->addSkill(new Qiangwu);
    zhangshi->addSkill(new QiangwuTargetMod);
    related_skills.insertMulti("qiangwu", "#qiangwu-target");

    General *zumao = new General(this, "zumao", "wu"); // SP 030
    zumao->addSkill(new Yinbing);
    zumao->addSkill(new YinbingClear);
    zumao->addSkill(new Juedi);
    related_skills.insertMulti("yinbing", "#yinbing-clear");
    
    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);
        
    General *zhugejin = new General(this, "zhugejin", "wu", 3); // WU 018
    zhugejin->addSkill(new Hongyuan);
    zhugejin->addSkill(new HongyuanDraw);
    zhugejin->addSkill(new Huanshi);
    zhugejin->addSkill(new Mingzhe);
    related_skills.insertMulti("hongyuan", "#hongyuan");

    addMetaObject<ZhoufuCard>();
    addMetaObject<QiangwuCard>();
    addMetaObject<YinbingCard>();
    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
    addMetaObject<HongyuanCard>();
    //addMetaObject<XiaoguoCard>();
    
    skills << new XiaoguoDiscard;
}

ADD_PACKAGE(SP)