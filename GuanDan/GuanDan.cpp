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
    , m_cardCounterWidget(nullptr)
    , m_gameInProgress(false)
    , m_settingsButton(nullptr)
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
    setFixedSize(1640, 1080); // 将宽度从1440增加到1640，为记牌器腾出空间

    // 创建中央窗口部件和主布局
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建游戏区域
    QWidget* gameArea = new QWidget(this);
    gameArea->setStyleSheet("QWidget { background-color: #1B5E20; }");
    m_mainLayout->addWidget(gameArea, 1);

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

    // 创建设置按钮
    m_settingsButton = new QPushButton(tr("设置"), controlArea);
    m_settingsButton->setFixedSize(100, 40);
    m_settingsButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 20px;"
        "   font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1565C0;"
        "}"
    );

    // 为控制区域创建布局
    QHBoxLayout* controlLayout = new QHBoxLayout(controlArea);
    controlLayout->addWidget(m_startButton, 0, Qt::AlignCenter);
    controlLayout->addWidget(m_settingsButton, 0, Qt::AlignCenter);
    // 全局出牌/跳过按钮，仅在玩家0回合可见
    m_globalPlayButton = new QPushButton(tr("出牌"), controlArea);
    m_globalPlayButton->setFixedSize(100, 40);
    m_globalPlayButton->hide();
    controlLayout->addWidget(m_globalPlayButton, 0, Qt::AlignCenter);
    m_globalSkipButton = new QPushButton(tr("跳过"), controlArea);
    m_globalSkipButton->setFixedSize(100, 40);
    m_globalSkipButton->hide();
    controlLayout->addWidget(m_globalSkipButton, 0, Qt::AlignCenter);

    // 将控制区域添加到主布局
    m_mainLayout->addWidget(controlArea);

    // 创建记牌器部件
    m_cardCounterWidget = new CardCounterWidget(gameArea);
    m_cardCounterWidget->hide(); // 初始时隐藏

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
    connect(m_globalPlayButton, &QPushButton::clicked, this, [this]() {
        // 从底部玩家获取选中牌
        QVector<Card> cards = m_playerWidgets[0]->getSelectedCards();
        if (!cards.isEmpty()) {
            m_gameController->onPlayerPlay(0, cards);
        }
    });
    connect(m_globalSkipButton, &QPushButton::clicked, this, [this]() {
        m_gameController->onPlayerPass(0);
    });
    // 根据控制器启用信号控制全局按钮显示
    connect(m_gameController, &GD_Controller::sigEnablePlayerControls,
        this, [this](int playerId, bool canPlay, bool canPass) {
            if (playerId == 0) {
                m_globalPlayButton->setVisible(canPlay);
                m_globalPlayButton->setEnabled(canPlay);
                m_globalSkipButton->setVisible(canPass);
                m_globalSkipButton->setEnabled(canPass);
            } else {
                m_globalPlayButton->hide();
                m_globalSkipButton->hide();
            }
        });

    // 连接游戏控制器信号
    connect(m_gameController, &GD_Controller::sigGameStarted,
        this, &GuanDan::onGameStarted);
    connect(m_gameController, &GD_Controller::sigNewRoundStarted,
        this, &GuanDan::onNewRoundStarted);
    connect(m_gameController, &GD_Controller::sigRoundOver,
        this, &GuanDan::onRoundOver);
    connect(m_gameController, &GD_Controller::sigGameOver,
        this, &GuanDan::onGameOver);
        
    // 连接出牌和过牌显示信号
    connect(m_gameController, &GD_Controller::sigUpdateTableCards,
        this, [this](int playerId, const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards) {
            // 更新所有玩家的出牌显示区域
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->updatePlayedCards(combo, originalCards);
                }
            }
        });
        
    connect(m_gameController, &GD_Controller::sigPlayerPassed,
        this, [this](int playerId) {
            // 清空过牌玩家的出牌显示
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->clearPlayedCards();
                }
            }
        });
        
    connect(m_gameController, &GD_Controller::sigClearTableCards,
        this, [this]() {
            // 清空所有玩家的出牌显示
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                widget->clearPlayedCards();
            }
        });
    connect(m_gameController, &GD_Controller::sigCardsDealt,
        this, [this](int playerId, const QVector<Card>& cards) {
            qDebug() << "收到发牌信号 - 玩家ID:" << playerId << "牌数:" << cards.size();
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
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
                    break;
                }
            }
        });

    // 连接玩家界面信号
    for (PlayerAreaWidget* widget : m_playerWidgets) {
        // 当玩家选择卡牌时更新按钮状态，但不重新显示所有卡牌
        connect(widget, &PlayerAreaWidget::cardsSelected,
            [this, widget](const QVector<Card>& cards) {
                qDebug() << "收到卡牌选择信号 - 玩家:" << widget->getPlayer()->getName()
                         << "选中卡牌数量:" << cards.size();
            });
        
        // 当玩家选择卡牌时通知游戏控制器
        connect(widget, &PlayerAreaWidget::playCardsRequested,
            [this, widget]() {
                if (widget->getPlayer()) {
                    QVector<Card> selectedCards = widget->getSelectedCards();
                    if (!selectedCards.isEmpty()) {
                        m_gameController->onPlayerPlay(widget->getPlayer()->getID(), selectedCards);
                    }
                }
            });
            
        // 当玩家点击跳过按钮时通知游戏控制器
        connect(widget, &PlayerAreaWidget::skipTurnRequested,
            [this, widget]() {
                if (widget->getPlayer()) {
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
            for (PlayerAreaWidget* widget : m_playerWidgets) {
                if (widget->getPlayer()) {
                }
            }
        });

    // 连接进贡/还贡请求，弹出TributeDialog
    connect(m_gameController, &GD_Controller::sigAskForTribute,
        this, &GuanDan::onAskForTribute);
        
    // 连接记牌器更新信号
    connect(m_gameController, &GD_Controller::sigCardCountsUpdated,
        m_cardCounterWidget, &CardCounterWidget::updateCounts);
}

void GuanDan::arrangePlayerWidgets()
{
    QWidget* gameArea = m_centralWidget->findChild<QWidget*>();
    if (!gameArea) return;

    QSize gameSize = gameArea->size();

    const int LAYOUT_OFFSET_X = 200;

    // 为1640x1080窗口重新设计的布局参数
    int horizontalWidth = 900;   // 上下玩家区域的宽度
    int bottomHeight = 400;      // 底部玩家区域的高度
    int topHeight = 340;         // 增加顶部玩家区域的高度，从280增加到320
    int verticalWidth = 500;     // 左右玩家区域的宽度
    int verticalHeight = 620;    // 左右玩家区域高度

    // 修正垂直位置计算，避免重叠
    // 此计算逻辑旨在将侧边区域放置在顶部和底部区域之间的中心
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

    // 布局记牌器，放在左上角
    if (m_cardCounterWidget) {
        m_cardCounterWidget->setGeometry(20, 20, 150, 450);
    }
}

void GuanDan::showSettingsDialog()
{
    SettingsDialog dialog(this);
    dialog.exec();
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
    updateGameStatus();
    
    // 显示记牌器
    if (m_cardCounterWidget) {
        m_cardCounterWidget->show();
    }
    
    // 添加调试代码，检查所有玩家控件的状态
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
    // 新一轮开始时，清空所有玩家的出牌区域，解决状态残留问题
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
    
    // 播放胜负音效
    if (winningTeamId == 0) { // 假设0是玩家所在队伍
        SoundManager::instance().playWinSound();
    } else {
        SoundManager::instance().playLoseSound();
    }
    
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