#include "standard.h"
#include "skill.h"
#include "client.h"
#include "engine.h"
#include "nostalgia.h"
#include "settings.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "room.h"
#include "roomthread.h"

class NosJianxiong : public MasochismSkill
{
public:
    NosJianxiong() : MasochismSkill("nosjianxiong")
    {
    }

    void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if (!card) return;

        QList<int> ids;
        if (card->isVirtualCard())
            ids = card->getSubcards();
        else
            ids << card->getEffectiveId();

        if (ids.isEmpty()) return;
        foreach (int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable) return;
        }
        QVariant data = QVariant::fromValue(damage);
        if (room->askForSkillInvoke(caocao, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            caocao->obtainCard(card);
        }
    }
};

class Fankui : public MasochismSkill
{
public:
    Fankui() : MasochismSkill("fankui")
    {
    }

    void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(simayi, "fankui", data)) {
            room->broadcastSkillInvoke("yangshi");
            int card_id = room->askForCardChosen(simayi, from, "he", "fankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id),
                reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class Guicai : public RetrialSkill
{
public:
    Guicai() : RetrialSkill("guicai")
    {

    }

    const Card *onRetrial(ServerPlayer *player, JudgeStruct *judge) const
    {
        if (player->isKongcheng())
            return NULL;

        QStringList prompt_list;
        prompt_list << "@guicai-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, ".", prompt, QVariant::fromValue(judge), Card::MethodResponse, judge->who, true);
        if (card)
            room->broadcastSkillInvoke("guiming");

        return card;
    }
};

NosYiji::NosYiji() : MasochismSkill("nosyiji")
{
    frequency = Frequent;
    n = 2;
}

void NosYiji::onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const
{
    Room *room = guojia->getRoom();
    int x = damage.damage;
    for (int i = 0; i < x; i++) {
        if (!guojia->isAlive() || !room->askForSkillInvoke(guojia, objectName()))
            return;
        room->broadcastSkillInvoke("nosyiji");

        QList<ServerPlayer *> _guojia;
        _guojia.append(guojia);
        QList<int> yiji_cards = room->getNCards(n, false);

        CardsMoveStruct move(yiji_cards, NULL, guojia, Player::PlaceTable, Player::PlaceHand,
            CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _guojia);
        room->notifyMoveCards(false, moves, false, _guojia);

        QList<int> origin_yiji = yiji_cards;
        while (room->askForYiji(guojia, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
            CardsMoveStruct move(QList<int>(), guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            foreach (int id, origin_yiji) {
                if (room->getCardPlace(id) != Player::DrawPile) {
                    move.card_ids << id;
                    yiji_cards.removeOne(id);
                }
            }
            origin_yiji = yiji_cards;
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);
            if (!guojia->isAlive())
                return;
        }

        if (!yiji_cards.isEmpty()) {
            CardsMoveStruct move(yiji_cards, guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);

            DummyCard *dummy = new DummyCard(yiji_cards);
            guojia->obtainCard(dummy, false);
            delete dummy;
        }
    }
}

class NosTieji : public TriggerSkill
{
public:
    NosTieji() : TriggerSkill("nostieji")
    {
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                p->setFlags("NosTiejiTarget"); // For AI

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                try {
                    room->judge(judge);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        p->setFlags("-NosTiejiTarget");
                    throw triggerEvent;
                }

                if (judge.isGood()) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }

                p->setFlags("-NosTiejiTarget");
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

NosKurouCard::NosKurouCard()
{
    target_fixed = true;
}

void NosKurouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->loseHp(source);
    if (source->isAlive())
        room->drawCards(source, 2, "noskurou");
}

class NosKurou : public ZeroCardViewAsSkill
{
public:
    NosKurou() : ZeroCardViewAsSkill("noskurou")
    {
    }

    const Card *viewAs() const
    {
        return new NosKurouCard;
    }
};

class NosYingzi : public DrawCardsSkill
{
public:
    NosYingzi() : DrawCardsSkill("nosyingzi")
    {
        frequency = Frequent;
    }

    int getDrawNum(ServerPlayer *zhouyu, int n) const
    {
        Room *room = zhouyu->getRoom();
        if (room->askForSkillInvoke(zhouyu, objectName())) {
            room->broadcastSkillInvoke("nosyingzi");
            return n + 1;
        } else
            return n;
    }
};

NosFanjianCard::NosFanjianCard()
{
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "nosfanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(card);
    room->showCard(target, card_id);

    if (card->getSuit() != suit)
        room->damage(DamageStruct("nosfanjian", zhouyu, target));
}

class NosFanjian : public ZeroCardViewAsSkill
{
public:
    NosFanjian() : ZeroCardViewAsSkill("nosfanjian")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("NosFanjianCard");
    }

    const Card *viewAs() const
    {
        return new NosFanjianCard;
    }
};

class NosTiandu : public TriggerSkill
{
public:
    NosTiandu() : TriggerSkill("nostiandu")
    {
        frequency = Frequent;
        events << FinishJudge;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        const Card *card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if (room->getCardPlace(card->getEffectiveId()) == Player::PlaceJudge
            && guojia->askForSkillInvoke(this, data_card)) {
            int index = qrand() % 2 + 1;
            if (Player::isNostalGeneral(guojia, "guojia"))
                index += 2;
            room->broadcastSkillInvoke(objectName(), index);
            guojia->obtainCard(judge->card);
            return false;
        }

        return false;
    }
};

class NosYinghun : public PhaseChangeSkill
{
public:
    NosYinghun() : PhaseChangeSkill("nosyinghun")
    {
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->isWounded();
    }

    bool onPhaseChange(ServerPlayer *sunjian) const
    {
        Room *room = sunjian->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(sunjian, room->getOtherPlayers(sunjian), objectName(), "nosyinghun-invoke", true, true);
        if (to) {
            int x = sunjian->getLostHp();

            int index = 1;

            if (x == 1) {
                room->broadcastSkillInvoke(objectName(), index);

                to->drawCards(1, objectName());
                room->askForDiscard(to, objectName(), 1, 1, false, true);
            } else {
                to->setFlags("NosYinghunTarget");
                QString choice = room->askForChoice(sunjian, objectName(), "d1tx+dxt1");
                to->setFlags("-NosYinghunTarget");
                if (choice == "d1tx") {
                    room->broadcastSkillInvoke(objectName(), index + 1);

                    to->drawCards(1, objectName());
                    room->askForDiscard(to, objectName(), x, x, false, true);
                } else {
                    room->broadcastSkillInvoke(objectName(), index);

                    to->drawCards(x, objectName());
                    room->askForDiscard(to, objectName(), 1, 1, false, true);
                }
            }
        }
        return false;
    }
};

NosRendeCard::NosRendeCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    QDateTime dtbefore = source->tag.value("nosrende", QDateTime(QDate::currentDate(), QTime(0, 0, 0))).toDateTime();
    QDateTime dtafter = QDateTime::currentDateTime();

    if (dtbefore.secsTo(dtafter) > 3 * Config.AIDelay / 1000)
        room->broadcastSkillInvoke("rende");

    source->tag["nosrende"] = QDateTime::currentDateTime();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("nosrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nosrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));
}

NostalStandardPackage::NostalStandardPackage()
    : Package("NostalStandard")
{
    General *nos_caocao = new General(this, "nos_caocao$", "wei", 4, true, true, true);
    nos_caocao->addSkill(new NosJianxiong);

    General *nos_simayi = new General(this, "nos_simayi", "wei", 3, true, true, true);
    nos_simayi->addSkill(new Fankui);
    nos_simayi->addSkill(new Guicai);

    General *nos_guojia = new General(this, "nos_guojia", "wei", 3, true, true, true);
    nos_guojia->addSkill(new NosTiandu);
    nos_guojia->addSkill(new NosYiji);

    General *nos_zhangfei = new General(this, "nos_zhangfei", "shu", 4, true, true, true);
    nos_zhangfei->addSkill("paoxiao");

    General *nos_machao = new General(this, "nos_machao", "shu", 4, true, true, true);
    nos_machao->addSkill("mashu");
    nos_machao->addSkill(new NosTieji);

    General *nos_ganning = new General(this, "nos_ganning", "wu", 4, true, true, true);
    nos_ganning->addSkill("qixi");

    General *nos_huanggai = new General(this, "nos_huanggai", "wu", 4, true, true, true);
    nos_huanggai->addSkill(new NosKurou);

    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3, true, true, true);
    nos_zhouyu->addSkill(new NosYingzi);
    nos_zhouyu->addSkill(new NosFanjian);
    
    General *nos_sunjian = new General(this, "nos_sunjian", "wu", 4, true, true, true);
    nos_sunjian->addSkill(new NosYinghun);

    addMetaObject<NosKurouCard>();
    addMetaObject<NosFanjianCard>();
    addMetaObject<NosRendeCard>();
}

ADD_PACKAGE(NostalStandard)
