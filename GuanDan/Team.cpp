#include "Team.h"
#include "Player.h"

#include <QDebug>  // 用于调试

Team::Team(int id)
    : m_id(id)
    , m_currentLevelRank(Card::Card_2)
    , m_score(0) // 初始化积分为0
{
}

void Team::addPlayer(Player* player)
{
    if (player && !m_players.contains(player)) {
        m_players.append(player);
        // 可以在这里调用Player的setTeam(Team*) 方法
        // player->setTeam(this);
    }
}

QVector<Player*> Team::getPlayers() const
{
    return m_players;
}

Card::CardPoint Team::getCurrentLevelRank() const
{
    return m_currentLevelRank;
}

void Team::setCurrentLevelRank(Card::CardPoint rank)
{
    // 具体升级逻辑由全局状态管理(LevelStatus类)
    m_currentLevelRank = rank;
    qDebug() << "Team" << m_id << "level rank set to:" << static_cast<int>(rank);
}

int Team::getId() const
{
    return m_id;
}

void Team::setScore(int score)
{
    m_score = score;
}

void Team::addScore(int amount)
{
    m_score += amount;
}

int Team::getScore() const
{
    return m_score;
}