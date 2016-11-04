#include "mountain.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "generaloverview.h"
#include "clientplayer.h"
#include "client.h"
#include "ai.h"
#include "json.h"
#include "util.h"
#include "room.h"
#include "roomthread.h"

QiaobianCard::QiaobianCard()
{
    mute = true;
}

bool QiaobianCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("qiaobianPhase");
    if (phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if (phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool QiaobianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("qiaobianPhase");
    if (phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
    else if (phase == Player::Play)
        return targets.isEmpty()
        && (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    return false;
}

void QiaobianCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const
{
    Player::Phase phase = (Player::Phase)zhanghe->getMark("qiaobianPhase");
    if (phase == Player::Draw) {
        if (targets.isEmpty())
            return;

        foreach (ServerPlayer *target, targets) {
            if (zhanghe->isAlive() && target->isAlive())
                room->cardEffect(this, zhanghe, target);
        }
    } else if (phase == Player::Play) {
        if (targets.isEmpty())
            return;

        ServerPlayer *from = targets.first();
        if (!from->hasEquip() && from->getJudgingArea().isEmpty())
            return;

        int card_id = room->askForCardChosen(zhanghe, from, "ej", "qiaobian");
        const Card *card = Sanguosha->getCard(card_id);
        Player::Place place = room->getCardPlace(card_id);

        int equip_index = -1;
        if (place == Player::PlaceEquip) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equip->location());
        }

        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (equip_index != -1) {
                if (p->getEquip(equip_index) == NULL)
                    tos << p;
            } else {
                if (!zhanghe->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                    tos << p;
            }
        }

        room->setTag("QiaobianTarget", QVariant::fromValue(from));
        ServerPlayer *to = room->askForPlayerChosen(zhanghe, tos, "qiaobian", "@qiaobian-to:::" + card->objectName());
        if (to)
            room->moveCardTo(card, from, to, place,
            CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
            zhanghe->objectName(), "qiaobian", QString()));
        room->removeTag("QiaobianTarget");
    }
}

void QiaobianCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        const Card *card = room->askForExchange(effect.to, "qiaobian", 1, 1, false, QString("@qiaobian:%1").arg(effect.from->objectName()));
        if (!card)
            card = effect.to->getRandomHandCard();
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.to->objectName(), effect.from->objectName(), "qiaobian", QString());
        room->obtainCard(effect.from, card, reason, false);
    }
}

class QiaobianViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QiaobianViewAsSkill() : ZeroCardViewAsSkill("qiaobian")
    {
        response_pattern = "@@qiaobian";
    }

    const Card *viewAs() const
    {
        return new QiaobianCard;
    }
};

class Qiaobian : public TriggerSkill
{
public:
    Qiaobian() : TriggerSkill("qiaobian")
    {
        events << EventPhaseChanging << FinishJudge;
        view_as_skill = new QiaobianViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging && TriggerSkill::triggerable(zhanghe)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            room->setPlayerMark(zhanghe, "qiaobianPhase", (int)change.to);
            int index = 0;
            switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
            case Player::NotActive: return false;

            case Player::Judge: index = 1; break;
            case Player::Draw: index = 2; break;
            case Player::Play: index = 3; break;
            case Player::Discard: index = 4; break;
            case Player::PhaseNone: Q_ASSERT(false);
            }

            QString discard_prompt = QString("#qiaobian-%1").arg(index);
            QString use_prompt = QString("@qiaobian-%1").arg(index);
            if (index > 0 && !zhanghe->isKongcheng() && zhanghe->askForSkillInvoke(this)) {
                room->broadcastSkillInvoke("qiaobian", index);
                JudgeStruct judge;
                judge.pattern = ".|heart";
                judge.good = false;
                judge.reason = "qiaobian";
                judge.who = zhanghe;
                room->judge(judge);

                if (!zhanghe->isAlive()) return false;
                if (!judge.isGood()) {
                    room->broadcastSkillInvoke("qiaobian", 5);
                    return false;
                }
                room->askForDiscard(zhanghe, objectName(), 1, 1, false, false, discard_prompt);
                if (!zhanghe->isSkipped(change.to) && (index == 2 || index == 3))
                    room->askForUseCard(zhanghe, "@@qiaobian", use_prompt, index);
                zhanghe->skip(change.to, true);
            }
        }
        else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return false;
    }
};

class Beige : public TriggerSkill
{
public:
    Beige() : TriggerSkill("beige")
    {
        events << Damaged << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
                return false;

            foreach (ServerPlayer *caiwenji, room->getAllPlayers()) {
                if (!TriggerSkill::triggerable(caiwenji)) continue;
                if (caiwenji->canDiscard(caiwenji, "he") && room->askForCard(caiwenji, "..", "@beige", data, objectName())) {
                    JudgeStruct judge;
                    judge.good = true;
                    judge.play_animation = false;
                    judge.who = player;
                    judge.reason = objectName();

                    room->judge(judge);

                    Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
                    switch (suit) {
                    case Card::Heart: {
                        room->broadcastSkillInvoke(objectName(), 4);
                        room->recover(player, RecoverStruct(caiwenji));

                        break;
                    }
                    case Card::Diamond: {
                        room->broadcastSkillInvoke(objectName(), 3);
                        player->drawCards(2, objectName());
                        break;
                    }
                    case Card::Club: {
                        room->broadcastSkillInvoke(objectName(), 1);
                        if (damage.from && damage.from->isAlive())
                            room->askForDiscard(damage.from, "beige", 2, 2, false, true);

                        break;
                    }
                    case Card::Spade: {
                        room->broadcastSkillInvoke(objectName(), 2);
                        if (damage.from && damage.from->isAlive())
                            damage.from->turnOver();

                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return false;
    }
};

class Duanchang : public TriggerSkill
{
public:
    Duanchang() : TriggerSkill("duanchang")
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

        if (death.damage && death.damage->from) {
            LogMessage log;
            log.type = "#DuanchangLoseSkills";
            log.from = player;
            log.to << death.damage->from;
            log.arg = objectName();
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            QList<const Skill *> skills = death.damage->from->getVisibleSkillList();
            QStringList detachList;
            foreach (const Skill *skill, skills) {
                if (!skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(death.damage->from, detachList);
            if (death.damage->from->isAlive())
                death.damage->from->gainMark("@duanchang");
        }

        return false;
    }
};

class Tuntian : public TriggerSkill
{
public:
    Tuntian() : TriggerSkill("tuntian")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            player->setMark("tuntian", 0);
        }
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return false;
            
            if (player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                player->addMark("tuntian", move.card_ids.length());
                
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && ((move.reason.m_reason == CardMoveReason::S_REASON_DISMANTLE
                && move.reason.m_playerId == room->getCurrent()->objectName())
                || (move.to && move.to != move.from && move.to_place != Player::PlaceHand && room->getCurrent() == move.to
                && move.reason.m_reason != CardMoveReason::S_REASON_GIVE
                && move.reason.m_reason != CardMoveReason::S_REASON_SWAP))
                && player->askForSkillInvoke(this)) {
                room->broadcastSkillInvoke("tuntian");
                QList<int> ids = room->getNCards(1, false); // For UI
                CardsMoveStruct move(ids, player, Player::PlaceTable,
                    CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "tuntian", QString()));
                room->moveCardsAtomic(move, true);
                player->addToPile("field", ids);
            }
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Discard && player->getMark("tuntian") >= 2 && player->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke("tuntian");
            QList<int> ids = room->getNCards(1, false); // For UI
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "tuntian", QString()));
            room->moveCardsAtomic(move, true);
            player->addToPile("field", ids);
        }

        return false;
    }
};

class TuntianDistance : public DistanceSkill
{
public:
    TuntianDistance() : DistanceSkill("#tuntian-dist")
    {
    }

    int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill("tuntian"))
            return -from->getPile("field").length();
        else
            return 0;
    }
};

class TuntianClear : public DetachEffectSkill
{
public:
    TuntianClear() : DetachEffectSkill("tuntian", "field")
    {
    }
};

class Zaoxian : public PhaseChangeSkill
{
public:
    Zaoxian() : PhaseChangeSkill("zaoxian")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("zaoxian") == 0
            && target->getPile("field").length() >= 2
            && target->isWounded();
    }

    bool onPhaseChange(ServerPlayer *dengai) const
    {
        Room *room = dengai->getRoom();
        room->notifySkillInvoked(dengai, objectName());

        LogMessage log;
        log.type = "#ZaoxianWake";
        log.from = dengai;
        log.arg = QString::number(dengai->getPile("field").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZaoxianAnimate", 4000);

        room->doSuperLightbox("dengai", "zaoxian");

        room->setPlayerMark(dengai, "zaoxian", 1);
        if (room->changeMaxHpForAwakenSkill(dengai) && dengai->getMark("zaoxian") == 1)
            room->acquireSkill(dengai, "jixi");

        return false;
    }
};

class Jixi : public OneCardViewAsSkill
{
public:
    Jixi() : OneCardViewAsSkill("jixi")
    {
        filter_pattern = ".|.|.|field";
        expand_pile = "field";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("field").isEmpty();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
    }
};

WujuCard::WujuCard()
{
}

bool WujuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void WujuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@wuju-slash:" + effect.from->objectName());
    if (!use_slash) {
        room->setPlayerFlag(effect.from, "wuju");
        if (effect.from->canDiscard(effect.to, "he"))
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "wuju", false, Card::MethodDiscard), effect.to, effect.from);
    }
}

class Wuju : public ZeroCardViewAsSkill
{
public:
    Wuju() : ZeroCardViewAsSkill("wuju")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("WujuCard");
    }

    const Card *viewAs() const
    {
        return new WujuCard;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        int index = (player->hasUsed("WujuCard")) ? 2 : 1;
        if (!player->hasInnateSkill(this) && player->hasSkill("baobian"))
            index += 3;
        else if (player->hasArmorEffect("eight_diagram"))
            index = 3;
        return index;
    }
};

class Zhiji : public PhaseChangeSkill
{
public:
    Zhiji() : PhaseChangeSkill("zhiji")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getMark("zhiji") == 0
            && target->getPhase() == Player::Start
            && target->isKongcheng();
    }

    bool onPhaseChange(ServerPlayer *jiangwei) const
    {
        Room *room = jiangwei->getRoom();
        room->notifySkillInvoked(jiangwei, objectName());

        LogMessage log;
        log.type = "#ZhijiWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZhijiAnimate", 4000);

        room->doSuperLightbox("jiangwei", "zhiji");

        room->setPlayerMark(jiangwei, "zhiji", 1);
        if (room->changeMaxHpForAwakenSkill(jiangwei)) {
            if (jiangwei->isWounded() && room->askForChoice(jiangwei, objectName(), "recover+draw") == "recover")
                room->recover(jiangwei, RecoverStruct(jiangwei));
            else
                room->drawCards(jiangwei, 2, objectName());
            if (jiangwei->getMark("zhiji") == 1)
                room->acquireSkill(jiangwei, "guanxing");
        }

        return false;
    }
};

ZhijianCard::ZhijianCard()
{
}

bool ZhijianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void ZhijianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    const Card *card = room->askForCard(source, "EquipCard|.|.|hand", QString("@zhijian:%1").arg(target->objectName()), QVariant(), Card::MethodNone);
    if (!card) return;
    room->useCard(CardUseStruct(card, target, target, false));

    LogMessage log;
    log.type = "$ZhijianEquip";
    log.from = target;
    log.card_str = QString::number(getEffectiveId());
    room->sendLog(log);

    source->drawCards(1, "zhijian");
}

class Zhijian : public ZeroCardViewAsSkill
{
public:
    Zhijian() :ZeroCardViewAsSkill("zhijian")
    {
        //filter_pattern = "EquipCard|.|.|hand";
    }

    const Card *viewAs() const
    {
        return new ZhijianCard();
    }
};

GuzhengCard::GuzhengCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void GuzhengCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->tag["guzheng_card"] = subcards.first();
}

class GuzhengVS : public OneCardViewAsSkill
{
public:
    GuzhengVS() : OneCardViewAsSkill("guzheng")
    {
        response_pattern = "@@guzheng";
    }

    bool viewFilter(const Card *to_select) const
    {
        QStringList l = Self->property("guzheng_toget").toString().split("+");
        QList<int> li = StringList2IntList(l);
        return li.contains(to_select->getId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        GuzhengCard *gz = new GuzhengCard;
        gz->addSubcard(originalCard);
        return gz;
    }
};

class Guzheng : public TriggerSkill
{
public:
    Guzheng() : TriggerSkill("guzheng")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
        view_as_skill = new GuzhengVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

            if (!current || player == current || current->getPhase() != Player::Discard)
                return false;

            QVariantList guzhengToGet = player->tag["GuzhengToGet"].toList();

            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int i = 0;
                foreach (int card_id, move.card_ids) {
                    if (move.from == current && move.from_places[i] == Player::PlaceHand)
                        guzhengToGet << card_id;
                    i++;
                }
            }

            player->tag["GuzhengToGet"] = guzhengToGet;
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard) {
            ServerPlayer *erzhang = room->findPlayerBySkillName(objectName());
            if (erzhang == NULL)
                return false;

            QVariantList guzheng_cardsToGet = erzhang->tag["GuzhengToGet"].toList();
            erzhang->tag.remove("GuzhengToGet");

            if (player->isDead())
                return false;

            QList<int> cardsToGet;
            foreach (QVariant card_data, guzheng_cardsToGet) {
                int card_id = card_data.toInt();
                if (room->getCardPlace(card_id) == Player::DiscardPile)
                    cardsToGet << card_id;
            }

            if (cardsToGet.isEmpty())
                return false;

            /*QString cardsList = IntList2StringList(cards).join("+");
            room->setPlayerProperty(erzhang, "guzheng_allCards", cardsList);*/
            QString toGetList = IntList2StringList(cardsToGet).join("+");
            room->setPlayerProperty(erzhang, "guzheng_toget", toGetList);

            erzhang->tag.remove("guzheng_card");
            room->setPlayerFlag(erzhang, "guzheng_InTempMoving");
            CardMoveReason r(CardMoveReason::S_REASON_UNKNOWN, erzhang->objectName());
            CardsMoveStruct fake_move(cardsToGet, NULL, erzhang, Player::DiscardPile, Player::PlaceHand, r);
            QList<CardsMoveStruct> moves;
            moves << fake_move;
            QList<ServerPlayer *> _erzhang;
            _erzhang << erzhang;
            room->notifyMoveCards(true, moves, true, _erzhang);
            room->notifyMoveCards(false, moves, true, _erzhang);
            bool invoke = room->askForUseCard(erzhang, "@@guzheng", "@guzheng:" + player->objectName(), -1, Card::MethodNone);
            CardsMoveStruct fake_move2(cardsToGet, erzhang, NULL, Player::PlaceHand, Player::DiscardPile, r);
            QList<CardsMoveStruct> moves2;
            moves2 << fake_move2;
            room->notifyMoveCards(true, moves2, true, _erzhang);
            room->notifyMoveCards(false, moves2, true, _erzhang);
            room->setPlayerFlag(erzhang, "-guzheng_InTempMoving");

            if (invoke && erzhang->tag.contains("guzheng_card")) {
                bool ok = false;
                int to_back = erzhang->tag["guzheng_card"].toInt(&ok);
                if (ok) {
                    player->obtainCard(Sanguosha->getCard(to_back));
                    cardsToGet.removeOne(to_back);
                    room->fillAG(cardsToGet, erzhang);
                    int id = room->askForAG(erzhang, cardsToGet, true, objectName());
                    if (id == -1) {
                        room->clearAG(erzhang);
                    }
                    cardsToGet.removeOne(id);
                    room->obtainCard(erzhang, id);
                    room->clearAG(erzhang);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard) {
                ServerPlayer *erzhang = room->findPlayerBySkillName(objectName());
                if (erzhang == NULL)
                    return false;
                erzhang->tag.remove("GuzhengToGet");
                erzhang->tag.remove("GuzhengOther");
            }
        }

        return false;
    }
};


class Buqu : public TriggerSkill
{
public:
    Buqu() : TriggerSkill("buqu")
    {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != zhoutai)
            return false;

        if (zhoutai->getHp() > 0) return false;
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(zhoutai, objectName());
        int need = 1 - zhoutai->getHp();
        for (int i = 0; i < need; i++) {
            int id = room->drawCard();
            int num = Sanguosha->getCard(id)->getNumber();
            bool duplicate = false;
            foreach (int card_id, zhoutai->getPile("wound")) {
                if (Sanguosha->getCard(card_id)->getNumber() == num) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, zhoutai->objectName(), objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
            } else {
                zhoutai->addToPile("wound", id);
                room->recover(zhoutai, RecoverStruct(zhoutai, NULL, 1));
            }
        }
        return false;
    }
};

class BuquMaxCards : public MaxCardsSkill
{
public:
    BuquMaxCards() : MaxCardsSkill("#buqu")
    {
    }

    int getFixed(const Player *target) const
    {
        int len = target->getPile("wound").length();
        if (len > 0)
            return qMin(5, len);
        else
            return -1;
    }
};

class BuquClear : public TriggerSkill
{
public:
    BuquClear() : TriggerSkill("#buqu-clear")
    {
        events << EventLoseSkill;
        frequency = Compulsory;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.toString() == "buqu") {
            int len = player->getPile("wound").length();
            player->clearOnePrivatePile("wound");
            if (len > 0) {
                room->loseHp(player, len);
            }
        }
        return false;
    }
};

class Fenji : public TriggerSkill
{
public:
    Fenji() : TriggerSkill("fenji")
    {
        events << TrickEffect;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Snatch") || effect.card->isKindOf("Dismantlement"))
            && !effect.card->isVirtualCard()) {
            foreach (ServerPlayer *zhoutai, room->getAllPlayers()) {
                if (TriggerSkill::triggerable(zhoutai) && zhoutai->getHp() > 0 && player != zhoutai) {
                    effect.to->setFlags("FenjiMoveFrom"); // For AI
                    bool invoke = room->askForSkillInvoke(zhoutai, objectName(), data);
                    effect.to->setFlags("-FenjiMoveFrom");
                    if (invoke) {
                        room->broadcastSkillInvoke(objectName());
                        room->loseHp(zhoutai);
                        if (effect.to->isAlive())
                            room->drawCards(player, 2, "fenji");
                    }
                }
            }
        }
        return false;
    }
};

class Xiangle : public TriggerSkill
{
public:
    Xiangle() : TriggerSkill("xiangle")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(liushan, objectName());

                QVariant dataforai = QVariant::fromValue(liushan);
                if (!room->askForCard(use.from, ".Basic", "@xiangle-discard", dataforai)) {
                    use.nullified_list << liushan->objectName();
                    data = QVariant::fromValue(use);
                }
            }
        }

        return false;
    }
};

FangquanCard::FangquanCard()
{
}

bool FangquanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void FangquanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ServerPlayer *liushan = effect.from, *player = effect.to;

    LogMessage log;
    log.type = "#Fangquan";
    log.from = liushan;
    log.to << player;
    room->sendLog(log);

    room->setTag("FangquanTarget", QVariant::fromValue(player));
}

class FangquanViewAsSkill : public OneCardViewAsSkill
{
public:
    FangquanViewAsSkill() : OneCardViewAsSkill("fangquan")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@fangquan";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        FangquanCard *fangquan = new FangquanCard;
        fangquan->addSubcard(originalCard);
        return fangquan;
    }
};

class Fangquan : public TriggerSkill
{
public:
    Fangquan() : TriggerSkill("fangquan")
    {
        events << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new FangquanViewAsSkill;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseStart)
            return 1;
        else
            return TriggerSkill::getPriority(triggerEvent);
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (liushan->getMark("fangquan")==1) {
                    if (!liushan->canDiscard(liushan, "h"))
                        return false;
                    room->askForUseCard(liushan, "@@fangquan", "@fangquan-give", -1, Card::MethodDiscard);
                }
            }
        } else if (triggerEvent == EventPhaseStart && liushan->getPhase() == Player::NotActive) {
            Room *room = liushan->getRoom();
            if (!room->getTag("FangquanTarget").isNull()) {
                ServerPlayer *target = room->getTag("FangquanTarget").value<ServerPlayer *>();
                room->removeTag("FangquanTarget");
                if (target->isAlive())
                    target->gainAnExtraTurn();
            }
        }
        return false;
    }
};

class FangquanRecord : public TriggerSkill
{
public:
    FangquanRecord() : TriggerSkill("#fangquan-record")
    {
        events << PreCardUsed << EventPhaseChanging;
    }

    int getPriority(TriggerEvent) const
    {
        return 6;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed && player->isAlive() && player->getPhase() == Player::Play
            && player->getMark("fangquan") == 1) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && !use.card->isKindOf("Peach") && !use.card->isKindOf("GodSalvation")) {
                player->setMark("fangquan", 0);
                return false;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play)
                player->setMark("fangquan", 1);
        }
        return false;
    }
};

class Ruoyu : public PhaseChangeSkill
{
public:
    Ruoyu() : PhaseChangeSkill("ruoyu$")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::Start
            && target->hasLordSkill("ruoyu")
            && target->isAlive()
            && target->getMark("ruoyu") == 0;
    }

    bool onPhaseChange(ServerPlayer *liushan) const
    {
        Room *room = liushan->getRoom();

        bool can_invoke = true;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (liushan->getHp() > p->getHp()) {
                can_invoke = false;
                break;
            }
        }

        if (can_invoke) {
            room->notifySkillInvoked(liushan, objectName());

            LogMessage log;
            log.type = "#RuoyuWake";
            log.from = liushan;
            log.arg = QString::number(liushan->getHp());
            log.arg2 = objectName();
            room->sendLog(log);           
            room->broadcastSkillInvoke(objectName());
            room->doSuperLightbox("liushan", "ruoyu");
            room->setPlayerMark(liushan, "ruoyu", 1);
            if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
                room->recover(liushan, RecoverStruct(liushan));
                if (liushan->getMark("ruoyu") == 1 && liushan->isLord())
                    room->acquireSkill(liushan, "jijiang");
            }
        }

        return false;
    }
};

class Huashen : public GameStartSkill
{
public:
    Huashen() : GameStartSkill("huashen")
    {
    }

    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name)
    {
        zuoci->getRoom()->broadcastSkillInvoke(skill_name, zuoci->isMale(), -1);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n)
    {
        Room *room = zuoci->getRoom();
        QVariantList huashens = zuoci->tag["Huashens"].toList();
        QStringList list = GetAvailableGenerals(zuoci);
        qShuffle(list);
        if (list.isEmpty()) return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        foreach (QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach (const TriggerSkill *skill, general->getTriggerSkills()) {
                    if (skill->isVisible())
                        room->getThread()->addTriggerSkill(skill);
                }
            }
        }
        zuoci->tag["Huashens"] = huashens;

        QStringList hidden;
        for (int i = 0; i < n; i++) hidden << "unknown";
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p == zuoci)
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), acquired.join(":"), QList<ServerPlayer *>() << p);
            else
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), hidden.join(":"), QList<ServerPlayer *>() << p);
        }

        LogMessage log;
        log.type = "#GetHuashen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GetHuashenDetail";
        log2.from = zuoci;
        log2.arg = acquired.join("\\, \\");
        room->sendLog(log2, zuoci);

        room->setPlayerMark(zuoci, "@huashen", huashens.length());
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode())
            || room->getMode().contains("_mini_")
            || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zuoci->tag["Huashens"].toList();
        foreach(QVariant huashen, huashens)
            huashen_set << huashen.toString();
        foreach (ServerPlayer *player, room->getAllPlayers(true)) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty()) {
            banned << "zuoci";
        }

        return (all - banned - huashen_set - room_set).toList();
    }

    static void SelectSkill(ServerPlayer *zuoci)
    {
        Room *room = zuoci->getRoom();
        playAudioEffect(zuoci, "huashen");
        QStringList ac_dt_list;

        QString huashen_skill = zuoci->tag["HuashenSkill"].toString();
        if (!huashen_skill.isEmpty())
            ac_dt_list.append("-" + huashen_skill);

        QVariantList huashens = zuoci->tag["Huashens"].toList();
        if (huashens.isEmpty()) return;

        QStringList huashen_generals;
        foreach(QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        QStringList skill_names;
        QString skill_name;
        const General *general = NULL;
        AI* ai = zuoci->getAI();
        if (ai) {
            QHash<QString, const General *> hash;
            foreach (QString general_name, huashen_generals) {
                const General *general = Sanguosha->getGeneral(general_name);
                foreach (const Skill *skill, general->getVisibleSkillList()) {
                    if (skill->isLordSkill()
                        || skill->getFrequency() == Skill::Limited
                        || skill->getFrequency() == Skill::Wake)
                        continue;

                    if (!skill_names.contains(skill->objectName())) {
                        hash[skill->objectName()] = general;
                        skill_names << skill->objectName();
                    }
                }
            }
            if (skill_names.isEmpty()) return;
            skill_name = ai->askForChoice("huashen", skill_names.join("+"), QVariant());
            general = hash[skill_name];
            Q_ASSERT(general != NULL);
        } else {
            QString general_name = room->askForGeneral(zuoci, huashen_generals);
            general = Sanguosha->getGeneral(general_name);

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill()
                    || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake)
                    continue;

                skill_names << skill->objectName();
            }

            if (!skill_names.isEmpty())
                skill_name = room->askForChoice(zuoci, "huashen", skill_names.join("+"));
        }
        //Q_ASSERT(!skill_name.isNull() && !skill_name.isEmpty());

        QString kingdom = general->getKingdom();
        if (zuoci->getKingdom() != kingdom) {
            if (kingdom == "god")
                kingdom = room->askForKingdom(zuoci);
            room->setPlayerProperty(zuoci, "kingdom", kingdom);
        }

        if (zuoci->getGender() != general->getGender())
            zuoci->setGender(general->getGender());

        JsonArray arg;
        arg << QSanProtocol::S_GAME_EVENT_HUASHEN << zuoci->objectName() << general->objectName() << skill_name;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        zuoci->tag["HuashenSkill"] = skill_name;
        if (!skill_name.isEmpty())
            ac_dt_list.append(skill_name);
        room->handleAcquireDetachSkills(zuoci, ac_dt_list, true);
    }

    void onGameStart(ServerPlayer *zuoci) const
    {
        zuoci->getRoom()->notifySkillInvoked(zuoci, "huashen");
        AcquireGenerals(zuoci, 2);
        SelectSkill(zuoci);
    }

    QDialog *getDialog() const
    {
        static HuashenDialog *dialog;

        if (dialog == NULL)
            dialog = new HuashenDialog;

        return dialog;
    }
};

HuashenDialog::HuashenDialog()
{
    setWindowTitle(Sanguosha->translate("huashen"));
}

void HuashenDialog::popup()
{
    QVariantList huashen_list = Self->tag["Huashens"].toList();
    QList<const General *> huashens;
    foreach(QVariant huashen, huashen_list)
        huashens << Sanguosha->getGeneral(huashen.toString());

    fillGenerals(huashens);
    show();
}

class HuashenSelect : public PhaseChangeSkill
{
public:
    HuashenSelect() : PhaseChangeSkill("#huashen-select")
    {
    }

    int getPriority(TriggerEvent) const
    {
        return 4;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && (target->getPhase() == Player::RoundStart || target->getPhase() == Player::NotActive);
    }

    bool onPhaseChange(ServerPlayer *zuoci) const
    {
        if (zuoci->askForSkillInvoke("huashen"))
            Huashen::SelectSkill(zuoci);
        return false;
    }
};

class HuashenClear : public DetachEffectSkill
{
public:
    HuashenClear() : DetachEffectSkill("huashen")
    {
    }

    void onSkillDetached(Room *room, ServerPlayer *player) const
    {
        if (player->getKingdom() != player->getGeneral()->getKingdom() && player->getGeneral()->getKingdom() != "god")
            room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
        if (player->getGender() != player->getGeneral()->getGender())
            player->setGender(player->getGeneral()->getGender());
        QString huashen_skill = player->tag["HuashenSkill"].toString();
        if (!huashen_skill.isEmpty())
            room->detachSkillFromPlayer(player, huashen_skill, false, true);
        player->tag.remove("Huashens");
        room->setPlayerMark(player, "@huashen", 0);
    }
};

class Xinsheng : public MasochismSkill
{
public:
    Xinsheng() : MasochismSkill("xinsheng")
    {
        frequency = Frequent;
    }

    void onDamaged(ServerPlayer *zuoci, const DamageStruct &damage) const
    {
        if (zuoci->askForSkillInvoke(this)) {
            Huashen::playAudioEffect(zuoci, objectName());
            Huashen::AcquireGenerals(zuoci, damage.damage);
        }
    }
};

MountainPackage::MountainPackage()
    : Package("mountain")
{
    General *zhanghe = new General(this, "zhanghe", "wei"); // WEI 009
    zhanghe->addSkill(new Qiaobian);

    General *dengai = new General(this, "dengai", "wei", 4); // WEI 015
    dengai->addSkill(new Tuntian);
    dengai->addSkill(new TuntianDistance);
    dengai->addSkill(new TuntianClear);
    dengai->addSkill(new Zaoxian);
    dengai->addRelateSkill("jixi");
    related_skills.insertMulti("tuntian", "#tuntian-dist");
    related_skills.insertMulti("tuntian", "#tuntian-clear");

    General *jiangwei = new General(this, "jiangwei", "shu"); // SHU 012
    jiangwei->addSkill(new Wuju);
    jiangwei->addSkill(new Zhiji);

    General *liushan = new General(this, "liushan$", "shu", 3); // SHU 013
    liushan->addSkill(new Xiangle);
    liushan->addSkill(new Fangquan);
    liushan->addSkill(new FangquanRecord);
    liushan->addSkill(new Ruoyu);
    related_skills.insertMulti("fangquan", "#fangquan-record");
    
    General *zhoutai = new General(this, "zhoutai", "wu"); // WU 013
    zhoutai->addSkill(new Buqu);
    zhoutai->addSkill(new BuquMaxCards);
    zhoutai->addSkill(new BuquClear);
    zhoutai->addSkill(new Fenji);
    related_skills.insertMulti("buqu", "#buqu");
    related_skills.insertMulti("buqu", "#buqu-clear");

    General *erzhang = new General(this, "erzhang", "wu", 3); // WU 015
    erzhang->addSkill(new Zhijian);
    erzhang->addSkill(new Guzheng);

    General *zuoci = new General(this, "zuoci", "qun", 3); // QUN 009
    zuoci->addSkill(new Huashen);
    zuoci->addSkill(new HuashenSelect);
    zuoci->addSkill(new HuashenClear);
    zuoci->addSkill(new Xinsheng);
    related_skills.insertMulti("huashen", "#huashen-select");
    related_skills.insertMulti("huashen", "#huashen-clear");

    General *caiwenji = new General(this, "caiwenji", "qun", 3, false); // QUN 012
    caiwenji->addSkill(new Beige);
    caiwenji->addSkill(new Duanchang);

    addMetaObject<QiaobianCard>();
    addMetaObject<WujuCard>();
    addMetaObject<ZhijianCard>();
    addMetaObject<FangquanCard>();
    addMetaObject<GuzhengCard>();

    skills << new Jixi;
}

ADD_PACKAGE(Mountain)

