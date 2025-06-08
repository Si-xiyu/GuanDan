#include "GuanDan.h"
#include <QMessageBox>
#include <QScreen>
#include <QApplication>
#include <QTimer>
#include <QDebug>

GuanDan::GuanDan(QWidget* parent)
    : QMainWindow(parent)
    , m_gameController(nullptr)
    , m_startButton(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_gameInProgress(false)
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
    // 设置窗口标题和初始大小
    setWindowTitle(tr("掼蛋游戏"));
    resize(1280, 960);

    // 创建中央窗口部件和主布局
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建游戏区域
    QWidget* gameArea = new QWidget(this);
    gameArea->setStyleSheet("QWidget { background-color: #1B5E20; }");
    m_mainLayout->addWidget(gameArea, 1); // 设置stretch为1，使其填充可用空间

    // 创建底部控制区域
    QWidget* controlArea = new QWidget(this);
    controlArea->setFixedHeight(80);
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

    // 为控制区域创建布局
    QHBoxLayout* controlLayout = new QHBoxLayout(controlArea);
    controlLayout->addWidget(m_startButton, 0, Qt::AlignCenter);

    // 将控制区域添加到主布局
    m_mainLayout->addWidget(controlArea);

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
        // 创建玩家对象
        Player* player = new Player(QString("玩家%1").arg(i + 1), i);
        m_players.append(player);
        qDebug() << "创建玩家:" << player->getName() << "ID:" << player->getID();
        
        // 创建玩家界面，设置位置和是否为当前玩家
        PlayerPosition position;
        switch (i) {
        case 0:
            position = PlayerPosition::Bottom;
            break;
        case 1:
            position = PlayerPosition::Left;
            break;
        case 2:
            position = PlayerPosition::Top;
            break;
        case 3:
            position = PlayerPosition::Right;
            break;
        }

        // 创建玩家界面
        PlayerWidget* playerWidget = new PlayerWidget(player, position, i == 0, gameArea);
        m_playerWidgets.append(playerWidget);
        playerWidget->show();
        qDebug() << "创建玩家界面:" << player->getName() << "位置:" << static_cast<int>(position);
    }

    // 初始布局玩家界面
    arrangePlayerWidgets();
}

void GuanDan::setupConnections()
{
    // 连接开始游戏按钮
    connect(m_startButton, &QPushButton::clicked, this, &GuanDan::startGame);

    // 连接游戏控制器信号
    connect(m_gameController, &GD_Controller::sigGameStarted,
        this, &GuanDan::onGameStarted);
    connect(m_gameController, &GD_Controller::sigNewRoundStarted,
        this, &GuanDan::onNewRoundStarted);
    connect(m_gameController, &GD_Controller::sigRoundOver,
        this, &GuanDan::onRoundOver);
    connect(m_gameController, &GD_Controller::sigGameOver,
        this, &GuanDan::onGameOver);
    connect(m_gameController, &GD_Controller::sigCardsDealt,
        this, [this](int playerId, const QVector<Card>& cards) {
            qDebug() << "收到发牌信号 - 玩家ID:" << playerId << "牌数:" << cards.size();
            for (PlayerWidget* widget : m_playerWidgets) {
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
            for (PlayerWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    widget->updateHandDisplayNoAnimation(cards, widget->getPlayer()->getID() == 0);
                    break;
                }
            }
        });

    // 连接玩家界面信号
    for (PlayerWidget* widget : m_playerWidgets) {
        // 当玩家选择卡牌时更新按钮状态，但不重新显示所有卡牌
        connect(widget, &PlayerWidget::cardsSelected,
            [this, widget](const QVector<Card>& cards) {
                qDebug() << "收到卡牌选择信号 - 玩家:" << widget->getPlayer()->getName()
                         << "选中卡牌数量:" << cards.size();
                
                // 只更新按钮状态，不重新显示所有卡牌
                widget->updateButtonsState();
            });
        
        // 当玩家选择卡牌时通知游戏控制器
        connect(widget, &PlayerWidget::playCardsRequested,
            [this, widget]() {
                if (widget->getPlayer()) {
                    QVector<Card> selectedCards = widget->getSelectedCards();
                    if (!selectedCards.isEmpty()) {
                        m_gameController->onPlayerPlay(widget->getPlayer()->getID(), selectedCards);
                    }
                }
            });
            
        // 当玩家点击跳过按钮时通知游戏控制器
        connect(widget, &PlayerWidget::skipTurnRequested,
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
            for (PlayerWidget* widget : m_playerWidgets) {
                if (widget->getPlayer() && widget->getPlayer()->getID() == playerId) {
                    found = true;
                    widget->setEnabled(canPlay);
                    widget->updateButtonsState();
                    widget->highlightTurn(true); // 高亮显示当前玩家
                    qDebug() << "已更新玩家UI状态:" << widget->getPlayer()->getName();
                } else if (widget->getPlayer()) {
                    // 其他玩家设置为非高亮
                    widget->highlightTurn(false);
                }
            }
            
            if (!found) {
                qWarning() << "警告: 未找到ID为" << playerId << "的玩家控件";
            }
        });

    // 连接当前玩家回合信号
    connect(m_gameController, &GD_Controller::sigSetCurrentTurnPlayer,
        this, [this](int playerId, const QString& playerName) {
            for (PlayerWidget* widget : m_playerWidgets) {
                if (widget->getPlayer()) {
                    bool isCurrentPlayer = (widget->getPlayer()->getID() == playerId);
                    widget->highlightTurn(isCurrentPlayer);
                }
            }
        });
}

void GuanDan::arrangePlayerWidgets()
{
    // 获取游戏区域
    QWidget* gameArea = m_centralWidget->findChild<QWidget*>();
    if (!gameArea) return;

    QSize gameSize = gameArea->size();
    int margin = 20;
    
    // 计算玩家区域的大小
    int horizontalWidth = qMin(800, gameSize.width() - 2 * margin);
    int verticalWidth = 180;
    int verticalHeight = qMin(600, gameSize.height() - 2 * margin);
    int horizontalHeight = 180;

    // 设置玩家界面位置和大小
    // 底部玩家
    m_playerWidgets[0]->setGeometry(
        (gameSize.width() - horizontalWidth) / 2,
        gameSize.height() - horizontalHeight - margin,
        horizontalWidth,
        horizontalHeight
    );
    
    // 左侧玩家
    m_playerWidgets[1]->setGeometry(
        margin,
        (gameSize.height() - verticalHeight) / 2,
        verticalWidth,
        verticalHeight
    );
    
    // 顶部玩家
    m_playerWidgets[2]->setGeometry(
        (gameSize.width() - horizontalWidth) / 2,
        margin,
        horizontalWidth,
        horizontalHeight
    );
    
    // 右侧玩家
    m_playerWidgets[3]->setGeometry(
        gameSize.width() - verticalWidth - margin,
        (gameSize.height() - verticalHeight) / 2,
        verticalWidth,
        verticalHeight
    );

    // 显示所有玩家界面
    for (PlayerWidget* widget : m_playerWidgets) {
        widget->show();
    }
}

void GuanDan::startGame()
{
    if (!m_gameInProgress) {
        qDebug() << "开始游戏...";
        m_gameInProgress = true;
        m_startButton->setEnabled(false);
        m_startButton->hide(); // 隐藏开始按钮
        
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
        for (PlayerWidget* widget : m_playerWidgets) {
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
    // 确保底部玩家（第一个）初始化按钮并显示
    if (!m_playerWidgets.isEmpty()) {
        PlayerWidget* bottomWidget = m_playerWidgets[0];
        bottomWidget->setupButtons();
        bottomWidget->updateButtonsState();
    }
    
    // 添加调试代码，检查所有玩家控件的状态
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "检查玩家控件状态:";
        for (int i = 0; i < m_playerWidgets.size(); ++i) {
            PlayerWidget* widget = m_playerWidgets[i];
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
    // 更新界面显示
    updateGameStatus();
    QMessageBox::information(this, tr("新一轮"),
        tr("第 %1 轮开始").arg(roundNumber));
}

void GuanDan::onRoundOver(const QString& summary, const QVector<int>& playerRanks)
{
    // 更新界面显示
    updateGameStatus();
    
    // 显示本局结果
    QMessageBox::information(this, tr("本局结束"), summary);
}

void GuanDan::onGameOver(int winningTeamId, const QString& winningTeamName, const QString& finalMessage)
{
    m_gameInProgress = false;
    m_startButton->setEnabled(true);
    
    // 显示游戏结果
    QString message = tr("获胜队伍: %1\n%2").arg(winningTeamName).arg(finalMessage);
    QMessageBox::information(this, tr("游戏结束"), message);
}

void GuanDan::updateGameStatus()
{
    // 更新玩家界面显示
    for (int i = 0; i < m_playerWidgets.size(); ++i) {
        if (PlayerWidget* widget = m_playerWidgets[i]) {
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