#ifndef _MANEUVERING_H
#define _MANEUVERING_H

#include "standard.h"

class Analeptic : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Analeptic(Card::Suit suit, int number);
    QString getSubtype() const;

    static bool IsAvailable(const Player *player, const Card *analeptic = NULL);

    bool isAvailable(const Player *player) const;
    void onUse(Room *room, const CardUseStruct &card_use) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class Fan : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Fan(Card::Suit suit, int number);
};

class GudingBlade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE GudingBlade(Card::Suit suit, int number);
};

class Vine : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Vine(Card::Suit suit, int number);
};

class Triblade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Triblade(Card::Suit suit, int number);
};

class TribladeSkillCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TribladeSkillCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SilverLion : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE SilverLion(Card::Suit suit, int number);

    void onUninstall(ServerPlayer *player) const;
};

class IronChain : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IronChain(Card::Suit suit, int number);

    QString getSubtype() const;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    void onUse(Room *room, const CardUseStruct &card_use) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class FireAttack : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE FireAttack(Card::Suit suit, int number);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class Drowning :public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE Drowning(Card::Suit suit, int number);    
    void onEffect(const CardEffectStruct &effect) const;
};

class SupplyShortage : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SupplyShortage(Card::Suit suit, int number);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void takeEffect(ServerPlayer *target) const;
};

class ManeuveringPackage : public Package
{
    Q_OBJECT

public:
    ManeuveringPackage();
};

#endif

