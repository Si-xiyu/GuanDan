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
    void updateScores(int team1Score, int team2Score);
    void updateMultiplier(int multiplier);
    void updateCardCounts(const QMap<Card::CardPoint, int>& counts);
    void setCurrentPlayer(const QString& playerName);
    void updateTurnIndicator(int currentPlayerId);
    void updateTimerDisplay(int secondsRemaining, int totalSeconds);
    void updateTeamLevels(Card::CardPoint team1Level, Card::CardPoint team2Level);
    void updateRanking(const QStringList& rankedPlayerNames);
    void clearRanking();

private:
    void setupUI();

    CardCounterWidget* m_cardCounterWidget;
    LevelIndicatorWidget* m_levelIndicator;

    QGroupBox* m_gameStateBox;
    QLabel* m_team1ScoreLabel;
    QLabel* m_team2ScoreLabel;
    QLabel* m_multiplierLabel;
    QLabel* m_baseScoreLabel;

    QGroupBox* m_currentTurnBox;
    QLabel* m_currentPlayerNameLabel;
    QProgressBar* m_turnTimerBar;
    QLabel* m_timeRemainingLabel;
    
    QGroupBox* m_rankingBox;
    QVector<QLabel*> m_rankLabels;
}; 