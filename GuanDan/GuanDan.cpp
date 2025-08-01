#include "GuanDan.h" 
#include "GD_Controller.h" 
#include "Team.h"         
#include "NPCPlayer.h"
#include "TributeDialog.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>
#include <QTimer>

#include "SettingsDialog.h"
#include "SoundManager.h"

GuanDan::GuanDan(QWidget* parent)
    : QMainWindow(parent)
    , m_gameController(nullptr)
    , m_startButton(nullptr)
    , m_globalPlayButton(nullptr)
    , m_globalSkipButton(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_gameInProgress(false)
    , m_settingsButton(nullptr)
    , m_hintButton(nullptr)
    , m_leftWidget(nullptr)
{
    ui.setupUi(this);
    initializeUI();
    createPlayers();
    setupConnections();
}

GuanDan::~GuanDan()
{
    delete m_gameController;
    qDeleteAll(m_playerWidgets);
    qDeleteAll(m_players);
}

void GuanDan::initializeUI()
{
    // 设置窗口标题和固定大小，使用更合适的尺寸
    setWindowTitle(tr("GuanDan -By Si-xiyu"));
    setFixedSize(1640, 1080); // 将宽度从1440增加到1640，为记牌器腾出空间(LeftWidget和PlayerAreaWidget)

    // 创建中央窗口部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建主布局
	m_mainLayout = new QVBoxLayout(m_centralWidget); // 中心部件使用垂直布局
    m_mainLayout->setContentsMargins(0, 0, 0, 0); // 设置无边距
	m_mainLayout->setSpacing(0); // 设置无间距

    // 创建游戏区域
    QWidget* gameArea = new QWidget(this);
    gameArea->setStyleSheet("QWidget { background-color: #1B5E20; }");
	m_mainLayout->addWidget(gameArea, 1); // 加入游戏区域 

    // 创建底部控制区域
    QWidget* controlArea = new QWidget(this);
    controlArea->setFixedHeight(80); // 减小控制区域高度
    controlArea->setStyleSheet("QWidget { background-color: #0D2E10; }");
    
    // 创建开始游戏按钮
    m_startButton = new QPushButton(tr("开始游戏"), controlArea);
    m_startButton->setFixedSize(200, 50);
    m_startButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 25px;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );

    // 设置按钮初始化
    m_settingsButton = new QPushButton(this);

    // 将设置按钮修改为图标
    m_settingsButton->setIcon(QIcon(":/icon/res/Setting_icon.jpg"));
    m_settingsButton->setIconSize(QSize(32, 32));
    m_settingsButton->setFixedSize(40, 40);
    m_settingsButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   border-radius: 20px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #40FFFFFF;" // 半透明白色
        "}"
        "QPushButton:pressed {"
        "   background-color: #80FFFFFF;" // 更不透明的白色
        "}"
    );

    // 全局出牌/跳过按钮，初始隐藏
    m_globalPlayButton = new QPushButton(tr("出牌"), controlArea);
    m_globalPlayButton->setFixedSize(100, 40);
    m_globalPlayButton->hide();
    m_globalPlayButton->setStyleSheet(
        "QPushButton { background-color: #87CEEB; color: white; border-radius: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #7AC5CD; }"
        "QPushButton:pressed { background-color: #6AADB4; }"
    );

    m_globalSkipButton = new QPushButton(tr("跳过"), controlArea);
    m_globalSkipButton->setFixedSize(100, 40);
    m_globalSkipButton->hide();
    m_globalSkipButton->setStyleSheet(
        "QPushButton { background-color: #87CEEB; color: white; border-radius: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #7AC5CD; }"
        "QPushButton:pressed { background-color: #6AADB4; }"
    );

    // 创建提示按钮
    m_hintButton = new QPushButton(tr("提示"), controlArea);
    m_hintButton->setFixedSize(80, 40);
    m_hintButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FFE4B5;"  // 淡黄色
        "   color:rgb(30, 28, 26);"            
        "   border: none;"
        "   border-radius: 20px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #FFD700;"  // 鼠标悬停时变为金黄色
        "}"
        "QPushButton:pressed {"
        "   background-color: #DAA520;"  // 点击时变为深金色
        "}"
    );
    m_hintButton->hide(); // 初始时隐藏

    // 添加提示按钮
    QHBoxLayout* controlLayout = new QHBoxLayout(controlArea);
    controlLayout->addWidget(m_settingsButton); // 设置按钮居左
    controlLayout->addStretch(); // 添加伸缩项
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_globalPlayButton);
    controlLayout->addWidget(m_globalSkipButton);
    controlLayout->addStretch(); // 添加伸缩项，使中间的按钮组居中
    controlLayout->addWidget(m_hintButton); // 提示按钮固定在最右边
    controlLayout->setContentsMargins(10, 10, 10, 10);

    // 将控制区域添加到主布局
    m_mainLayout->addWidget(controlArea);

    // 创建新的LeftWidget
    m_leftWidget = new LeftWidget(gameArea);
    m_leftWidget->hide(); // 初始时隐藏

    // 确保初始布局正确
    QTimer::singleShot(0, this, &GuanDan::arrangePlayerWidgets);
}

void GuanDan::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    arrangePlayerWidgets();
}

void GuanDan::createPlayers()
{
    qDebug() << "开始创建玩家...";
    // 创建游戏控制器
    m_gameController = new GD_Controller(this);

    // 获取游戏区域
    QWidget* gameArea = m_centralWidget->findChild<QWidget*>();
    if (!gameArea) {
        qWarning() << "无法找到游戏区域！";
        return;
    }

    // 创建四个玩家界面
    for (int i = 0; i < 4; ++i) {
        // 创建玩家对象，底部玩家为人类，其他玩家为 AI
        Player* player = nullptr;
        if (i == 0) {
            player = new Player(QString("玩家%1").arg(i), i);
            player->setType(Player::Human);
        }
        else {
            player = new NPCPlayer(QString("AI%1").arg(i), i);
            player->setType(Player::AI);
        }
        m_players.append(player);
        qDebug() << "创建玩家:" << player->getName() << "ID:" << player->getID();

        // 创建玩家界面，设置位置和是否为当前玩家
        PlayerPosition position;

        // 【关键修复点】 交换 Left 和 Right 的位置
        switch (i) {
        case 0:
            position = PlayerPosition::Bottom;
            break;
        case 1: // ID 1 是下家，应该在右边
            position = PlayerPosition::Right; // 原来是 Left
            break;
        case 2:
            position = PlayerPosition::Top;
            break;
        case 3: // ID 3 是上家，应该在左边
            position = PlayerPosition::Left; // 原来是 Right
            break;
        }

        // 创建玩家界面（使用PlayerAreaWidget）
        PlayerAreaWidget* playerAreaWidget = new PlayerAreaWidget(player, position, i == 0, gameArea);
        m_playerWidgets.append(playerAreaWidget);
        playerAreaWidget->show();
        qDebug() << "创建玩家界面:" << player->getName() << "位置:" << static_cast<int>(position);
    }

    // 初始布局玩家界面
    arrangePlayerWidgets();
}

void GuanDan::setupConnections()
{
    // 连接开始游戏按钮
    connect(m_startButton, &QPushButton::clicked, this, &GuanDan::startGame);

    // 连接设置按钮
    connect(m_settingsButton, &QPushButton::clicked, this, &GuanDan::showSettingsDialog);

    // 连接全局出牌/跳过按钮
    // 将出牌按钮与被选中的牌连接
    connect(m_globalPlayButton, &QPushButton::clicked, this, [this]() {
        // 从底部玩家获取选中牌(人类玩家选牌信号)
        QVector<Card> cards = m_playerWidgets[0]->getSelectedCards();
        if (!cards.isEmpty()) {
			m_gameController->onPlayerPlay(0, cards); // 将选中牌传给onPlayerPlay方法
        }
    });
	// 连接跳过按钮
    connect(m_globalSkipButton, &QPushButton::clicked, this, [this]() {
        m_gameController->onPlayerPass(0);
    });

    // 连接提示按钮点击
    connect(m_hintButton, &QPushButton::clicked, this, [this]() {
        m_gameController->onPlayerRequestHint(0);
    });

    // 连接消息显示信号
    connect(m_gameController, &GD_Controller::sigShowPlayerMessage,
        this, [this](int playerId, const QString& message, bool isError) {
            // 只对人类玩家（ID为0）显示消息框
            if (playerId == 0) {
                QMessageBox msgBox(this);
                msgBox.setText(message);
                msgBox.setWindowTitle(isError ? "错误" : "提示");
                msgBox.setIcon(isError ? QMessageBox::Warning : QMessageBox::Information);
                msgBox.exec();
            }
        });

    // 根据控制器启用信号控制全局按钮显示
    connect(m_gameController, &GD_Controller::sigEnablePlayerControls,
        this, [this](int playerId, bool canPlay, bool canPass) {
            if (playerId == 0) {
				m_globalPlayButton->setVisible(canPlay); // 仅当人类玩家可以出牌时显示按钮
				m_globalPlayButton->setEnabled(canPlay); // 启用按钮
				m_globalSkipButton->setVisible(canPass); // 仅当人类玩家可以跳过时显示按钮
				m_globalSkipButton->setEnabled(canPass); // 启用按钮
                m_hintButton->setVisible(canPlay); // 当可以出牌时显示提示按钮
                m_hintButton->setEnabled(canPlay);
            } else {
                m_globalPlayButton->hide();
                m_globalSkipButton->hide();
                m_hintButton->hide();
            }
        });

	// 连接游戏控制器信号&视图更新信号
    // 游戏开始
    connect(m_gameController, &GD_Controller::sigGameStarted,
        this, &GuanDan::onGameStarted);
	// 新一轮开始
    connect(m_gameController, &GD_Controller::sigNewRoundStarted,
        this, &GuanDan::onNewRoundStarted);
	// Round结束
    connect(m_gameController, &GD_Controller::sigRoundOver,
        this, &GuanDan::onRoundOver);
	// 连接游戏结束信号
    connect(m_gameController, &GD_Controller::sigGameOver,
        this, &GuanDan::onGameOver);
        
    // 连接更新桌面牌型信号
    connect(m_gameController, &GD_Controller::sigUpdateTableCards,
		// [this]表示捕获当前类的this指针
        this, [this](int playerId, const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards) {
            // 更新所有玩家的出牌显示区域
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->updatePlayedCards(combo, originalCards); // 出牌区刷新
                }
            }
        });

	// 连接玩家已过牌信号
    connect(m_gameController, &GD_Controller::sigPlayerPassed,
        this, [this](int playerId) {
            // 清空过牌玩家的出牌区
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->clearPlayedCards();
                }
            }
        });

	// 连接清空桌面牌信号
    connect(m_gameController, &GD_Controller::sigClearTableCards,
        this, [this]() {
            // 清空所有玩家的出牌显示
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                widget->clearPlayedCards();
            }
        });

	// 连接已发牌信号
    connect(m_gameController, &GD_Controller::sigCardsDealt,
        this, [this](int playerId, const QVector<Card>& cards) {
            qDebug() << "收到发牌信号 - 玩家ID:" << playerId << "牌数:" << cards.size();
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
					// 更新手牌区显示，底部玩家显示正面，其他玩家显示背面
                    widget->updateHandDisplay(cards, widget->getPlayer()->getID() == 0);
                    break;
                }
            }
        });

    // 当玩家出牌或过牌后更新手牌显示
    connect(m_gameController, &GD_Controller::sigUpdatePlayerHand,
        this, [this](int playerId, const QVector<Card>& cards) {
            qDebug() << "收到玩家手牌更新信号(无动画) - 玩家ID:" << playerId << "牌数:" << cards.size();
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->updateHandDisplayNoAnimation(cards, widget->getPlayer()->getID() == 0);
                    // 明确命令刷新玩家信息（如剩余牌数）
                    widget->getPlayerWidget()->updatePlayerInfo();
                    break;
                }
            }
        });

    // 连接每个玩家界面的信号
    for (PlayerAreaWidget* widget : m_playerWidgets) {
		// 为游戏中的每一个玩家区域（PlayerAreaWidget）建立一个响应机制，输出选中卡牌数量
        connect(widget, &PlayerAreaWidget::cardsSelected,
            [this, widget](const QVector<Card>& cards) {
                qDebug() << "收到卡牌选择信号 - 玩家:" << widget->getPlayer()->getName()
                         << "选中卡牌数量:" << cards.size();
            });
        
        // 当玩家出牌卡牌时
        connect(widget, &PlayerAreaWidget::playCardsRequested,
            [this, widget]() {
                if (widget->getPlayer()) {
                    QVector<Card> selectedCards = widget->getSelectedCards();
                    if (!selectedCards.isEmpty()) {
						// 如果选中卡牌不为空，调用游戏控制器的出牌方法onPlayerPlay
                        m_gameController->onPlayerPlay(widget->getPlayer()->getID(), selectedCards);
                    }
                }
            });
            
        // 当玩家点击跳过按钮时通知游戏控制器
        connect(widget, &PlayerAreaWidget::skipTurnRequested,
            [this, widget]() {
                if (widget->getPlayer()) {
					// 调用游戏控制器的跳过方法onPlayerPass
                    m_gameController->onPlayerPass(widget->getPlayer()->getID());
                }
            });
    }

    // 连接游戏控制器的玩家控制信号
    connect(m_gameController, &GD_Controller::sigEnablePlayerControls,
        this, [this](int playerId, bool canPlay, bool canPass) {
            qDebug() << "收到启用玩家控制信号 - 玩家ID:" << playerId << "可出牌:" << canPlay << "可跳过:" << canPass;
            
            bool found = false;
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    found = true;
                    widget->setEnabled(canPlay);
                    qDebug() << "已更新玩家UI状态:" << widget->getPlayer()->getName();
                }
            }
            
            if (!found) {
                qWarning() << "警告: 未找到ID为" << playerId << "的玩家控件";
            }
        });

    // 连接当前玩家回合信号
    connect(m_gameController, &GD_Controller::sigSetCurrentTurnPlayer,
        this, [this](int playerId, const QString& playerName) {
			// 调用GuanDan类的方法来更新左侧信息面板
            m_leftWidget->setCurrentPlayer(playerName);
            m_leftWidget->updateTurnIndicator(playerId);
        });

    // 连接进贡/还贡请求，弹出TributeDialog
    connect(m_gameController, &GD_Controller::sigAskForTribute,
        this, &GuanDan::onAskForTribute);
        
    // 连接提示按钮
    connect(m_gameController, &GD_Controller::sigShowHint, this, &GuanDan::onShowHint);

    // 添加新的连接
    connect(m_gameController, &GD_Controller::sigCardCountsUpdated,
        m_leftWidget, &LeftWidget::updateCardCounts);
    
    connect(m_gameController, &GD_Controller::sigScoresUpdated,
        m_leftWidget, &LeftWidget::updateScores);
    
    connect(m_gameController, &GD_Controller::sigMultiplierUpdated,
        m_leftWidget, &LeftWidget::updateMultiplier);
        
    // 添加计时器相关信号连接
    connect(m_gameController, &GD_Controller::sigTurnTimerTick,
        m_leftWidget, &LeftWidget::updateTimerDisplay);
        
    connect(m_gameController, &GD_Controller::sigSetCurrentTurnPlayer,
        m_leftWidget, &LeftWidget::updateTurnIndicator);
        
    // 连接级牌更新信号
    connect(m_gameController, &GD_Controller::sigTeamLevelsUpdated,
        m_leftWidget, &LeftWidget::updateTeamLevels);
    
    // 1. 当一局结束时更新排行榜
    // 使用lambda函数将玩家ID转换为玩家名字
    connect(m_gameController, &GD_Controller::sigRoundOver, this,
        [this](const QString& summary, const QVector<int>& playerRanks) {
            QStringList rankedNames;
            for (int playerId : playerRanks) {
                // 查找对应ID的玩家对象
                Player* foundPlayer = nullptr;
                for (Player* p : m_players) {
                    if (p->getID() == playerId) {
                        foundPlayer = p;
                        break;
                    }
                }
                
                if (foundPlayer) {
                    rankedNames.append(foundPlayer->getName());
                } else {
                    rankedNames.append(tr("未知玩家")); // 后备显示文本
                }
            }
            // 将玩家名字列表传递给LeftWidget
            m_leftWidget->updateRanking(rankedNames);
        });

    // 2. 当新的一局开始时清空排行榜
    connect(m_gameController, &GD_Controller::sigNewRoundStarted,
        m_leftWidget, &LeftWidget::clearRanking);
}

// 布局函数
void GuanDan::arrangePlayerWidgets()
{
    QWidget* gameArea = m_centralWidget->findChild<QWidget*>();
    if (!gameArea) return;

    QSize gameSize = gameArea->size();

    const int LAYOUT_OFFSET_X = 200;

    // 为1640x1080窗口重新设计的布局参数
    int horizontalWidth = 900;   // 上下玩家区域的宽度
    int bottomHeight = 460;      // 底部玩家区域的高度
    int topHeight = 340;         // 顶部玩家区域的高度
    int verticalWidth = 500;     // 左右玩家区域的宽度
    int verticalHeight = 620;    // 左右玩家区域高度

    // 将侧边区域放置在顶部和底部区域之间的中心
    int topAreaBottomEdge = 20 + topHeight; // 顶部区域下边缘 Y 坐标 (20是顶部边距)
    int bottomAreaTopEdge = gameSize.height() - bottomHeight; // 底部区域上边缘 Y 坐标
    int centralFreeSpace = bottomAreaTopEdge - topAreaBottomEdge; // 中间可用空间高度
    int verticalY = topAreaBottomEdge + (centralFreeSpace - verticalHeight) / 2; // 计算侧边区域的起始Y坐标使其居中

    for (PlayerAreaWidget* widget : m_playerWidgets)
    {
        if (!widget) continue;

        switch (widget->getPosition())
        {
        case PlayerPosition::Bottom:
            widget->setGeometry(
                LAYOUT_OFFSET_X + (gameSize.width() - LAYOUT_OFFSET_X - horizontalWidth) / 2,
                gameSize.height() - bottomHeight,
                horizontalWidth,
                bottomHeight
            );
            break;

        case PlayerPosition::Left:
            widget->setGeometry(
                LAYOUT_OFFSET_X + 20,  // 左侧留出边距
                verticalY,  // 垂直居中
                verticalWidth,
                verticalHeight
            );
            break;

        case PlayerPosition::Top:
            widget->setGeometry(
                LAYOUT_OFFSET_X + (gameSize.width() - LAYOUT_OFFSET_X - horizontalWidth) / 2,
                20,
                horizontalWidth,
                topHeight
            );
            break;

        case PlayerPosition::Right:
            widget->setGeometry(
                // 修正：去掉 LAYOUT_OFFSET_X，直接从窗口右边计算位置
                gameSize.width() - verticalWidth - 20,
                verticalY,
                verticalWidth,
                verticalHeight
            );
            break;
        }
        widget->show();
    }

    // 布局LeftWidget，放在左侧，垂直居中
    if (m_leftWidget) {
        int y_pos = (gameSize.height() - 600) / 2; // 假设LeftWidget高度600
        m_leftWidget->setGeometry(20, y_pos, 180, 600);
    }
}

// 调用设置窗口函数
void GuanDan::showSettingsDialog()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

// 处理提示信号，选中玩家的可出牌型
void GuanDan::onShowHint(int playerId, const QVector<Card>& suggestedCards)
{
    qDebug() << "GuanDan: 接收到提示信号，准备选中玩家" << playerId << "的" << suggestedCards.size() << "张牌";

    // 提示功能只对人类玩家（ID为0）有效
    if (playerId != 0) {
        qWarning() << "GuanDan::onShowHint: 接收到非人类玩家的提示信号，已忽略。PlayerID:" << playerId;
        return;
    }

    // 遍历所有玩家界面，找到对应ID的那个玩家
    bool playerWidgetFound = false;
    for (PlayerAreaWidget* widget : m_playerWidgets) {
        if (widget && widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
            // 命令这个玩家的PlayerAreaWidget选中推荐的牌
            widget->selectCards(suggestedCards);
            playerWidgetFound = true;
            break; 
        }
    }

    if (!playerWidgetFound) {
        qWarning() << "GuanDan::onShowHint: 未能找到ID为" << playerId << "的玩家界面控件。";
    }
}

void GuanDan::startGame()
{
    if (!m_gameInProgress) {
        qDebug() << "开始游戏...";
        m_gameInProgress = true;
        m_startButton->setEnabled(false);
        m_startButton->hide(); // 隐藏开始按钮
        
        // 播放BGM
        SoundManager::instance().playBGM();
        
        // 设置玩家和队伍
        QVector<Team*> teams;
        // 创建两个队伍
        Team* team1 = new Team(0);
        Team* team2 = new Team(1);

        // 分配玩家到队伍
        team1->addPlayer(m_players[0]);
        team1->addPlayer(m_players[2]);
        team2->addPlayer(m_players[1]);
        team2->addPlayer(m_players[3]);

        // 设置玩家的队伍
        m_players[0]->setTeam(team1);
        m_players[2]->setTeam(team1);
        m_players[1]->setTeam(team2);
        m_players[3]->setTeam(team2);

        teams.append(team1);
        teams.append(team2);

        qDebug() << "队伍1玩家:" << team1->getPlayers()[0]->getName() << "," << team1->getPlayers()[1]->getName();
        qDebug() << "队伍2玩家:" << team2->getPlayers()[0]->getName() << "," << team2->getPlayers()[1]->getName();

        // 初始化游戏
        m_gameController->setupNewGame(m_players, teams);
        m_gameController->startGame();

        // 更新所有玩家界面
        for (PlayerAreaWidget* widget : m_playerWidgets) {
            if (widget && widget->getPlayer()) {
                widget->updatePlayerInfo();
            }
        }
    }
}

void GuanDan::onGameStarted()
{
    qDebug() << "GuanDan::onGameStarted - 游戏开始";
	// 更新游戏状态显示
    updateGameStatus();
    
    // 显示左侧信息面板
    if (m_leftWidget) {
        m_leftWidget->show();
    }
    
    // 调试代码，检查所有玩家控件的状态
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "检查玩家控件状态:";
        for (int i = 0; i < m_playerWidgets.size(); ++i) {
            PlayerAreaWidget* widget = m_playerWidgets[i];
            if (widget && widget->getPlayer()) {
                qDebug() << "玩家" << i << ":" << widget->getPlayer()->getName()
                         << "位置:" << static_cast<int>(widget->getPosition())
                         << "是否启用:" << widget->isEnabled();
            }
        }
    });
}

void GuanDan::onNewRoundStarted(int roundNumber)
{
    // 新一轮开始时，清空所有玩家的出牌区域
    for (PlayerAreaWidget* widget : m_playerWidgets) {
        if (widget) {
            widget->clearPlayedCards();
        }
    }

    // 更新界面显示（主要是手牌）
    updateGameStatus();
	qDebug() << "GuanDan::onNewRoundStarted：新一轮QMessageBox被调用,第" << roundNumber << "轮开始";
    QMessageBox::information(this, tr("新一轮"),
        tr("第 %1 轮开始").arg(roundNumber));
}

void GuanDan::onRoundOver(const QString& summary, const QVector<int>& playerRanks)
{
    // 更新界面显示
    updateGameStatus();
    
    // 显示本局结果
    qDebug() << "GuanDan::onRoundOver：本局结束QMessageBox被调用";
    QMessageBox::information(this, tr("本局结束"), summary);
}

void GuanDan::onGameOver(int winningTeamId, const QString& winningTeamName, const QString& finalMessage)
{
    m_gameInProgress = false;
    m_startButton->setEnabled(true);
    m_startButton->show(); // 重新显示开始按钮
    
    // 停止BGM
    SoundManager::instance().stopBGM();
    
    QString title;
    QString message;

    // 确保玩家和队伍信息是有效的
    if (m_players.isEmpty() || !m_players[0] || !m_players[0]->getTeam())
    {
        // 备用逻辑，如果玩家信息丢失
        title = tr("游戏结束");
        message = tr("获胜队伍: %1\n%2").arg(winningTeamName).arg(finalMessage);
    }
    else
    {
        // 获取人类玩家（ID为0）的队伍ID
        int humanPlayerTeamId = m_players[0]->getTeam()->getId();

        if (humanPlayerTeamId == winningTeamId)
        {
            // 人类玩家的队伍获胜
            title = tr("胜利！");
            message = tr("恭喜，你和你的队友取得了最终的胜利！");
        }
        else
        {
            // 人类玩家的队伍失败
            title = tr("失败");
            message = tr("很遗憾，你输了。再接再厉！");
        }
    }
    
    // 显示游戏结果
    qDebug() << "GuanDan::onGameOver：游戏结束QMessageBox被调用";
    QMessageBox::information(this, title, message);
}

void GuanDan::updateGameStatus()
{
    // 更新玩家界面显示
    for (int i = 0; i < m_playerWidgets.size(); ++i) {
        if (PlayerAreaWidget* widget = m_playerWidgets[i]) {
            if (Player* player = widget->getPlayer()) {
                // 更新手牌显示，底部玩家显示正面，其他玩家显示背面
                widget->updateHandDisplay(
                    player->getHandCards(),
                    widget->getPosition() == PlayerPosition::Bottom
                );
            }
        }
    }
}

// 处理进贡/还贡对话框
void GuanDan::onAskForTribute(int fromPlayerId, const QString& fromPlayerName, int toPlayerId, const QString& toPlayerName, bool isReturn)
{
    // 仅对人类玩家弹窗
    if (fromPlayerId != m_players[0]->getID()) return;
    
    // 获取手牌
    QVector<Card> hand = m_players[0]->getHandCards();
    qDebug() << "GuanDan::onAskForTribute：TributeDialog被调用";
    
    // 判断是否还贡给队友
    bool isToTeammate = false;
    if (isReturn) {
        // 获取fromPlayer和toPlayer
        Player* fromPlayer = nullptr;
        Player* toPlayer = nullptr;
        
        for (Player* p : m_players) {
            if (p->getID() == fromPlayerId) fromPlayer = p;
            if (p->getID() == toPlayerId) toPlayer = p;
        }
        
        // 检查是否为队友
        if (fromPlayer && toPlayer && fromPlayer->getTeam() && toPlayer->getTeam()) {
            isToTeammate = (fromPlayer->getTeam() == toPlayer->getTeam());
        }
    }
    
    // 创建对话框
    TributeDialog dialog(hand, isReturn, isToTeammate, this);
    if (dialog.exec() == QDialog::Accepted) {
        Card selectedCard = dialog.getSelectedCard();
        m_gameController->onPlayerTributeCardSelected(fromPlayerId, selectedCard);
    }
}