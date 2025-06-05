#include "Levelstatus.h"
#include "Team.h"
#include <QDebug>

LevelStatus::LevelStatus() : m_isGameOver(false), m_gameWinnerTeamId(-1) {
    // 实际初始化在 initializeGameLevels 中进行
    m_teamPlayingLevels[0] = Card::CardPoint::Card_2; // 默认值以防万一
    m_teamPlayingLevels[1] = Card::CardPoint::Card_2;
    m_teamFailuresAtAce[0] = 0;
    m_teamFailuresAtAce[1] = 0;
}

void LevelStatus::initializeGameLevels(Team& team0, Team& team1)
{
    m_teamPlayingLevels[0] = Card::CardPoint::Card_2;
    m_teamPlayingLevels[1] = Card::CardPoint::Card_2;
    m_teamFailuresAtAce[0] = 0;
    m_teamFailuresAtAce[1] = 0;
    m_isGameOver = false;
    m_gameWinnerTeamId = -1;

    // 将初始级别设置到 Team 对象中
    team0.setCurrentLevelRank(m_teamPlayingLevels[0]);
    team1.setCurrentLevelRank(m_teamPlayingLevels[1]);

    qDebug() << "游戏级别已初始化：队伍0当前级别为2, 队伍1当前级别为2。";
}

void LevelStatus::updateLevelsAfterRound(int winningTeamId, int partnerRankOfWinner, Team& team0, Team& team1)
{
    if (m_isGameOver) {
        qDebug() << "游戏已经结束 (获胜队伍:" << m_gameWinnerTeamId << ")，不再更新级别。";
        return; // 游戏已结束
    }

    int losingTeamId = 1 - winningTeamId;
    // 规则: "一二位升三级，一三位升两级，一四位升一级。"
    // partnerRankOfWinner: 0=第一, 1=第二, 2=第三, 3=第四
    int levelIncrement = 0;
    if (partnerRankOfWinner == 1) { // 头游的对家是第二名
        levelIncrement = 3;
    }
    else if (partnerRankOfWinner == 2) { // 头游的对家是第三名
        levelIncrement = 2;
    }
    else if (partnerRankOfWinner == 3) { // 头游的对家是第四名
        levelIncrement = 1;
    }
    else if (partnerRankOfWinner == 0) {
        // 这种情况理论上不应该发生，因为partnerRankOfWinner是指“头游玩家的对家”的排名。
        qWarning() << "LevelStatus::updateLevelsAfterRound: partnerRankOfWinner 为 0 (头游的对家也是头游?)，升级逻辑可能需要复查。默认为不升级。";
        levelIncrement = 0;
    }
    // 如果 partnerRankOfWinner 不是 1, 2, 3 (例如，对家是0，或者队伍未获胜导致没有有效对家排名)，则不升级。

    qDebug() << "队伍" << winningTeamId << "本轮获胜。头游对家排名:" << partnerRankOfWinner + 1 << "。计划升级:" << levelIncrement << "级。";

    // 1. 处理获胜队伍的级别
    if (levelIncrement > 0) {
        Card::CardPoint currentWinnerLevelPoint = m_teamPlayingLevels[winningTeamId];
        int currentWinnerLevelInt = cardPointToLevelInt(currentWinnerLevelPoint);
        int newWinnerLevelInt = currentWinnerLevelInt + levelIncrement;

        // 检查是否过A级获胜，从而结束游戏
        if (currentWinnerLevelPoint == Card::CardPoint::Card_A && levelIncrement > 1) {
            m_isGameOver = true;
            m_gameWinnerTeamId = winningTeamId;
            qDebug() << "队伍" << winningTeamId << "过A成功！游戏结束。";
            // 级别不再改变，游戏结束
            team0.setCurrentLevelRank(m_teamPlayingLevels[0]); // 确保Team对象状态一致
            team1.setCurrentLevelRank(m_teamPlayingLevels[1]);
            return;
        }

        // 是否直接超过A级获胜
        else if (newWinnerLevelInt > 13) { //A对应的序数为13
            m_isGameOver = true;
            m_gameWinnerTeamId = winningTeamId;
            qDebug() << "队伍" << winningTeamId << "获胜！游戏结束。";
            // 级别不再改变，游戏结束
            team0.setCurrentLevelRank(m_teamPlayingLevels[0]); // 确保Team对象状态一致
            team1.setCurrentLevelRank(m_teamPlayingLevels[1]);
            return;
        }

        // 正常升级
        else if (newWinnerLevelInt <= 13) {
            m_teamPlayingLevels[winningTeamId] = levelIntToCardPoint(newWinnerLevelInt);
            qDebug() << "队伍" << winningTeamId << "升级到" << cardPointToLevelInt(m_teamPlayingLevels[winningTeamId])
                << " (" << m_teamPlayingLevels[winningTeamId] << ")";
        }
    }
    else {
        qDebug() << "队伍" << winningTeamId << "未成功升级";
    }

    // 2. 处理失败队伍的级别 (打A失败的情况)
    Card::CardPoint currentLoserLevelPoint = m_teamPlayingLevels[losingTeamId];
    if (currentLoserLevelPoint == Card::CardPoint::Card_A) { // 如果输的队伍当前是A级 (即打A失败)
        m_teamFailuresAtAce[losingTeamId]++;
        qDebug() << "队伍" << losingTeamId << "打A失败，累计失败次数:" << m_teamFailuresAtAce[losingTeamId];
        if (m_teamFailuresAtAce[losingTeamId] >= 3) { // 三次打A失败则退回2级
            m_teamPlayingLevels[losingTeamId] = Card::CardPoint::Card_2;
            m_teamFailuresAtAce[losingTeamId] = 0; // 重置失败次数
            qDebug() << "队伍" << losingTeamId << "打A连续三次失败，降回2级。";
        }
        // 如果不是三次失败，级别保持不变 (还是A，下一轮继续尝试)
    }

    // 将更新后的级别设置回 Team 对象
    team0.setCurrentLevelRank(m_teamPlayingLevels[0]);
    team1.setCurrentLevelRank(m_teamPlayingLevels[1]);

    qDebug() << "级别更新完毕：队伍0当前级别:" << cardPointToLevelInt(m_teamPlayingLevels[0])
        << "，队伍1当前级别:" << cardPointToLevelInt(m_teamPlayingLevels[1]);
}

Card::CardPoint LevelStatus::getTeamPlayingLevel(int teamId) const
{
    if (teamId == 0 || teamId == 1) {
        return m_teamPlayingLevels[teamId];
    }
    qWarning() << "LevelStatus::getTeamPlayingLevel: 无效的队伍ID:" << teamId;
    return Card::CardPoint::Card_2; // 返回一个默认值
}

bool LevelStatus::isGameOver() const
{
    return m_isGameOver;
}

int LevelStatus::getGameWinnerTeamId() const
{
    return m_gameWinnerTeamId;
}

int LevelStatus::cardPointToLevelInt(Card::CardPoint point)
{
    switch (point) {
    case Card::CardPoint::Card_2:  return 1;
    case Card::CardPoint::Card_3:  return 2;
    case Card::CardPoint::Card_4:  return 3;
    case Card::CardPoint::Card_5:  return 4;
    case Card::CardPoint::Card_6:  return 5;
    case Card::CardPoint::Card_7:  return 6;
    case Card::CardPoint::Card_8:  return 7;
    case Card::CardPoint::Card_9:  return 8;
    case Card::CardPoint::Card_10: return 9;
    case Card::CardPoint::Card_J:  return 10;
    case Card::CardPoint::Card_Q:  return 11;
    case Card::CardPoint::Card_K:  return 12;
    case Card::CardPoint::Card_A:  return 13;
    default: // 一般不会调用
        qWarning() << "LevelStatus::cardPointToLevelInt: Invalid CardPoint for level:" << static_cast<int>(point);
        return 2; // Default to 2
    }
}

Card::CardPoint LevelStatus::levelIntToCardPoint(int level)
{
    switch (level) {
    case 1:  return Card::CardPoint::Card_2;
    case 2:  return Card::CardPoint::Card_3;
    case 3:  return Card::CardPoint::Card_4;
    case 4:  return Card::CardPoint::Card_5;
    case 5:  return Card::CardPoint::Card_6;
    case 6:  return Card::CardPoint::Card_7;
    case 7:  return Card::CardPoint::Card_8;
    case 8:  return Card::CardPoint::Card_9;
    case 9:  return Card::CardPoint::Card_10;
    case 10: return Card::CardPoint::Card_J;
    case 11: return Card::CardPoint::Card_Q;
    case 12: return Card::CardPoint::Card_K;
    case 13: return Card::CardPoint::Card_A;
    default: // 同上，一般不会调用
        qWarning() << "LevelStatus::levelIntToCardPoint: Invalid integer for level:" << level;
        return Card::CardPoint::Card_2; // Default to 2
    }
}

