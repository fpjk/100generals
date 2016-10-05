#ifndef _YJCM_H
#define _YJCM_H

#include "package.h"
#include "card.h"
#include "skill.h"

class YJCMPackage : public Package
{
    Q_OBJECT

public:
    YJCMPackage();
};

class JieyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JieyueCard();
    
    bool targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class MingceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MingceCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class GanluCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GanluCard();
    void swapEquip(ServerPlayer *first, ServerPlayer *second) const;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XuanhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XuanhuoCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class XinzhanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XinzhanCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class PaiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PaiyiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QiceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiceCard();

    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    const Card *validate(CardUseStruct &card_use) const;
};

class DangxianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DangxianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GongqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GongqiCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JiefanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiefanCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class AnxuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AnxuCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QiaoshuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiaoshuiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ExtraCollateralCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ExtraCollateralCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onUse(Room *room, const CardUseStruct &card_use) const;
};

class XiansiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiansiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    //void onEffect(const CardEffectStruct &effect) const;
};

class XiansiSlashCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiansiSlashCard();

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    const Card *validate(CardUseStruct &cardUse) const;
};

class ZongxuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZongxuanCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MiejiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MiejiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class FenchengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FenchengCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void onEffect(const CardEffectStruct &effect) const;
};

/*class ZhuikongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuikongCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};*/

class LishouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LishouCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class ShenxingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenxingCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BingyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BingyiCard();

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SidiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SidiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuaiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuaiyiCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuaiyiSnatchCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuaiyiSnatchCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif

