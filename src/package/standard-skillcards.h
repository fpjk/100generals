#ifndef _STANDARD_SKILLCARDS_H
#define _STANDARD_SKILLCARDS_H

#include "skill.h"
#include "card.h"

class TuxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TuxiCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class YijiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YijiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QuhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QuhuCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class RendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RendeCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JijiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JijiangCard();

    bool targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class YijueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YijueCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuzhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuzhuCard();

    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    const Card *validateInResponse(ServerPlayer *liushan) const;
    const Card *validate(CardUseStruct &cardUse) const;
};

class JianyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JianyanCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhihengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhihengCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FenweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FenweiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class KurouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KurouCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FanjianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FanjianCard();
    void onEffect(const CardEffectStruct &effect) const;
};

class GuoseCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuoseCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    const Card *validate(CardUseStruct &cardUse) const;
    void onUse(Room *room, const CardUseStruct &use) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class LiuliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LiuliCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class LianyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianyingCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class JieyinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JieyinCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class ChuliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuliCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LijianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LijianCard(bool cancelable = true);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    void onUse(Room *room, const CardUseStruct &card_use) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

private:
    bool duel_cancelable;
};

class LuanwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuanwuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class HuangtianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuangtianCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class MizhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MizhaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

#endif
