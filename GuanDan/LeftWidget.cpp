#include "LeftWidget.h"
#include "CardCounterWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

LeftWidget::LeftWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void LeftWidget::setupUI() {
    this->setFixedWidth(180); // 给一个固定的宽度
    
    // --- 样式表 ---
    this->setStyleSheet(R"(
        LeftWidget {
            background-color: rgba(0, 0, 0, 0.4);
            border-radius: 10px;
            padding: 10px;
        }
        QGroupBox {
            color: white;
            font-weight: bold;
            font-size: 16px;
            border: 1px solid rgba(255, 255, 255, 0.5);
            border-radius: 5px;
            margin-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 0 5px;
        }
        QLabel {
            color: white;
            font-size: 14px;
        }
        QProgressBar {
            border: 1px solid grey;
            border-radius: 5px;
            background-color: #444;
            height: 10px;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #4CAF50;
            border-radius: 4px;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // 1. 记牌器
    m_cardCounterWidget = new CardCounterWidget(this);
    m_cardCounterWidget->setStyleSheet("background-color: transparent; border: none; padding: 0px;"); // 透明化记牌器背景
    mainLayout->addWidget(m_cardCounterWidget);

    // 2. 积分状态 GroupBox
    m_gameStateBox = new QGroupBox("积分状态", this);
    QVBoxLayout* gameStateLayout = new QVBoxLayout(m_gameStateBox);

    // -- 基础底分 --
    QHBoxLayout* baseScoreLayout = new QHBoxLayout();
    baseScoreLayout->addWidget(new QLabel("基础底分:", this));
    m_baseScoreLabel = new QLabel("1", this);
    m_baseScoreLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    baseScoreLayout->addWidget(m_baseScoreLabel);
    gameStateLayout->addLayout(baseScoreLayout);

    // -- 我方积分 -- (玩家0是我方)
    QHBoxLayout* team1Layout = new QHBoxLayout();
    team1Layout->addWidget(new QLabel("我方积分:", this));
    m_team1ScoreLabel = new QLabel("0", this);
    m_team1ScoreLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    team1Layout->addWidget(m_team1ScoreLabel);
    gameStateLayout->addLayout(team1Layout);

    // -- 对方积分 --
    QHBoxLayout* team2Layout = new QHBoxLayout();
    team2Layout->addWidget(new QLabel("对方积分:", this));
    m_team2ScoreLabel = new QLabel("0", this);
    m_team2ScoreLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    team2Layout->addWidget(m_team2ScoreLabel);
    gameStateLayout->addLayout(team2Layout);

    // -- 本局倍率 --
    QHBoxLayout* multiplierLayout = new QHBoxLayout();
    multiplierLayout->addWidget(new QLabel("本局倍率:", this));
    m_multiplierLabel = new QLabel("x1", this);
    m_multiplierLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_multiplierLabel->setStyleSheet("font-weight: bold; color: #FFD700;"); // 倍率用金色突出
    multiplierLayout->addWidget(m_multiplierLabel);
    gameStateLayout->addLayout(multiplierLayout);
    mainLayout->addWidget(m_gameStateBox);

    // 3. 当前回合 GroupBox
    m_currentTurnBox = new QGroupBox("当前回合", this);
    QVBoxLayout* currentTurnLayout = new QVBoxLayout(m_currentTurnBox);
    currentTurnLayout->setSpacing(5);

    m_currentPlayerNameLabel = new QLabel("等待开始...", this);
    m_currentPlayerNameLabel->setAlignment(Qt::AlignCenter);
    m_currentPlayerNameLabel->setStyleSheet("font-weight: bold; color: #87CEEB;"); // 天蓝色
    currentTurnLayout->addWidget(m_currentPlayerNameLabel);

    // 添加剩余时间标签
    m_timeRemainingLabel = new QLabel("30s", this);
    m_timeRemainingLabel->setAlignment(Qt::AlignCenter);
    m_timeRemainingLabel->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    currentTurnLayout->addWidget(m_timeRemainingLabel);

    m_turnTimerBar = new QProgressBar(this);
    m_turnTimerBar->setRange(0, 100);
    m_turnTimerBar->setValue(100);
    m_turnTimerBar->setTextVisible(false);
    currentTurnLayout->addWidget(m_turnTimerBar);
    
    mainLayout->addWidget(m_currentTurnBox);
    mainLayout->addStretch();
}

// --- 槽函数实现 ---
void LeftWidget::updateScores(int team1Score, int team2Score) {
    m_team1ScoreLabel->setText(QString::number(team1Score));
    m_team2ScoreLabel->setText(QString::number(team2Score));
}

void LeftWidget::updateMultiplier(int multiplier) {
    m_multiplierLabel->setText(QString("x%1").arg(multiplier));
}

void LeftWidget::updateCardCounts(const QMap<Card::CardPoint, int>& counts) {
    m_cardCounterWidget->updateCounts(counts);
}

void LeftWidget::setCurrentPlayer(const QString& playerName) {
    m_currentPlayerNameLabel->setText(playerName);
}

void LeftWidget::updateTurnIndicator(int currentPlayerId) {
    if (currentPlayerId == 0) { // 轮到我方（人类玩家）
        m_currentTurnBox->setStyleSheet(R"(
            QGroupBox {
                color: white;
                font-weight: bold;
                font-size: 16px;
                border: 2px solid #4CAF50; /* 绿色边框 */
                border-radius: 5px;
                margin-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top center;
                padding: 0 5px;
            }
        )");
    } else { // 轮到其他玩家
        m_currentTurnBox->setStyleSheet(R"(
            QGroupBox {
                color: white;
                font-weight: bold;
                font-size: 16px;
                border: 2px solid #F44336; /* 红色边框 */
                border-radius: 5px;
                margin-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top center;
                padding: 0 5px;
            }
        )");
    }
}

void LeftWidget::updateTimerDisplay(int secondsRemaining, int totalSeconds) {
    // 更新文字
    m_timeRemainingLabel->setText(QString("%1s").arg(secondsRemaining));

    // 更新进度条
    if (totalSeconds <= 0) return; // 防止除零
    int percentage = (secondsRemaining * 100) / totalSeconds;
    m_turnTimerBar->setValue(percentage);

    // 根据百分比更新颜色
    QString styleSheet;
    if (percentage > 60) {
        // 绿色
        styleSheet = "QProgressBar::chunk { background-color: #4CAF50; border-radius: 4px; }";
    } else if (percentage > 25) {
        // 黄色
        styleSheet = "QProgressBar::chunk { background-color: #FFC107; border-radius: 4px; }";
    } else {
        // 红色
        styleSheet = "QProgressBar::chunk { background-color: #F44336; border-radius: 4px; }";
    }
    m_turnTimerBar->setStyleSheet(styleSheet);
} 