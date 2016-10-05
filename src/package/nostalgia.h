#ifndef _NOSTALGIA_H
#define _NOSTALGIA_H

#include "package.h"
#include "card.h"
#include "standard.h"
#include "standard-skillcards.h"

class NostalStandardPackage : public Package
{
    Q_OBJECT

public:
    NostalStandardPackage();
};

class NosRendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosRendeCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NosKurouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosKurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NosFanjianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosFanjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosYiji : public MasochismSkill
{
public:
    NosYiji();
    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const;

protected:
    int n;
};

#endif

