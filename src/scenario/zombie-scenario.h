#ifndef ZOMBIE_MODE_H
#define ZOMBIE_MODE_H

#include "scenario.h"
#include "special3v3.h"
#include "maneuvering.h"

class ZombieScenario : public Scenario
{
    Q_OBJECT

public:
    explicit ZombieScenario();

    bool exposeRoles() const;
    void assign(QStringList &generals, QStringList &roles) const;
    int getPlayerCount() const;
    QString getRoles() const;
    void onTagSet(Room *room, const QString &key) const;
    bool generalSelection() const;
    AI::Relation relationTo(const ServerPlayer *a, const ServerPlayer *b) const;

private:
    QStringList females;
};

class GanranEquip : public IronChain
{
    Q_OBJECT

public:
    Q_INVOKABLE GanranEquip(Card::Suit suit, int number);
};

class PeachingCard : public QingnangCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PeachingCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};


#endif // ZOMBIE_MODE_H