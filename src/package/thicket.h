#ifndef _THICKET_H
#define _THICKET_H

#include "package.h"
#include "card.h"
#include "skill.h"

class ThicketPackage : public Package
{
    Q_OBJECT

public:
    ThicketPackage();
};

class LuanshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuanshiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const;
};

class HaoshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HaoshiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class DimengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DimengCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class PreDimengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PreDimengCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Jushou : public PhaseChangeSkill
{
public:
    Jushou();
    bool onPhaseChange(ServerPlayer *target) const;

protected:
    int getJushouDrawNum(ServerPlayer *caoren) const;
};

#endif

