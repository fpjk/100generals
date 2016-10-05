#include "special3v3.h"
#include "skill.h"
#include "standard.h"
#include "server.h"
#include "engine.h"
#include "ai.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"
#include "clientstruct.h"

class VsGanglie : public MasochismSkill
{
public:
    VsGanglie() : MasochismSkill("vsganglie")
    {
    }

    void onDamaged(ServerPlayer *xiahou, const DamageStruct &) const
    {
        Room *room = xiahou->getRoom();
        QString mode = room->getMode();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(xiahou)) {
            if ((!mode.startsWith("06_") && !mode.startsWith("04_")) || AI::GetRelation3v3(xiahou, p) == AI::Enemy)
                targets << p;
        }

        ServerPlayer *from = room->askForPlayerChosen(xiahou, targets, objectName(), "vsganglie-invoke", true, true);
        if (!from) return;

        room->broadcastSkillInvoke("ganglie");

        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = xiahou;

        room->judge(judge);
        if (from->isDead()) return;
        if (judge.isGood()) {
            if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                room->damage(DamageStruct(objectName(), xiahou, from));
        }
    }
};

ZhongyiCard::ZhongyiCard()
{
    mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ZhongyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->broadcastSkillInvoke("zhongyi");
    //room->doLightbox("$ZhongyiAnimate");
    room->doSuperLightbox("vs_guanyu", "zhongyi");
    room->removePlayerMark(source, "@loyal");
    source->addToPile("loyal", this);
}

class Zhongyi : public OneCardViewAsSkill
{
public:
    Zhongyi() : OneCardViewAsSkill("zhongyi")
    {
        frequency = Limited;
        limit_mark = "@loyal";
        filter_pattern = ".|red|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && player->getMark("@loyal") > 0;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ZhongyiCard *card = new ZhongyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ZhongyiAction : public TriggerSkill
{
public:
    ZhongyiAction() : TriggerSkill("#zhongyi-action")
    {
        events << DamageCaused << EventPhaseStart << ActionedReset;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QString mode = room->getMode();
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user) return false;
            if (damage.card && damage.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("loyal").isEmpty()) continue;
                    bool on_effect = false;
                    if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_"))
                        on_effect = (AI::GetRelation3v3(player, p) == AI::Friend);
                    else
                        on_effect = (room->askForSkillInvoke(p, "zhongyi", data));
                    if (on_effect) {
                        LogMessage log;
                        log.type = "#ZhongyiBuff";
                        log.from = p;
                        log.to << damage.to;
                        log.arg = QString::number(damage.damage);
                        log.arg2 = QString::number(++damage.damage);
                        room->sendLog(log);
                    }
                }
            }
            data = QVariant::fromValue(damage);
        } else if ((mode == "06_3v3" && triggerEvent == ActionedReset) || (mode != "06_3v3" && triggerEvent == EventPhaseStart)) {
            if (triggerEvent == EventPhaseStart && player->getPhase() != Player::RoundStart)
                return false;
            if (player->getPile("loyal").length() > 0)
                player->clearOnePrivatePile("loyal");
        }
        return false;
    }
};

JiuzhuCard::JiuzhuCard()
{
    target_fixed = true;
}

void JiuzhuCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    room->loseHp(player);
    room->recover(who, RecoverStruct(player));
}

class Jiuzhu : public OneCardViewAsSkill
{
public:
    Jiuzhu() : OneCardViewAsSkill("jiuzhu")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern != "peach" || !player->canDiscard(player, "he") || player->getHp() <= 1) return false;
        QString dyingobj = player->property("currentdying").toString();
        const Player *who = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == dyingobj) {
                who = p;
                break;
            }
        }
        if (!who) return false;
        if (ServerInfo.GameMode.startsWith("06_"))
            return player->getRole().at(0) == who->getRole().at(0);
        else
            return true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        JiuzhuCard *card = new JiuzhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Zhanshen : public TriggerSkill
{
public:
    Zhanshen() : TriggerSkill("zhanshen")
    {
        events << Death << EventPhaseStart;
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            foreach (ServerPlayer *lvbu, room->getAllPlayers()) {
                if (!TriggerSkill::triggerable(lvbu)) continue;
                if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("zhanshen_fight") == 0
                        && AI::GetRelation3v3(lvbu, player) == AI::Friend)
                        lvbu->addMark("zhanshen_fight");
                } else {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("@fight") == 0
                        && room->askForSkillInvoke(player, objectName(), "mark:" + lvbu->objectName()))
                        room->addPlayerMark(lvbu, "@fight");
                }
            }
        } else if (TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start
            && player->getMark(objectName()) == 0
            && player->isWounded()
            && (player->getMark("zhanshen_fight") > 0 || player->getMark("@fight") > 0)) {
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#ZhanshenWake";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$ZhanshenAnimate");
            room->doSuperLightbox("vs_lvbu", "zhanshen");

            if (player->getMark("@fight") > 0)
                room->setPlayerMark(player, "@fight", 0);
            player->setMark("zhanshen_fight", 0);

            room->setPlayerMark(player, "zhanshen", 1);
            if (room->changeMaxHpForAwakenSkill(player)) {
                if (player->getWeapon())
                    room->throwCard(player->getWeapon(), player);
                if (player->getMark("zhanshen") == 1)
                    room->handleAcquireDetachSkills(player, "mashu|shenji");
            }
        }
        return false;
    }
};

class ZhenweiDistance : public DistanceSkill
{
public:
    ZhenweiDistance() : DistanceSkill("#zhenwei")
    {
    }

    int getCorrect(const Player *from, const Player *to) const
    {
        if (ServerInfo.GameMode.startsWith("06_") || ServerInfo.GameMode.startsWith("04_")
            || ServerInfo.GameMode == "08_defense") {
            int dist = 0;
            if (from->getRole().at(0) != to->getRole().at(0)) {
                foreach (const Player *p, to->getAliveSiblings()) {
                    if (p->hasSkill("zhenwei") && p->getRole().at(0) == to->getRole().at(0))
                        dist++;
                }
            }
            return dist;
        } else if (to->getMark("@defense") > 0 && from->getMark("@defense") == 0
            && from->objectName() != to->property("zhenwei_from").toString()) {
            return 1;
        }
        return 0;
    }
};

ZhenweiCard::ZhenweiCard()
{
}

bool ZhenweiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total = Self->getSiblings().length() + 1;
    return targets.length() < total / 2 - 1 && to_select != Self;
}

void ZhenweiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->setPlayerProperty(effect.to, "zhenwei_from", QVariant::fromValue(effect.from->objectName()));
    room->addPlayerMark(effect.to, "@defense");
}

class ZhenweiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ZhenweiViewAsSkill() : ZeroCardViewAsSkill("zhenwei")
    {
        response_pattern = "@@zhenwei";
    }

    const Card *viewAs() const
    {
        return new ZhenweiCard;
    }
};

class Zhenwei : public TriggerSkill
{
public:
    Zhenwei() : TriggerSkill("zhenwei")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new ZhenweiViewAsSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        QString mode = target->getRoom()->getMode();
        return !mode.startsWith("06_") && !mode.startsWith("04_") && mode != "08_defense";
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName(), true))
                return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->property("zhenwei_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "zhenwei_from", QVariant());
                    room->setPlayerMark(p, "@defense", 0);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (!TriggerSkill::triggerable(player) || Sanguosha->getPlayerCount(room->getMode()) <= 3)
                return false;
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->property("zhenwei_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "zhenwei_from", QVariant());
                    room->setPlayerMark(p, "@defense", 0);
                }
            }
            room->askForUseCard(player, "@@zhenwei", "@zhenwei");
        }
        return false;
    }
};

QingnangCard::QingnangCard()
{
}

bool QingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isWounded();
}

bool QingnangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.value(0, Self)->isWounded();
}

void QingnangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.value(0, source);
    room->cardEffect(this, source, target);
}

void QingnangCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class Qingnang : public OneCardViewAsSkill
{
public:
    Qingnang() : OneCardViewAsSkill("qingnang")
    {
        filter_pattern = ".|.|.|hand!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && !player->hasUsed("QingnangCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        return qingnang_card;
    }
};

class VsHongyan : public FilterSkill
{
public:
    VsHongyan() : FilterSkill("vshongyan")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("vshongyan");
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

VsTianxiangCard::VsTianxiangCard()
{
}

void VsTianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("VsTianxiangDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_reason = "vstianxiang";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class VsTianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    VsTianxiangViewAsSkill() : OneCardViewAsSkill("vstianxiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@vstianxiang";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        VsTianxiangCard *vstianxiangCard = new VsTianxiangCard;
        vstianxiangCard->addSubcard(originalCard);
        return vstianxiangCard;
    }
};

class VsTianxiang : public TriggerSkill
{
public:
    VsTianxiang() : TriggerSkill("vstianxiang")
    {
        events << DamageInflicted;
        view_as_skill = new VsTianxiangViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data) const
    {
        if (xiaoqiao->canDiscard(xiaoqiao, "h")) {
            xiaoqiao->tag["VsTianxiangDamage"] = data;
            return room->askForUseCard(xiaoqiao, "@@vstianxiang", "@vstianxiang-card", -1, Card::MethodDiscard);
        }
        return false;
    }

};

class VsTianxiangDraw : public TriggerSkill
{
public:
    VsTianxiangDraw() : TriggerSkill("#vstianxiang")
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
        if (player->isAlive() && damage.transfer && damage.transfer_reason == "vstianxiang")
            player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

class NosGuose : public OneCardViewAsSkill
{
public:
    NosGuose() : OneCardViewAsSkill("nosguose")
    {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

class Jizhi : public TriggerSkill
{
public:
    Jizhi() : TriggerSkill("jizhi")
    {
        frequency = Frequent;
        events << CardUsed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isNDTrick() && room->askForSkillInvoke(yueying, objectName())) {
            room->broadcastSkillInvoke("jizhi");
            yueying->drawCards(1, objectName());
        }

        return false;
    }
};

class VsQicai : public TargetModSkill
{
public:
    VsQicai() : TargetModSkill("vsqicai")
    {
        pattern = "TrickCard";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};


class Duanliang : public OneCardViewAsSkill
{
public:
    Duanliang() : OneCardViewAsSkill("duanliang")
    {
        filter_pattern = "BasicCard,EquipCard|black";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class DuanliangTargetMod : public TargetModSkill
{
public:
    DuanliangTargetMod() : TargetModSkill("#duanliang-target")
    {
        frequency = NotFrequent;
        pattern = "SupplyShortage";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill("duanliang"))
            return 1;
        else
            return 0;
    }
};

New3v3CardPackage::New3v3CardPackage()
    : Package("New3v3Card")
{
    QList<Card *> cards;
    cards << new SupplyShortage(Card::Spade, 1)
        << new SupplyShortage(Card::Club, 12)
        << new Nullification(Card::Heart, 12);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3Card)

Special3v3Package::Special3v3Package()
: Package("Special3v3")
{
    General *vs_xiahoudun = new General(this, "vs_xiahoudun", "wei", 4, true, true, true);
    vs_xiahoudun->addSkill(new VsGanglie);

    General *vs_guanyu = new General(this, "vs_guanyu", "shu", 4, true, true, true);
    vs_guanyu->addSkill("wusheng");
    vs_guanyu->addSkill(new Zhongyi);
    vs_guanyu->addSkill(new ZhongyiAction);
    related_skills.insertMulti("zhongyi", "#zhongyi-action");

    General *vs_zhaoyun = new General(this, "vs_zhaoyun", "shu", 4, true, true, true);
    vs_zhaoyun->addSkill("longdan");
    vs_zhaoyun->addSkill(new Jiuzhu);

    General *vs_lvbu = new General(this, "vs_lvbu", "qun", 4, true, true, true);
    vs_lvbu->addSkill("wushuang");
    vs_lvbu->addSkill(new Zhanshen);
    
    General *vs_xiahouyuan = new General(this, "vs_xiahouyuan", "wei", 4, true, true, true); // WEI 008
    vs_xiahouyuan->addSkill("shensu");
    
    General *vs_xiaoqiao = new General(this, "vs_xiaoqiao", "wu", 3, false, true, true); // WU 011
    vs_xiaoqiao->addSkill(new VsTianxiang);
    vs_xiaoqiao->addSkill(new VsTianxiangDraw);
    vs_xiaoqiao->addSkill(new VsHongyan);
    related_skills.insertMulti("vstianxiang", "#vstianxiang");
    
    General *vs_huatuo = new General(this, "vs_huatuo", "qun", 3, true, true, true);
    vs_huatuo->addSkill(new Qingnang);
    vs_huatuo->addSkill("jijiu");
    
    General *vs_daqiao = new General(this, "vs_daqiao", "wu", 3, false, true, true);
    vs_daqiao->addSkill(new NosGuose);
    vs_daqiao->addSkill("liuli");
    
    General *vs_huangyueying = new General(this, "vs_huangyueying", "shu", 3, false, true, true);
    vs_huangyueying->addSkill(new Jizhi);
    vs_huangyueying->addSkill(new VsQicai);
    
    General *vs_xuhuang = new General(this, "vs_xuhuang", "wei", 4, true, true, true); // WEI 010
    vs_xuhuang->addSkill(new Duanliang);
    vs_xuhuang->addSkill(new DuanliangTargetMod);
    related_skills.insertMulti("duanliang", "#duanliang-target");

    addMetaObject<ZhongyiCard>();
    addMetaObject<JiuzhuCard>();
    addMetaObject<VsTianxiangCard>();
    addMetaObject<QingnangCard>();
}

ADD_PACKAGE(Special3v3)

Special3v3ExtPackage::Special3v3ExtPackage()
: Package("Special3v3Ext")
{
    General *wenpin = new General(this, "wenpin", "wei", 4, true, true, true); // WEI 019
    wenpin->addSkill(new Zhenwei);
    wenpin->addSkill(new ZhenweiDistance);
    related_skills.insert("zhenwei", "#zhenwei");

    addMetaObject<ZhenweiCard>();
}

ADD_PACKAGE(Special3v3Ext)
