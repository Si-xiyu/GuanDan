#ifndef TEAM_H
#define TEAM_H

#include <QList>
#include <QString>
#include <QVector>

#include "Card.h"
#include "Player.h"

class Team
{
public:

    Team(int id); //必须指定队伍Id
    ~Team();

    // 管理玩家
    void addPlayer(Player* player);
    QVector<Player*> getPlayers() const;

    // 管理队伍的级牌
    Card::CardPoint getCurrentLevelRank() const;
    void setCurrentLevelRank(Card::CardPoint rank); // 公开的设置级牌接口

    int getId() const;

    // 积分相关方法
    void setScore(int score);
    void addScore(int amount);
    int getScore() const;

private:
    int m_id;
    QVector<Player*> m_players;
    Card::CardPoint m_currentLevelRank;
    int m_score;
};

#endif // TEAM_H