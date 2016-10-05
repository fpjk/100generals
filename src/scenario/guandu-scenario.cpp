#include "scenario.h"
#include "skill.h"
#include "maneuvering.h"
#include "guandu-scenario.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

ZhanShuangxiongCard::ZhanShuangxiongCard()
{
}

bool ZhanShuangxiongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->getGeneralName() == "yanliangwenchou" && !to_select->isKongcheng();
}

void ZhanShuangxiongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setTag("ZhanShuangxiong", true);
    source->pindian(targets.first(), "zhanshuangxiong");
}

class GreatYiji : public NosYiji
{
public:
    GreatYiji() : NosYiji()
    {
        setObjectName("greatyiji");
        n = 3;
    }
};

class DamageBeforePlay : public PhaseChangeSkill
{
public:
    DamageBeforePlay() : PhaseChangeSkill("damagebeforeplay")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() == Player::Play) {
            DamageStruct damage;
            damage.to = target;
            damage.reason = objectName();
            target->getRoom()->damage(damage);
        }
        return false;
    }
};

class ZhanShuangxiongViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ZhanShuangxiongViewAsSkill() : ZeroCardViewAsSkill("zhanshuangxiong")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("ZhanShuangxiongCard");
    }

    const Card *viewAs() const
    {
        return new ZhanShuangxiongCard();
    }
};

class ZhanShuangxiong : public TriggerSkill
{
public:
    ZhanShuangxiong() : TriggerSkill("zhanshuangxiong")
    {
        events << Pindian;
        view_as_skill = new ZhanShuangxiongViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName())
            return false;
        if (pindian->from_card->getNumber() == pindian->to_card->getNumber())
            return false;

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        DamageStruct damage;
        damage.from = winner;
        damage.to = loser;
        damage.reason = objectName();
        room->damage(damage);

        return false;
    }
};

class GuanduRule : public ScenarioRule
{
public:
    GuanduRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << DrawNCards << Damaged << GameOverJudge;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        switch (triggerEvent) {
        case GameStart: {
            player = room->getLord();
            room->installEquip(player, "renwang_shield");
            room->installEquip(player, "hualiu");

            ServerPlayer *caocao = room->findPlayer("caocao");
            room->installEquip(caocao, "qinggang_sword");
            room->installEquip(caocao, "zhuahuangfeidian");

            ServerPlayer *liubei = room->findPlayer("liubei");
            room->installEquip(liubei, "double_sword");

            ServerPlayer *guanyu = room->findPlayer("guanyu");
            room->installEquip(guanyu, "blade");
            room->installEquip(guanyu, "chitu");
            room->acquireSkill(guanyu, "zhanshuangxiong");


            ServerPlayer *zhangliao = room->findPlayer("nos_zhangliao");
            room->handleAcquireDetachSkills(zhangliao, "-nostuxi|smalltuxi");

            break;
        }
        case DrawNCards: {
            if (player->getPhase() == Player::Draw) {
                bool burned = room->getTag("BurnWuchao").toBool();
                if (!burned) {
                    QString name = player->getGeneralName();
                    if (name == "caocao" || name == "nos_guojia" || name == "guanyu")
                        data = data.toInt() - 1;
                }
            }
            break;
        }
        case Damaged: {
            bool burned = room->getTag("BurnWuchao").toBool();
            if (burned) return false;

            DamageStruct damage = data.value<DamageStruct>();
            if (player->getGeneralName() == "yuanshao" && damage.nature == DamageStruct::Fire
                && damage.from->getRoleEnum() == Player::Rebel) {
                room->setTag("BurnWuchao", true);

                QStringList tos;
                tos << "yuanshao" << "yanliangwenchou" << "zhenji" << "liubei";

                foreach (QString name, tos) {
                    ServerPlayer *to = room->findPlayer(name);
                    if (to == NULL || to->containsTrick("supply_shortage"))
                        continue;

                    int card_id = room->getCardFromPile("@duanliang");
                    if (card_id == -1)
                        break;

                    const Card *originalCard = Sanguosha->getCard(card_id);

                    LogMessage log;
                    log.type = "#BurnWuchao";
                    log.from = to;
                    log.card_str = originalCard->toString();
                    room->sendLog(log);

                    SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
                    shortage->setSkillName("duanliang");
                    WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
                    card->takeOver(shortage);
                    room->broadcastUpdateCard(room->getPlayers(), card->getId(), card);
                    room->moveCardTo(card, to, Player::PlaceDelayedTrick, true);
                    shortage->deleteLater();
                }
            }
            break;
        }
        case GameOverJudge: {
            if (player->isLord()) {
                QStringList roles = room->aliveRoles(player);
                if (roles.length() == 2) {
                    QString first = roles.at(0);
                    QString second = roles.at(1);
                    if (first == "renegade" && second == "renegade") {
                        player->bury();
                        room->gameOver("renegade");
                        return true;
                    }
                }
            }
            break;
        }
        default:
            break;
        }

        return false;
    }
};

GuanduScenario::GuanduScenario()
    : Scenario("guandu")
{
    lord = "yuanshao";
    loyalists << "yanliangwenchou" << "zhenji";
    rebels << "caocao" << "zhangliao" << "nos_guojia";
    renegades << "liubei" << "guanyu";

    rule = new GuanduRule(this);

    skills << new ZhanShuangxiong
        << new GreatYiji
        << new DamageBeforePlay;

    addMetaObject<ZhanShuangxiongCard>();
}

AI::Relation GuanduScenario::relationTo(const ServerPlayer *a, const ServerPlayer *b) const
{
    if (a->getRole() == "renegade" && b->getRole() == "renegade")
        return AI::Friend;
    else
        return AI::GetRelation(a, b);
}

void GuanduScenario::onTagSet(Room *room, const QString &) const
{
    bool zhanshuangxiong = room->getTag("ZhanShuangxiong").toBool();
    bool burnwuchao = room->getTag("BurnWuchao").toBool();

    if (zhanshuangxiong && burnwuchao) {
        ServerPlayer *guojia = room->findPlayer("nos_guojia");
        if (guojia && !guojia->hasSkill("greatyiji")) {
            room->detachSkillFromPlayer(guojia, "nosyiji");
            room->acquireSkill(guojia, "greatyiji");
            room->acquireSkill(guojia, "damagebeforeplay", false);
        }
    }
}
