#include "Team.h"
#include "Player.h"

#include <QDebug>  // 用于调试

Team::Team(int id)
    : m_id(id), m_currentLevelRank(Card::CardPoint::Card_2) // 初始级牌默认为 2
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

const QList<Player*>& Team::getPlayers() const
{
    return m_players;
}

Card::CardPoint Team::getCurrentLevelRank() const
{
    return m_currentLevelRank;
}

void Team::setCurrentLevelRank(Card::CardPoint rank)
{
    // 具体升级逻辑由全局状态管理(Status类)
    m_currentLevelRank = rank;
    qDebug() << "Team" << m_id << "level rank set to:" << static_cast<int>(rank);
}

int Team::getId() const
{
    return m_id;
}