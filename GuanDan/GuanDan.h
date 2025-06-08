#pragma once

#include <QtWidgets/QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include "ui_GuanDan.h"
#include "GD_Controller.h"
#include "PlayerWidget.h"
#include "Player.h"

class GuanDan : public QMainWindow
{
    Q_OBJECT

public:
    GuanDan(QWidget* parent = nullptr);
    ~GuanDan();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;

private slots:
    void startGame();                    // 开始游戏
    void onGameStarted();               // 游戏开始
    void onNewRoundStarted(int roundNumber); // 新一轮开始
    void onRoundOver(const QString& summary, const QVector<int>& playerRanks); // 一局结束
    void onGameOver(int winningTeamId, const QString& winningTeamName, const QString& finalMessage); // 游戏结束

private:
    void initializeUI();                // 初始化界面
    void setupConnections();            // 建立信号槽连接
    void createPlayers();               // 创建玩家实例
    void arrangePlayerWidgets();        // 布局玩家窗口
    void updateGameStatus();            // 更新游戏状态显示

    Ui::GuanDanClass ui;
    GD_Controller* m_gameController;    // 游戏控制器
    QVector<PlayerWidget*> m_playerWidgets; // 玩家界面
    QVector<Player*> m_players;         // 玩家对象
    QPushButton* m_startButton;         // 开始游戏按钮
    QPushButton* m_globalPlayButton;    // 全局出牌按钮
    QPushButton* m_globalSkipButton;    // 全局跳过按钮
    QWidget* m_centralWidget;           // 中央窗口部件
    QVBoxLayout* m_mainLayout;          // 主布局
    
    bool m_gameInProgress;              // 游戏进行状态
};

