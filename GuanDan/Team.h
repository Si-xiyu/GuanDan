#ifndef TEAM_H
#define TEAM_H

#include <QList>
#include <QString>

#include "Card.h"
#include "Player.h"

class Team
{
public:

    Team(int& id); //必须指定队伍Id

    // 管理玩家
    void addPlayer(Player* player);
    const QList<Player*>& getPlayers() const;

    // 管理队伍的级牌
    Card::CardPoint getCurrentLevelRank() const;
    void setCurrentLevelRank(Card::CardPoint rank); // 公开的设置级牌接口

    int getId() const;

private:
    int m_id;
    QList<Player*> m_players;
    Card::CardPoint m_currentLevelRank;
};

#endif // TEAM_H