#ifndef LEVELSTATUS_H
#define LEVELSTATUS_H

#include <array>

#include "Card.h"

class Team; // 前向声明 Team 类

class LevelStatus
{
public:
    LevelStatus();

    // 初始化新游戏的级别状态，并设置队伍的初始级别 (开局时调用)
    void initializeGameLevels(Team& team0, Team& team1);

    // 升级逻辑 (包含大局胜利判断)
    void updateLevelsAfterRound(int winningTeamId, int partnerRankOfWinner, Team& team0, Team& team1);
    // team0, team1: 两个队伍对象的引用，用于更新其内部的当前级牌

    // 获取指定队伍当前的级牌
    Card::CardPoint getTeamPlayingLevel(int teamId) const;

    // 检查整个游戏是否结束
    bool isGameOver() const;

    // 获取导致游戏结束的获胜队伍ID (如果游戏未结束，-1) 用于“战绩”模块
    int getGameWinnerTeamId() const;


    // 转换类型的辅助函数
    static int cardPointToLevelInt(Card::CardPoint point);
    static Card::CardPoint levelIntToCardPoint(int level);

private:
    std::array<Card::CardPoint, 2> m_teamPlayingLevels; // 储存两队级牌(在Team类中有实现，此处降低代码耦合度)
    std::array<int, 2> m_teamFailuresAtAce; // 存储两队打A失败的次数

    bool m_isGameOver; // 默认为false
    int m_gameWinnerTeamId; // -1 表示游戏未结束，0或1表示获胜队伍

};

#endif // LEVELSTATUS_H
