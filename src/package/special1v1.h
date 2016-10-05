#ifndef _SPECIAL1V1_H
#define _SPECIAL1V1_H

#include "package.h"
#include "card.h"
#include "standard.h"

class XiechanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiechanCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class KOFCangjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KOFCangjiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class MouzhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MouzhuCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class KOFQiangxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KOFQiangxiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class KOFDrowning : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE KOFDrowning(Card::Suit suit, int number);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class Special1v1Package : public Package
{
    Q_OBJECT

public:
    Special1v1Package();
};

class Special1v1ExtPackage : public Package
{
    Q_OBJECT

public:
    Special1v1ExtPackage();
};

class New1v1CardPackage : public Package
{
    Q_OBJECT

public:
    New1v1CardPackage();
};

#endif
