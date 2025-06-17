#pragma once

#include <QWidget>
#include <QMap>
#include <QVector>
#include "Card.h"
#include "LevelIndicatorWidget.h"

// 前向声明
class QLabel;
class QProgressBar;
class QGroupBox;
class CardCounterWidget;

class LeftWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LeftWidget(QWidget *parent = nullptr);

public slots:
	// 更新队伍分数显示
    void updateScores(int team1Score, int team2Score);
	// 更新本局倍率显示
    void updateMultiplier(int multiplier);
    // 更新记牌器显示
    void updateCardCounts(const QMap<Card::CardPoint, int>& counts);
    // 设置当前回合玩家的名称
    void setCurrentPlayer(const QString& playerName);
    // 根据当前玩家ID更新回合指示器的视觉效果
    void updateTurnIndicator(int currentPlayerId);
	// 更新剩余时间显示
    void updateTimerDisplay(int secondsRemaining, int totalSeconds);
    // 更新双方队伍的级牌显示
    void updateTeamLevels(Card::CardPoint team1Level, Card::CardPoint team2Level);
	// 在一局结束后，更新排行榜的显示
    void updateRanking(const QStringList& rankedPlayerNames);
    // 在新一局开始时，清空排行榜的显示
    void clearRanking();

private:
    void setupUI();

    // --- UI 子控件指针 ---
    CardCounterWidget* m_cardCounterWidget; // 记牌器控件
	LevelIndicatorWidget* m_levelIndicator; // 级牌展示区控件

    //QGroupBox 是 Qt 框架中的一个容器控件，用于将相关的 UI 元素组织到一个带标题边框的框架中

    QGroupBox* m_gameStateBox; // "积分状态" 的分组框
	QLabel* m_team1ScoreLabel; // 队伍1的分数标签(我方)
	QLabel* m_team2ScoreLabel; // 队伍2的分数标签(对方)
	QLabel* m_multiplierLabel; // 本局倍率标签
	QLabel* m_baseScoreLabel; // 本局基础分数标签

	QGroupBox* m_currentTurnBox; // "当前回合" 的分组框
	QLabel* m_currentPlayerNameLabel; // 当前回合玩家名称标签
	QProgressBar* m_turnTimerBar; // 回合计时进度条
	QLabel* m_timeRemainingLabel; // 剩余时间标签
    
	QGroupBox* m_rankingBox; // "排行榜" 的分组框
	QVector<QLabel*> m_rankLabels; // 用于显示排行榜的标签列表
}; 