#include "fire.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

class Xunxun : public PhaseChangeSkill
{
public:
    Xunxun() : PhaseChangeSkill("xunxun")
    {
        frequency = Frequent;
    }

    bool onPhaseChange(ServerPlayer *lidian) const
    {
        if (lidian->getPhase() == Player::Draw) {
            Room *room = lidian->getRoom();
            if (room->askForSkillInvoke(lidian, objectName())) {
                room->broadcastSkillInvoke(objectName());
                QList<ServerPlayer *> p_list;
                p_list << lidian;
                QList<int> card_ids = room->getNCards(4);
                QList<int> obtained;
                room->fillAG(card_ids, lidian);
                int id1 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id1);
                obtained << id1;
                room->takeAG(lidian, id1, false, p_list);
                int id2 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id2);
                obtained << id2;
                room->clearAG(lidian);

                room->askForGuanxing(lidian, card_ids, Room::GuanxingDownOnly);
                DummyCard *dummy = new DummyCard(obtained);
                lidian->obtainCard(dummy, false);
                delete dummy;

                return true;
            }
        }

        return false;
    }
};

class Wangxi : public TriggerSkill
{
public:
    Wangxi() : TriggerSkill("wangxi")
    {
        events << Damage << Damaged;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage && !damage.to->hasFlag("Global_DebutFlag"))
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (!target || target == player) return false;
        QList<ServerPlayer *> players;
        players << player << target;
        room->sortByActionOrder(players);

        for (int i = 1; i <= damage.damage; i++) {
            if (!target->isAlive() || !player->isAlive())
                return false;
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target))) {
                room->broadcastSkillInvoke(objectName(), (triggerEvent == Damaged) ? 1 : 2);
                room->drawCards(players, 1, objectName());
            } else {
                break;
            }
        }
        return false;
    }
};

QiangxiCard::QiangxiCard()
{
}

bool QiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return Self->inMyAttackRange(to_select);
}

void QiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->damage(DamageStruct("qiangxi", effect.from, effect.to));
    if (!room->askForCard(effect.from, ".Weapon", "@qiangxi-discard", QVariant())) {
        room->loseHp(effect.from);
    }
}

class Qiangxi : public ZeroCardViewAsSkill
{
public:
    Qiangxi() : ZeroCardViewAsSkill("qiangxi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QiangxiCard");
    }

    const Card *viewAs() const
    {
        return new QiangxiCard;
    }
};

class LuanjiVS : public ViewAsSkill
{
public:
    LuanjiVS() : ViewAsSkill("luanji")
    {
        response_or_use = true;
    }
    
    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("LuanjiUsed");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !to_select->isEquipped() && selected.length() < 2;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class Luanji : public TriggerSkill
{
public:
    Luanji() : TriggerSkill("luanji")
    {
        events << Death << CardFinished;
        view_as_skill = new LuanjiVS;
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
            if (!death.damage)
                return false;
            if (death.damage->card != NULL && death.damage->card->isKindOf("ArcheryAttack") && death.damage->card->getSkillName() == "luanji")
                death.damage->card->setFlags("LuanjiKill");
        } else if (!player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == "luanji") {
                ServerPlayer *yuanshao = room->getCurrent();                
                if (yuanshao) {
                    room->setPlayerFlag(yuanshao, "LuanjiUsed");
                    if (use.card->hasFlag("LuanjiKill"))
                        yuanshao->drawCards(2, "luanji");
                }
            }
        }
        return false;
    }
};

class Xueyi : public MaxCardsSkill
{
public:
    Xueyi() : MaxCardsSkill("xueyi$")
    {
    }

    int getExtra(const Player *target) const
    {
        if (target->hasLordSkill(this)) {
            int extra = 0;
            QList<const Player *> players = target->getAliveSiblings();
            foreach (const Player *player, players) {
                if (player->getKingdom() == "qun")
                    extra += 2;
            }
            return extra;
        } else
            return 0;
    }
};

ShuangxiongCard::ShuangxiongCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ShuangxiongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void ShuangxiongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> card_ids = getSubcards();
    room->fillAG(card_ids, target);
    int id = room->askForAG(target, card_ids, false, "Shuangxiong");
    room->throwCard(id, source, target);
    room->clearAG(target);
    room->setPlayerFlag(target, "ShuangxiongTarget");
    room->setPlayerMark(source, "shuangxiong", (Sanguosha->getCard(id)->isRed())? 1 : 2);
    room->setPlayerMark(source, "ViewAsSkill_shuangxiongEffect", 1);
}

class ShuangxiongViewAsSkill : public ViewAsSkill
{
public:
    ShuangxiongViewAsSkill() : ViewAsSkill("shuangxiong")
    {
    }

    bool isResponseOrUse() const
    {
        return !Self->getMark("shuangxiong");
    }


    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("shuangxiong") != 0 && !player->isNude();
    }
    
    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@shuangxiong";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@shuangxiong") {
            if (selected.isEmpty())
                return true;
            else if (selected.length() == 1) {
                const Card *card = selected.first();
                return to_select->getColor() != card->getColor();
            } else
                return false;            
        }
        else {
            int value = Self->getMark("shuangxiong");
            if (value == 1)
                return to_select->isBlack();
            else if (value == 2)
                return to_select->isRed();
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@shuangxiong") {
            if (cards.length() != 2) return NULL;
            ShuangxiongCard *card = new ShuangxiongCard;
            card->addSubcards(cards);
            return card;
        }
        else {
            if (cards.length() != 1) return NULL;
            const Card *card = cards.first();
            Duel *duel = new Duel(card->getSuit(), card->getNumber());
            duel->addSubcard(card);
            duel->setSkillName("_" + objectName());
            return duel;
        }
    }
};

class Shuangxiong : public TriggerSkill
{
public:
    Shuangxiong() : TriggerSkill("shuangxiong")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ShuangxiongViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (shuangxiong->getPhase() == Player::Start) {
                room->setPlayerMark(shuangxiong, "shuangxiong", 0);
            } else if (shuangxiong->getPhase() == Player::Play && TriggerSkill::triggerable(shuangxiong)) {
                room->askForUseCard(shuangxiong, "@@shuangxiong", "@shuangxiong-card", 1, Card::MethodNone);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && shuangxiong->getMark("ViewAsSkill_shuangxiongEffect") > 0)
                room->setPlayerMark(shuangxiong, "ViewAsSkill_shuangxiongEffect", 0);
        }

        return false;
    }
};

class ShuangxiongPro : public ProhibitSkill
{
public:
    ShuangxiongPro() : ProhibitSkill("#shuangxiongPro")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return !to->hasFlag("ShuangxiongTarget") && card->getSkillName() == "shuangxiong"; // Be care!!!!!!
    }
};

class Lianhuan : public OneCardViewAsSkill
{
public:
    Lianhuan() : OneCardViewAsSkill("lianhuan")
    {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class Niepan : public TriggerSkill
{
public:
    Niepan() : TriggerSkill("niepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nirvana";
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@nirvana") > 0;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return false;

        if (pangtong->askForSkillInvoke(this, data)) {
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$NiepanAnimate");
            room->doSuperLightbox("pangtong", "niepan");

            room->removePlayerMark(pangtong, "@nirvana");

            pangtong->throwAllHandCardsAndEquips();
            QList<const Card *> tricks = pangtong->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
                room->throwCard(trick, reason, NULL);
            }

            room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
            pangtong->drawCards(3, objectName());

            if (pangtong->isChained())
                room->setPlayerProperty(pangtong, "chained", false);

            if (!pangtong->faceUp())
                pangtong->turnOver();
        }

        return false;
    }
};

HuojiCard::HuojiCard()
{
}

bool HuojiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void HuojiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        
        LogMessage log;
        log.type = "$ViewAllCards";
        log.from = effect.from;
        log.to << effect.to;
        log.card_str = IntList2StringList(effect.to->handCards()).join("+");
        room->sendLog(log, effect.from);
        
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "huoji", true);
        room->showCard(effect.to, card_id);
        QString num_str = Sanguosha->getCard(card_id)->getNumberString();
        QString pattern = ".|.|" + num_str + "|hand";
        QString prompt = QString("@huoji:%1::%2").arg(effect.to->objectName()).arg(num_str);
        if (effect.from->isAlive()) {
            const Card *card_to_throw = room->askForCard(effect.from, pattern, prompt);
            if (card_to_throw)
                room->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
        }
    }
}

class Huoji : public ZeroCardViewAsSkill
{
public:
    Huoji() : ZeroCardViewAsSkill("huoji")
    {
    }

    const Card *viewAs() const
    {
        return new HuojiCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("HuojiCard");
    }

};

class Qimen : public ZeroCardViewAsSkill
{
public:
    Qimen() : ZeroCardViewAsSkill("qimen")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern == "jink" && !player->isChained()) || (pattern == "nullification" && player->isChained());
    }

    const Card *viewAs() const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "jink")
        {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName(objectName());
            return jink;
        }
        else
        {
            Nullification *nullification = new Nullification(Card::NoSuit, 0);
            nullification->setSkillName(objectName());
            return nullification;
        }
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->isChained();
    }
};

class QimenFlip : public TriggerSkill
{
public:
    QimenFlip() : TriggerSkill("#qimen-flip")
    {
        events << PreCardUsed << PreCardResponded;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        const Card *card_star = NULL;
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card_star = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card_star = resp.m_card;
        }
        if (card_star->getSkillName() == "qimen") {
            player->setChained(!player->isChained());
            room->broadcastProperty(player, "chained");
            room->setEmotion(player, "chain");
            room->getThread()->trigger(ChainStateChanged, room, player);
        }
        return false;
    }
};

TianyiCard::TianyiCard()
{
}

bool TianyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void TianyiCard::use(Room *room, ServerPlayer *taishici, QList<ServerPlayer *> &targets) const
{
    bool success = taishici->pindian(targets.first(), "tianyi", NULL);
    if (success)
        room->setPlayerFlag(taishici, "TianyiSuccess");
    else
        room->setPlayerCardLimitation(taishici, "use", "Slash", true);
}

class Tianyi : public ZeroCardViewAsSkill
{
public:
    Tianyi() : ZeroCardViewAsSkill("tianyi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("TianyiCard") && !player->isKongcheng();
    }

    const Card *viewAs() const
    {
        return new TianyiCard;
    }
};

class TianyiTargetMod : public TargetModSkill
{
public:
    TianyiTargetMod() : TargetModSkill("#tianyi-target")
    {
        frequency = NotFrequent;
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1000;
        else
            return 0;
    }

    int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }
};

FenxunCard::FenxunCard()
{
}

bool FenxunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void FenxunCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *from = effect.from;
    Room *room = from->getRoom();
    if (subcardsLength() == 1) 
    {
        room->setPlayerFlag(from, "Fenxun1");
    }
    else
    {
        QStringList choices;
        if (!from->hasFlag("Fenxun2") && !from->isChained()) choices << "setChained";
        if (!from->hasFlag("Fenxun3")) choices << "hpLost";
        QString choice = room->askForChoice(from, "fenxun", choices.join("+"));
        
        if (choice == "setChained")
        {
            room->setPlayerFlag(from, "Fenxun2");
            from->setChained(true);
            room->broadcastProperty(from, "chained");
            room->setEmotion(from, "chain");
            room->getThread()->trigger(ChainStateChanged, room, from);
        }
        else
        {
            room->setPlayerFlag(from, "Fenxun3");
            room->loseHp(from, 1);
        }
    }
    if (effect.to->getMark("FenxunTarget") == 0){
        effect.to->addMark("FenxunTarget");
        room->setFixedDistance(effect.from, effect.to, 1);
    }
}

class FenxunViewAsSkill : public ViewAsSkill
{
public:
    FenxunViewAsSkill() : ViewAsSkill("fenxun")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return (player->canDiscard(player, "he") && !player->hasFlag("Fenxun1"))
        || (!player->isChained() && !player->hasFlag("Fenxun2"))
        ||    !player->hasFlag("Fenxun3");
    }
    
    bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.isEmpty() && !Self->hasFlag("Fenxun1");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        FenxunCard *first = new FenxunCard;
        first->addSubcards(cards);
        first->setSkillName(objectName());
        return first;
    }
};

class Fenxun : public TriggerSkill
{
public:
    Fenxun() : TriggerSkill("fenxun")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new FenxunViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *dingfeng, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != dingfeng)
                return false;
        }
        foreach (ServerPlayer *target, room->getOtherPlayers(dingfeng)) {
            if (target->getMark("FenxunTarget") != 0) {
                room->removeFixedDistance(dingfeng, target, 1);
                target->removeMark("FenxunTarget");
            }
        }
        return false;
    }
};

FirePackage::FirePackage()
    : Package("fire")
{
    General *dianwei = new General(this, "dianwei", "wei"); // WEI 012
    dianwei->addSkill(new Qiangxi);
   
    General *lidian = new General(this, "lidian", "wei", 3); // WEI 017
    lidian->addSkill(new Xunxun);
    lidian->addSkill(new Wangxi);

    General *pangtong = new General(this, "pangtong", "shu", 3); // SHU 010
    pangtong->addSkill(new Lianhuan);
    pangtong->addSkill(new Niepan);

    General *wolong = new General(this, "wolong", "shu", 3); // SHU 011
    wolong->addSkill(new Huoji);
    wolong->addSkill(new Qimen);
    wolong->addSkill(new QimenFlip);

    General *taishici = new General(this, "taishici", "wu"); // WU 012
    taishici->addSkill(new Tianyi);
    taishici->addSkill(new TianyiTargetMod);
    related_skills.insertMulti("tianyi", "#tianyi-target");
    
    General *dingfeng = new General(this, "dingfeng", "wu"); // WU 016
    dingfeng->addSkill(new Skill("duanbing", Skill::Compulsory));
    dingfeng->addSkill(new Fenxun);

    General *yuanshao = new General(this, "yuanshao$", "qun"); // QUN 004
    yuanshao->addSkill(new Luanji);
    yuanshao->addSkill(new Xueyi);

    General *yanliangwenchou = new General(this, "yanliangwenchou", "qun"); // QUN 005
    yanliangwenchou->addSkill(new Shuangxiong);
    yanliangwenchou->addSkill(new ShuangxiongPro);
    related_skills.insertMulti("shuangxiong", "#shuangxiongPro");

    addMetaObject<QiangxiCard>();
    addMetaObject<ShuangxiongCard>();
    addMetaObject<HuojiCard>();
    addMetaObject<TianyiCard>();
    addMetaObject<FenxunCard>();
}

ADD_PACKAGE(Fire)
