#include "settings.h"
#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "engine.h"
#include "ai.h"
#include "general.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

#include "json.h"


class Yinghun : public PhaseChangeSkill
{
public:
    Yinghun() : PhaseChangeSkill("yinghun")
    {
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start;
    }

    bool onPhaseChange(ServerPlayer *sunjian) const
    {
        Room *room = sunjian->getRoom();
        if (room->askForSkillInvoke(sunjian, objectName(), "lose"))
            room->loseMaxHp(sunjian);
        int x = sunjian->getGeneralMaxHp() - sunjian->getMaxHp();
        if (x > 0) {
            ServerPlayer *to = room->askForPlayerChosen(sunjian, room->getOtherPlayers(sunjian), objectName(), "yinghun-invoke", true, true);
            if (to) {

                int index = 1;
                if (!sunjian->hasInnateSkill("yinghun") && sunjian->hasSkill("hunshang"))
                    index += 2;

                {
                    to->setFlags("YinghunTarget");
                    QString choice = room->askForChoice(sunjian, objectName(), "tx+dx");
                    to->setFlags("-YinghunTarget");
                    if (choice == "tx") {
                        room->broadcastSkillInvoke(objectName(), index + 1);
                        room->askForDiscard(to, objectName(), x, x, false, true);
                    }
                    else {
                        room->broadcastSkillInvoke(objectName(), index);

                        to->drawCards(x, objectName());
                    }
                }
            }
        }
        return false;
    }
};

class Jiang : public TriggerSkill
{
public:
    Jiang() : TriggerSkill("jiang")
    {
        events << CardFinished;
        //frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *sunce, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length()==1 && use.card->isKindOf("Slash")) {
            ServerPlayer *to = use.to.first();
            if (to->isAlive() && sunce->askForSkillInvoke(this, data)) {
                room->broadcastSkillInvoke(objectName());
                sunce->drawCards(1, objectName());
                Duel *duel = new Duel(Card::NoSuit, 0);
                if (to->isCardLimited(duel, Card::MethodUse)) {
                    delete(duel);
                    return false;
                }
                duel->setSkillName("_jiang");
                room->useCard(CardUseStruct(duel, to, sunce));
            }
        }            
        return false;
    }
};

class Hunshang : public TriggerSkill
{
public:
    Hunshang() : TriggerSkill("hunshang")
    {
        events << DamageInflicted << PreHpLost;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunce, QVariant &data) const
    {
        int lose = (triggerEvent == DamageInflicted)? data.value<DamageStruct>().damage: data.toInt();
        if (lose >= sunce->getHp()) { 
            room->loseMaxHp(sunce, lose);
            if (sunce->isAlive() && sunce->getMark("hunshang") == 0) {
                Room *room = sunce->getRoom();
                room->notifySkillInvoked(sunce, objectName());

                room->broadcastSkillInvoke(objectName());
                //room->doLightbox("$HunshangAnimate", 5000);

                room->doSuperLightbox("sunce", "hunshang");
                room->addPlayerMark(sunce, "@waked");
                room->setPlayerMark(sunce, "hunshang", 1);
                if (sunce->getMark("hunshang") == 1)
                    room->acquireSkill(sunce, "yinghun");
            }
            return true;
        }
        return false;
    }
};

class Zhiba : public TriggerSkill
{
public:
    Zhiba() : TriggerSkill("zhiba")
    {
        events << BeforeCardsMove << SlashHit << SlashMissed;
        frequency = Compulsory;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() ;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                const Card *zhiba_card = move.reason.m_extraData.value<const Card *>();
                if (!zhiba_card || !zhiba_card->isKindOf("Slash") || !zhiba_card->hasFlag("zhiba"))
                    return false;
                ServerPlayer *zhiba_target = room->getTag("zhiba_target").value<ServerPlayer *>();
                room->removeTag("zhiba_target");
                if (zhiba_target) {
                    room->broadcastSkillInvoke(objectName());
                    zhiba_target->obtainCard(zhiba_card);
                    move.removeCardIds(move.card_ids);
                    data = QVariant::fromValue(move);
                }
            }
        }
        else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->hasLordSkill("zhiba") && effect.from->getKingdom() == "wu" && effect.from != effect.to) {
                if (triggerEvent == SlashHit)  { room->broadcastSkillInvoke(objectName(), 1); }
                else { room->broadcastSkillInvoke(objectName(), 4); }
            }
        }
        return false;
    }
};

class ZhibaRecord : public TriggerSkill
{
public:
    ZhibaRecord() : TriggerSkill("#zhiba-record")
    {
        events << DamageCaused;
        global = true;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        //const Card *card = NULL;
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = damage.card;
        if (card && card->isKindOf("Slash") && damage.to->hasLordSkill("zhiba") && damage.from->getKingdom() == "wu" && damage.from != damage.to) {
            if (room->askForSkillInvoke(damage.from, "zhiba")) {
                room->broadcastSkillInvoke("zhiba", 2);
                damage.from->drawCards(1);
                QList<int> ids;
                if (!card->isVirtualCard())
                    ids << card->getEffectiveId();
                else if (card->subcardsLength() > 0)
                    ids = card->getSubcards();
                if (!ids.isEmpty()) {
                    room->setCardFlag(card, "zhiba");
                    room->setTag("zhiba_target", QVariant::fromValue(damage.to));
                }
                return true;
            }
            else {
                room->broadcastSkillInvoke("zhiba", 3);
                if (!damage.to->isKongcheng()) {
                    QList<int> ids;
                    foreach(const Card *card, damage.to->getHandcards()) {
                        if (card->isKindOf("Jink"))
                            ids << card->getEffectiveId();
                    }

                    int card_id = room->doGongxin(damage.from, damage.to, ids);
                    if (card_id == -1) return false;

                    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, damage.from->objectName(), QString(), "gongxin", QString());
                    room->throwCard(Sanguosha->getCard(card_id), reason, damage.to, damage.from);
                }
            }
        }

        return false;
    }
};

class Mengjin : public TriggerSkill
{
public:
    Mengjin() :TriggerSkill("mengjin")
    {
        events << TargetSpecified << SlashHit << CardFinished;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *pangde, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (!pangde->isAlive() || pangde->isNude())
                    break;
                if (!p->isAlive() || !pangde->canDiscard(p, "he"))
                    continue;
                if (pangde->askForSkillInvoke(this, QVariant::fromValue(pangde))) {
                    room->broadcastSkillInvoke(objectName());
                    int id = room->askForCardChosen(pangde, p, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, p, pangde);
                    room->addPlayerMark(p, objectName() + use.card->toString());
                }            
            }
        }
        else if(triggerEvent == SlashHit) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            ServerPlayer *to = effect.to;
            if (to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (pangde->isAlive() && pangde->canDiscard(to, "he")) {
                int id = room->askForCardChosen(pangde, to, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, to, pangde);
            }
            room->removePlayerMark(to, objectName() + effect.slash->toString());
        }
        else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }
        
        return false;
    }
};

class Changqu : public TriggerSkill
{
public:
    Changqu() : TriggerSkill("changqu")
    {
        events << EventPhaseStart;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Start || player->isKongcheng() || player->getEquips().isEmpty())
            return false;

        foreach (ServerPlayer *xuhuang, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(xuhuang)
                && player != xuhuang && !xuhuang->isKongcheng()
                && room->askForSkillInvoke(xuhuang, objectName())) {
                room->broadcastSkillInvoke("changqu");
                if (xuhuang->pindian(player, objectName(), NULL)) {
                    int disc = room->askForCardChosen(xuhuang, player, "e", objectName(), false, Card::MethodDiscard);
                    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, xuhuang->objectName(), player->objectName(),
                        objectName(), QString());
                    room->throwCard(Sanguosha->getCard(disc), reason, player, xuhuang);
                } else {
                    room->setFixedDistance(player, xuhuang, 1);
                    QVariantList changqulist = player->tag[objectName()].toList();
                    changqulist.append(QVariant::fromValue(xuhuang));
                    player->tag[objectName()] = QVariant::fromValue(changqulist);
                }
            }
        }
        return false;
    }
};

class ChangquClear : public TriggerSkill
{
public:
    ChangquClear() : TriggerSkill("#changqu-clear")
    {
        events << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QVariantList changqulist = player->tag["changqu"].toList();
        if (changqulist.isEmpty()) return false;
        foreach (QVariant p, changqulist) {
            ServerPlayer *xuhuang = p.value<ServerPlayer *>();
            room->removeFixedDistance(player, xuhuang, 1);
        }
        player->tag.remove("changqu");
        return false;
    }
};

ShensuCard::ShensuCard()
{
    mute = true;
}

bool ShensuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shensu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void ShensuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_shensu");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class ShensuViewAsSkill : public ViewAsSkill
{
public:
    ShensuViewAsSkill() : ViewAsSkill("shensu")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@shensu");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")) {
            return cards.isEmpty() ? new ShensuCard : NULL;
        } else {
            if (cards.length() != 1)
                return NULL;

            ShensuCard *card = new ShensuCard;
            card->addSubcards(cards);

            return card;
        }
    }
};

class Shensu : public TriggerSkill
{
public:
    Shensu() : TriggerSkill("shensu")
    {
        events << EventPhaseChanging;
        view_as_skill = new ShensuViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
            && !xiahouyuan->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(xiahouyuan) && room->askForUseCard(xiahouyuan, "@@shensu1", "@shensu1", 1)) {
                xiahouyuan->skip(Player::Judge, true);
                xiahouyuan->skip(Player::Draw, true);
            }
        } else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)) {
            if (xiahouyuan->canDiscard(xiahouyuan, "he") && room->askForUseCard(xiahouyuan, "@@shensu2", "@shensu2", 2, Card::MethodDiscard))
                xiahouyuan->skip(Player::Play, true);
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(this) && player->hasSkill("baobian"))
            index += 2;
        return index;
    }
};

class Suzi : public TriggerSkill
{
public:
    Suzi() : TriggerSkill("suzi")
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
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (!xiahouyuan || xiahouyuan == death.who || !killer || killer != xiahouyuan)
            return false;
        if (room->askForSkillInvoke(xiahouyuan, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            QList<int> card_ids;
            foreach(const Card *equip, player->getEquips())
                card_ids.append(equip->getId());
            foreach(const Card *hand, player->getHandcards()) {
                if (hand->getTypeId() == Card::TypeEquip)
                    card_ids.append(hand->getEffectiveId());    
            }
            if (card_ids.isEmpty())
                return false;
            while (card_ids.length() >= 1) {
                room->fillAG(card_ids, xiahouyuan);
                int id = room->askForAG(xiahouyuan, card_ids, true, objectName());
                if (id == -1) {
                    room->clearAG(xiahouyuan);
                    break;
                }
                card_ids.removeOne(id);
                room->useCard(CardUseStruct(Sanguosha->getCard(id), xiahouyuan, xiahouyuan));
                room->clearAG(xiahouyuan);
            }
        }
        return false;
    }
};

class Liegong : public TriggerSkill
{
public:
    Liegong() : TriggerSkill("liegong")
    {
        frequency = Compulsory;
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || !use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (player->isWounded() ) {
                room->broadcastSkillInvoke(objectName());

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

class LiegongTargetMod : public TargetModSkill
{
public:
    LiegongTargetMod() : TargetModSkill("#liegong-target")
    {
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill("liegong"))
            return 1000;
        else
            return 0;
    }
};

class Jingao : public DistanceSkill
{
public:
    Jingao() : DistanceSkill("jingao")
    {
    }

    int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill(this) && !from->faceUp())
            correct -= 1000;
        if (to->hasSkill(this) && !to->faceUp())
            correct++;

        return correct;
    }
};

XianmouSlashCard::XianmouSlashCard()
{
    mute = true;
}

bool XianmouSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("xianmou");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}


void XianmouSlashCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
        targets.removeOne(target);
    }

    if (targets.length() > 0) {
        room->setTag("XianmouUser", QVariant::fromValue(source));
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_xianmou");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

XianmouCard::XianmouCard()
{
    target_fixed = true;
    mute = true;
}

void XianmouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1, objectName());
    source->turnOver();
    room->askForUseCard(source, "@@xianmou", "@xianmou-slash", -1, Card::MethodNone);
}

class XianmouVS : public ZeroCardViewAsSkill
{
public:
    XianmouVS() : ZeroCardViewAsSkill("xianmou")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XianmouCard");
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern == "@@xianmou";
    }

    const Card *viewAs() const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@xianmou") {
            return new XianmouSlashCard;
        }
        else {
            return new XianmouCard;
        }
    }
};

class Xianmou : public TriggerSkill
{
public:
    Xianmou() : TriggerSkill("xianmou")
    {
        events << Damage;
        view_as_skill = new XianmouVS;
    }
    
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == "xianmou") {
            ServerPlayer *weiyan = room->getTag("XianmouUser").value<ServerPlayer *>();
            if (weiyan) {
                QStringList choices;
                choices << "draw" << "cancel";
                if (weiyan->isWounded())
                    choices << "recover";
                QString choice = room->askForChoice(weiyan, objectName(), choices.join("+"), QVariant::fromValue(weiyan));
                if (choice != "cancel") {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = weiyan;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->notifySkillInvoked(weiyan, objectName());
                    room->broadcastSkillInvoke(objectName());
                    if (choice == "recover")
                        room->recover(weiyan, RecoverStruct(weiyan));
                    else
                        weiyan->drawCards(1, objectName());
                }
            }
        }
        return false;
    }
};

class Hongyan : public FilterSkill
{
public:
    Hongyan() : FilterSkill("hongyan")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("hongyan");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Spade;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return changeToHeart(originalCard->getEffectiveId());
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

TianxiangCard::TianxiangCard()
{
}

void TianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("TianxiangDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_reason = "tianxiang";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class TianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    TianxiangViewAsSkill() : OneCardViewAsSkill("tianxiang")
    {
        filter_pattern = ".|red|.|hand!";
        response_pattern = "@@tianxiang";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        TianxiangCard *tianxiangCard = new TianxiangCard;
        tianxiangCard->addSubcard(originalCard);
        return tianxiangCard;
    }
};

class Tianxiang : public TriggerSkill
{
public:
    Tianxiang() : TriggerSkill("tianxiang")
    {
        events << DamageInflicted;
        view_as_skill = new TianxiangViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data) const
    {
        if (xiaoqiao->canDiscard(xiaoqiao, "h")) {
            xiaoqiao->tag["TianxiangDamage"] = data;
            return room->askForUseCard(xiaoqiao, "@@tianxiang", "@tianxiang-card", -1, Card::MethodDiscard);
        }
        return false;
    }
};

class TianxiangDraw : public TriggerSkill
{
public:
    TianxiangDraw() : TriggerSkill("#tianxiang")
    {
        events << DamageComplete;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isAlive() && damage.transfer && damage.transfer_reason == "tianxiang")
            player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

GuhuoDialog *GuhuoDialog::getInstance(const QString &object, bool left, bool right,
    bool play_only, bool slash_combined, bool delayed_tricks)
{
    static GuhuoDialog *instance;
    if (instance == NULL || instance->objectName() != object)
        instance = new GuhuoDialog(object, left, right, play_only, slash_combined, delayed_tricks);

    return instance;
}

GuhuoDialog::GuhuoDialog(const QString &object, bool left, bool right, bool play_only,
    bool slash_combined, bool delayed_tricks)
    : object_name(object), play_only(play_only),
    slash_combined(slash_combined), delayed_tricks(delayed_tricks)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left) layout->addWidget(createLeft());
    if (right) layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

bool GuhuoDialog::isButtonEnabled(const QString &button_name) const
{
    const Card *card = map[button_name];
    //return !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
    QString allowings = Self->property("allowed_guhuo_dialog_buttons").toString();
    if (allowings.isEmpty())
        return !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
    else {
        if (!allowings.split("+").contains(card->objectName())) // for OLDB~
            return false;
        else
            return !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
    }
}

void GuhuoDialog::popup()
{
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }

    bool has_enabled_button = false;
    foreach (QAbstractButton *button, group->buttons()) {
        bool enabled = isButtonEnabled(button->objectName());
        if (enabled)
            has_enabled_button = true;
        button->setEnabled(enabled);
    }
    if (!has_enabled_button) {
        emit onButtonClick();
        return;
    }

    Self->tag.remove(object_name);
    exec();
}

void GuhuoDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    if (button->objectName().contains("slash")) {
        if (objectName() == "guhuo")
            Self->tag["GuhuoSlash"] = button->objectName();
        else if (objectName() == "nosguhuo")
            Self->tag["NosGuhuoSlash"] = button->objectName();
    }
    emit onButtonClick();
    accept();
}

QGroupBox *GuhuoDialog::createLeft()
{
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())
            && !(slash_combined && map.contains("slash") && card->objectName().contains("slash"))) {
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));

            if (!slash_combined && card->objectName() == "slash"
                && !ServerInfo.Extensions.contains("!maneuvering")) {
                Card *c2 = Sanguosha->cloneCard(card->objectName());
                c2->setParent(this);
                layout->addWidget(createButton(c2));
            }
        }
    }

    layout->addStretch();
    box->setLayout(layout);
    return box;
}

QGroupBox *GuhuoDialog::createRight()
{
    QGroupBox *box = new QGroupBox(Sanguosha->translate("trick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target_trick"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_target_trick"));
    QVBoxLayout *layout2 = new QVBoxLayout;

    QGroupBox *box3 = new QGroupBox(Sanguosha->translate("delayed_trick"));
    QVBoxLayout *layout3 = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeTrick && (delayed_tricks || card->isNDTrick())
            && !map.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setSkillName(object_name);
            c->setParent(this);

            QVBoxLayout *layout;
            if (c->isKindOf("DelayedTrick"))
                layout = layout3;
            else if (c->isKindOf("SingleTargetTrick"))
                layout = layout1;
            else
                layout = layout2;
            layout->addWidget(createButton(c));
        }
    }

    box->setLayout(layout);
    box1->setLayout(layout1);
    box2->setLayout(layout2);
    box3->setLayout(layout3);

    layout1->addStretch();
    layout2->addStretch();
    layout3->addStretch();

    layout->addWidget(box1);
    layout->addWidget(box2);
    if (delayed_tricks)
        layout->addWidget(box3);
    return box;
}

QAbstractButton *GuhuoDialog::createButton(const Card *card)
{
    if (card->objectName() == "slash" && map.contains(card->objectName()) && !map.contains("normal_slash")) {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate("normal_slash"));
        button->setObjectName("normal_slash");
        button->setToolTip(card->getDescription());

        map.insert("normal_slash", card);
        group->addButton(button);

        return button;
    } else {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
        button->setObjectName(card->objectName());
        button->setToolTip(card->getDescription());

        map.insert(card->objectName(), card);
        group->addButton(button);

        return button;
    }
}

GuhuoCard::GuhuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool GuhuoCard::guhuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach(int card_id, getSubcards())
        used_cards << card_id;
    room->setTag("GuhuoType", user_string);

    ServerPlayer *questioned = NULL;
    foreach (ServerPlayer *player, players) {
        if (player->hasSkill("chanyuan")) {
            LogMessage log;
            log.type = "#Chanyuan";
            log.from = player;
            log.arg = "chanyuan";
            room->sendLog(log);

            room->notifySkillInvoked(player, "chanyuan");
            room->broadcastSkillInvoke("chanyuan");
            room->setEmotion(player, "no-question");
            continue;
        }

        QString choice = room->askForChoice(player, "guhuo", "noquestion+question");
        if (choice == "question")
            room->setEmotion(player, "question");
        else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#GuhuoQuery";
        log.from = player;
        log.arg = choice;
        room->sendLog(log);
        if (choice == "question") {
            questioned = player;
            break;
        }
    }

    LogMessage log;
    log.type = "$GuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if (!questioned) {
        success = true;
        foreach(ServerPlayer *player, players)
            room->setEmotion(player, ".");

        CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "guhuo");
        CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    } else {
        const Card *card = Sanguosha->getCard(subcards.first());
        if (user_string == "peach+analeptic")
            success = card->objectName() == yuji->tag["GuhuoSaveSelf"].toString();
        else if (user_string == "slash")
            success = card->objectName().contains("slash");
        else if (user_string == "normal_slash")
            success = card->objectName() == "slash";
        else
            success = card->match(user_string);

        if (success) {
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "guhuo");
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "guhuo"), true);
        }
        foreach (ServerPlayer *player, players) {
            room->setEmotion(player, ".");
            if (success && questioned == player)
                room->acquireSkill(questioned, "chanyuan");
        }
    }
    yuji->tag.remove("GuhuoSaveSelf");
    yuji->tag.remove("GuhuoSlash");
    room->setPlayerFlag(yuji, "GuhuoUsed");
    return success;
}

bool GuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *_card = Self->tag.value("guhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool GuhuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("guhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool GuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("guhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *GuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_guhuo = user_string;
    if (user_string == "slash"
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_guhuo = room->askForChoice(yuji, "guhuo_slash", guhuo_list.join("+"));
        yuji->tag["GuhuoSlash"] = QVariant(to_guhuo);
    }
    room->broadcastSkillInvoke("guhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#GuhuoNoTarget" : "#Guhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_guhuo;
    log.arg2 = "guhuo";

    room->sendLog(log);

    if (guhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_guhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_guhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_guhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("guhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();

        QList<ServerPlayer *> tos = card_use.to;
        foreach (ServerPlayer *to, tos) {
            const ProhibitSkill *skill = room->isProhibited(card_use.from, to, use_card);
            if (skill) {
                if (skill->isVisible()) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = to;
                    log.arg = skill->objectName();
                    log.arg2 = use_card->objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
                card_use.to.removeOne(to);
            }
        }
        return use_card;
    } else
        return NULL;
}

const Card *GuhuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("guhuo");

    QString to_guhuo;
    if (user_string == "peach+analeptic") {
        QStringList guhuo_list;
        guhuo_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "analeptic";
        to_guhuo = room->askForChoice(yuji, "guhuo_saveself", guhuo_list.join("+"));
        yuji->tag["GuhuoSaveSelf"] = QVariant(to_guhuo);
    } else if (user_string == "slash") {
        QStringList guhuo_list;
        guhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            guhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_guhuo = room->askForChoice(yuji, "guhuo_slash", guhuo_list.join("+"));
        yuji->tag["GuhuoSlash"] = QVariant(to_guhuo);
    } else
        to_guhuo = user_string;

    LogMessage log;
    log.type = "#GuhuoNoTarget";
    log.from = yuji;
    log.arg = to_guhuo;
    log.arg2 = "guhuo";
    room->sendLog(log);

    if (guhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_guhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_guhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_guhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("guhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class Guhuo : public OneCardViewAsSkill
{
public:
    Guhuo() : OneCardViewAsSkill("guhuo")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;

        if (player->isKongcheng() || player->hasFlag("GuhuoUsed")
            || pattern.startsWith(".") || pattern.startsWith("@"))
            return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;
        return !player->isKongcheng() && !player->hasFlag("GuhuoUsed");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            GuhuoCard *card = new GuhuoCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("guhuo").value<const Card *>();
        if (c) {
            GuhuoCard *card = new GuhuoCard;
            if (!c->objectName().contains("slash"))
                card->setUserString(c->objectName());
            else
                card->setUserString(Self->tag["GuhuoSlash"].toString());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("guhuo");
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (!card->isKindOf("GuhuoCard"))
            return -2;
        else
            return -1;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        ServerPlayer *current = player->getRoom()->getCurrent();
        if (!current || current->isDead() || current->getPhase() == Player::NotActive) return false;
        return !player->isKongcheng() && !player->hasFlag("GuhuoUsed");
    }
};

class GuhuoClear : public TriggerSkill
{
public:
    GuhuoClear() : TriggerSkill("#guhuo-clear")
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
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("GuhuoUsed"))
                    room->setPlayerFlag(p, "-GuhuoUsed");
            }
        }
        return false;
    }
};

class Chanyuan : public TriggerSkill
{
public:
    Chanyuan() : TriggerSkill("chanyuan")
    {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent) const
    {
        return 5;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName()) return false;
            room->removePlayerMark(player, "@chanyuan");
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
            room->addPlayerMark(player, "@chanyuan");
        }
        if (triggerEvent != EventLoseSkill && !player->hasSkill(this)) return false;

        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->filterCards(p, p->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        return false;
    }
};

class ChanyuanInvalidity : public InvaliditySkill
{
public:
    ChanyuanInvalidity() : InvaliditySkill("#chanyuan-inv")
    {
    }

    bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return skill->objectName() == "chanyuan" || !player->hasSkill("chanyuan")
            || player->getHp() != 1 || skill->isAttachedLordSkill();
    }
};

class Wushen : public FilterSkill
{
public:
    Wushen() : FilterSkill("wushen")
    {
    }

    bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return to_select->getSuit() == Card::Heart && place == Player::PlaceHand;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class WushenTargetMod : public TargetModSkill
{
public:
    WushenTargetMod() : TargetModSkill("#wushen-target")
    {
    }

    int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("wushen") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class Wuhun : public TriggerSkill
{
public:
    Wuhun() : TriggerSkill("wuhun")
    {
        events << PreDamageDone;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.from && damage.from != player) {
            damage.from->gainMark("@nightmare", damage.damage);
            damage.from->getRoom()->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
        }

        return false;
    }
};

class WuhunRevenge : public TriggerSkill
{
public:
    WuhunRevenge() : TriggerSkill("#wuhun")
    {
        events << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill("wuhun");
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *shenguanyu, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != shenguanyu)
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(shenguanyu);

        int max = 0;
        foreach(ServerPlayer *player, players)
            max = qMax(max, player->getMark("@nightmare"));
        if (max == 0) return false;

        QList<ServerPlayer *> foes;
        foreach (ServerPlayer *player, players) {
            if (player->getMark("@nightmare") == max)
                foes << player;
        }

        if (foes.isEmpty())
            return false;

        ServerPlayer *foe;
        if (foes.length() == 1)
            foe = foes.first();
        else
            foe = room->askForPlayerChosen(shenguanyu, foes, "wuhun", "@wuhun-revenge");

        room->notifySkillInvoked(shenguanyu, "wuhun");

        JudgeStruct judge;
        judge.pattern = "Peach,GodSalvation";
        judge.good = true;
        judge.negative = true;
        judge.reason = "wuhun";
        judge.who = foe;

        room->judge(judge);

        if (judge.isBad()) {
            room->broadcastSkillInvoke("wuhun", 2);
            //room->doLightbox("$WuhunAnimate", 3000);
            room->doSuperLightbox("shenguanyu", "wuhun");

            LogMessage log;
            log.type = "#WuhunRevenge";
            log.from = shenguanyu;
            log.to << foe;
            log.arg = QString::number(max);
            log.arg2 = "wuhun";
            room->sendLog(log);

            room->killPlayer(foe);
        } else
            room->broadcastSkillInvoke("wuhun", 3);
        QList<ServerPlayer *> killers = room->getAllPlayers();
        foreach(ServerPlayer *player, killers)
            player->loseAllMarks("@nightmare");

        return false;
    }
};

static bool CompareBySuit(int card1, int card2)
{
    const Card *c1 = Sanguosha->getCard(card1);
    const Card *c2 = Sanguosha->getCard(card2);

    int a = static_cast<int>(c1->getSuit());
    int b = static_cast<int>(c2->getSuit());

    return a < b;
}

class Shelie : public PhaseChangeSkill
{
public:
    Shelie() : PhaseChangeSkill("shelie")
    {
    }

    bool onPhaseChange(ServerPlayer *shenlvmeng) const
    {
        if (shenlvmeng->getPhase() != Player::Draw)
            return false;

        Room *room = shenlvmeng->getRoom();
        if (!shenlvmeng->askForSkillInvoke(objectName()))
            return false;

        room->broadcastSkillInvoke(objectName());

        QList<int> card_ids = room->getNCards(5);
        qSort(card_ids.begin(), card_ids.end(), CompareBySuit);
        room->fillAG(card_ids);

        QList<int> to_get, to_throw;
        while (!card_ids.isEmpty()) {
            int card_id = room->askForAG(shenlvmeng, card_ids, false, "shelie");
            card_ids.removeOne(card_id);
            to_get << card_id;
            // throw the rest cards that matches the same suit
            const Card *card = Sanguosha->getCard(card_id);
            Card::Suit suit = card->getSuit();

            room->takeAG(shenlvmeng, card_id, false);

            QList<int> _card_ids = card_ids;
            foreach (int id, _card_ids) {
                const Card *c = Sanguosha->getCard(id);
                if (c->getSuit() == suit) {
                    card_ids.removeOne(id);
                    room->takeAG(NULL, id, false);
                    to_throw.append(id);
                }
            }
        }
        DummyCard *dummy = new DummyCard;
        if (!to_get.isEmpty()) {
            dummy->addSubcards(to_get);
            shenlvmeng->obtainCard(dummy);
        }
        dummy->clearSubcards();
        if (!to_throw.isEmpty()) {
            dummy->addSubcards(to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, shenlvmeng->objectName(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
        }
        delete dummy;
        room->clearAG();
        return true;
    }
};

GongxinCard::GongxinCard()
{
}

bool GongxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void GongxinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        QList<int> ids;
        foreach (const Card *card, effect.to->getHandcards()) {
            if (card->getSuit() == Card::Heart)
                ids << card->getEffectiveId();
        }

        int card_id = room->doGongxin(effect.from, effect.to, ids);
        if (card_id == -1) return;

        QString result = room->askForChoice(effect.from, "gongxin", "discard+put");
        effect.from->tag.remove("gongxin");
        if (result == "discard") {
            CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), QString(), "gongxin", QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, effect.to, effect.from);
        } else {
            effect.from->setFlags("Global_GongxinOperator");
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "gongxin", QString());
            room->moveCardTo(Sanguosha->getCard(card_id), effect.to, NULL, Player::DrawPile, reason, true);
            effect.from->setFlags("-Global_GongxinOperator");
        }
    }
}

class Gongxin : public ZeroCardViewAsSkill
{
public:
    Gongxin() : ZeroCardViewAsSkill("gongxin")
    {
    }

    const Card *viewAs() const
    {
        return new GongxinCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GongxinCard");
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        int index = qrand() % 2 + 1;
        if (!player->hasInnateSkill(objectName()) && player->getMark("qinxue") > 0)
            index += 2;
        return index;
    }
};

WindPackage::WindPackage()
    :Package("wind")
{
    General *xiahouyuan = new General(this, "xiahouyuan", "wei"); // WEI 008
    xiahouyuan->addSkill(new Shensu);
    xiahouyuan->addSkill(new SlashNoDistanceLimitSkill("shensu"));
    xiahouyuan->addSkill(new Suzi);
    related_skills.insertMulti("shensu", "#shensu-slash-ndl");
    
    General *xuhuang = new General(this, "xuhuang", "wei"); // WEI 010
    xuhuang->addSkill(new Changqu);
    xuhuang->addSkill(new ChangquClear);
    related_skills.insertMulti("changqu", "#changqu-clear");

    General *huangzhong = new General(this, "huangzhong", "shu"); // SHU 008
    huangzhong->addSkill(new Liegong);
    huangzhong->addSkill(new LiegongTargetMod);
    related_skills.insertMulti("liegong", "#liegong-target");

    General *weiyan = new General(this, "weiyan", "shu"); // SHU 009
    weiyan->addSkill(new Jingao);
    weiyan->addSkill(new Xianmou);
        
    General *sunce = new General(this, "sunce$", "wu"); // WU 010
    sunce->addSkill(new Jiang);
    sunce->addSkill(new Hunshang);
    sunce->addSkill(new Zhiba);
    sunce->addSkill(new ZhibaRecord);
    related_skills.insertMulti("zhiba", "#zhiba-record");

    General *xiaoqiao = new General(this, "xiaoqiao", "wu", 3, false); // WU 011
    xiaoqiao->addSkill(new Tianxiang);
    xiaoqiao->addSkill(new TianxiangDraw);
    xiaoqiao->addSkill(new Hongyan);
    related_skills.insertMulti("tianxiang", "#tianxiang");
    
    General *pangde = new General(this, "pangde", "qun"); // QUN 008
    pangde->addSkill("mashu");
    pangde->addSkill(new Mengjin);

    General *yuji = new General(this, "yuji", "qun", 3); // QUN 011
    yuji->addSkill(new Guhuo);
    yuji->addSkill(new GuhuoClear);
    related_skills.insertMulti("guhuo", "#guhuo-clear");
    yuji->addRelateSkill("chanyuan");
    
    General *shenguanyu = new General(this, "shenguanyu", "god", 5, true, true, true); // LE 001
    shenguanyu->addSkill(new Wushen);
    shenguanyu->addSkill(new WushenTargetMod);
    shenguanyu->addSkill(new Wuhun);
    shenguanyu->addSkill(new WuhunRevenge);
    related_skills.insertMulti("wushen", "#wushen-target");
    related_skills.insertMulti("wuhun", "#wuhun");

    General *shenlvmeng = new General(this, "shenlvmeng", "god", 3, true, true, true); // LE 002
    shenlvmeng->addSkill(new Shelie);
    shenlvmeng->addSkill(new Gongxin);

    addMetaObject<ShensuCard>();
    addMetaObject<TianxiangCard>();
    addMetaObject<XianmouCard>();
    addMetaObject<XianmouSlashCard>();
    addMetaObject<GuhuoCard>();
    addMetaObject<GongxinCard>();

    skills << new Chanyuan << new ChanyuanInvalidity << new Yinghun;
    related_skills.insertMulti("chanyuan", "#chanyuan-inv");
}

ADD_PACKAGE(Wind)

