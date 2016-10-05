#ifndef _SPECIAL3V3_H
#define _SPECIAL3V3_H

#include "package.h"
#include "card.h"
#include "standard-equips.h"

class ZhongyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhongyiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JiuzhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiuzhuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhenweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhenweiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class VsTianxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE VsTianxiangCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class QingnangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingnangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class Special3v3Package : public Package
{
    Q_OBJECT

public:
    Special3v3Package();
};

class Special3v3ExtPackage : public Package
{
    Q_OBJECT

public:
    Special3v3ExtPackage();
};

class New3v3CardPackage : public Package
{
    Q_OBJECT

public:
    New3v3CardPackage();
};

#endif

