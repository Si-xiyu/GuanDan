#pragma once

// GuanDan主窗口类，负责游戏的界面显示逻辑和用户交互
#include <QtWidgets/QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include "ui_GuanDan.h"
#include "GD_Controller.h"
#include "PlayerAreaWidget.h"
#include "Player.h"
#include "LeftWidget.h"

class GuanDan : public QMainWindow
{
    Q_OBJECT

public:
    GuanDan(QWidget* parent = nullptr);
    ~GuanDan();

protected:
	// ！重写resizeEvent以适应窗口大小变化，由于表现不好，暂时不使用
    virtual void resizeEvent(QResizeEvent* event) override;

private slots:
	// 初始化游戏，创建玩家，设置界面
    void startGame();
	// 更新游戏状态后，更新显示
    void onGameStarted(); 
    // 新一轮开始，更新界面显示
    void onNewRoundStarted(int roundNumber);
	// 一局结束，更新界面，显示本局结果
    void onRoundOver(const QString& summary, const QVector<int>& playerRanks);
    // 游戏结束，处理视图逻辑，显示最终结果
    void onGameOver(int winningTeamId, const QString& winningTeamName, const QString& finalMessage); 
    // 处理进贡/还贡对话框
	void onAskForTribute(int fromPlayerId, const QString& fromPlayerName, int toPlayerId, const QString& toPlayerName, bool isReturn);
    // 弹出设置窗口
	void showSettingsDialog();
    // 提示功能 
	void onShowHint(int playerId, const QVector<Card>& suggestedCards); 

private:
    void initializeUI();                // 初始化界面
    void setupConnections();            // 建立信号槽连接
    void createPlayers();               // 创建玩家实例
    void arrangePlayerWidgets();        // 布局玩家窗口
    void updateGameStatus();            // 更新游戏状态显示

    Ui::GuanDanClass ui;
    GD_Controller* m_gameController;    // 游戏控制器
    QVector<PlayerAreaWidget*> m_playerWidgets; // 玩家界面
    QVector<Player*> m_players;         // 玩家对象
    QPushButton* m_startButton;         // 开始游戏按钮
    QPushButton* m_globalPlayButton;    // 全局出牌按钮
    QPushButton* m_globalSkipButton;    // 全局跳过按钮
    QPushButton* m_settingsButton;      // 设置按钮
    QPushButton* m_hintButton;          // 提示按钮
    QWidget* m_centralWidget;           // 中央窗口部件
    QVBoxLayout* m_mainLayout;          // 主布局
    
	LeftWidget* m_leftWidget;           // 左侧信息显示部件
    
    bool m_gameInProgress;              // 游戏进行状态
};