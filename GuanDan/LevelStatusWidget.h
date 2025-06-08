#ifndef LEVELSTATUSWIDGET_H
#define LEVELSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include "Card.h"

class LevelStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LevelStatusWidget(QWidget* parent = nullptr);

    // 设置队伍名称
    void setTeamNames(const QString& team0Name, const QString& team1Name);

    // 重置级牌显示（游戏重置时使用）
    void resetLevels();

public slots:
    // 更新队伍级牌
    void updateTeamLevel(int teamId, Card::CardPoint newLevel);

private:
    // UI组件
    QLabel* m_team0NameLabel;
    QLabel* m_team0LevelLabel;
    QLabel* m_team1NameLabel;
    QLabel* m_team1LevelLabel;

    // 布局
    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_team0Layout;
    QVBoxLayout* m_team1Layout;

    // 动画效果
    QGraphicsOpacityEffect* m_team0Effect;
    QGraphicsOpacityEffect* m_team1Effect;
    QPropertyAnimation* m_team0Animation;
    QPropertyAnimation* m_team1Animation;
    QTimer* m_animationTimer;

    // 私有方法
    void setupUI();
    QWidget* createSeparator();
    void setupAnimations();
    QString levelToString(Card::CardPoint level) const;
    void playLevelChangeAnimation(int teamId);
    void updateTeamDisplay(int teamId, Card::CardPoint level);
};

#endif // LEVELSTATUSWIDGET_H