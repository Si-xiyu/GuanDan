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
    this->setFixedWidth(200); // 适度增加宽度
    this->setMinimumHeight(600); // 设置最小高度
    
    // --- 样式表 ---
    this->setStyleSheet(R"(
        QGroupBox {
            color: white;
            font-weight: bold;
            font-size: 14px; /* 减小标题字体 */
            border: 1px solid rgba(255, 255, 255, 0.5);
            border-radius: 5px;
            margin-top: 8px; /* 减小顶部边距 */
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center; /* 标题居中 */
            padding: 0 5px;
        }
        QLabel {
            color: white;
            font-size: 12px; /* 减小标签字体 */
        }
        QProgressBar {
            border: 1px solid grey;
            border-radius: 5px;
            background-color: #444;
            height: 8px; /* 减小进度条高度 */
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #2196F3;
            border-radius: 4px;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6); // 减小控件间距
    mainLayout->setContentsMargins(5, 5, 5, 5); // 减小边距

    // 1. 级牌显示器
    m_levelIndicator = new LevelIndicatorWidget(this);
    m_levelIndicator->setFixedHeight(90); // 增加高度
    mainLayout->addWidget(m_levelIndicator);

    // 2. 记牌器
    m_cardCounterWidget = new CardCounterWidget(this);
    m_cardCounterWidget->setStyleSheet("background-color: transparent; border: none; padding: 0px;");
    m_cardCounterWidget->setFixedHeight(180); // 增加高度
    mainLayout->addWidget(m_cardCounterWidget, 1); // 给予较大的拉伸因子

    // 3. 排行榜
    m_rankingBox = new QGroupBox("本局排名", this);
    QVBoxLayout* rankingLayout = new QVBoxLayout(m_rankingBox);
    rankingLayout->setSpacing(2); // 减小间距
    rankingLayout->setContentsMargins(5, 12, 5, 5); // 减小内边距，但保持顶部空间给标题

    m_rankLabels.clear();
    for (int i = 0; i < 4; ++i) {
        QLabel* rankLabel = new QLabel(QString("第 %1 名: -").arg(i + 1), this);
        rankLabel->setStyleSheet("color: #E0E0E0;");
        m_rankLabels.append(rankLabel);
        rankingLayout->addWidget(rankLabel);
    }
    m_rankingBox->setMinimumHeight(100); // 减小高度
    mainLayout->addWidget(m_rankingBox, 1); // 给予拉伸因子

    // 4. 积分状态 GroupBox
    m_gameStateBox = new QGroupBox("积分状态", this);
    QVBoxLayout* gameStateLayout = new QVBoxLayout(m_gameStateBox);
    gameStateLayout->setSpacing(2); // 减小间距
    gameStateLayout->setContentsMargins(5, 12, 5, 5); // 减小内边距
    m_gameStateBox->setMinimumHeight(100); // 设置最小高度
    mainLayout->addWidget(m_gameStateBox, 1); // 给予拉伸因子

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

    // 5. 当前回合 GroupBox
    m_currentTurnBox = new QGroupBox("当前回合", this);
    QVBoxLayout* currentTurnLayout = new QVBoxLayout(m_currentTurnBox);
    currentTurnLayout->setSpacing(2); // 减小间距
    currentTurnLayout->setContentsMargins(5, 12, 5, 5); // 减小内边距
    m_currentTurnBox->setMinimumHeight(100); // 设置最小高度
    mainLayout->addWidget(m_currentTurnBox, 1); // 给予拉伸因子

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
    
    mainLayout->addStretch(0.5); // 减小底部拉伸因子
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

void LeftWidget::updateTeamLevels(Card::CardPoint team1Level, Card::CardPoint team2Level) {
    // 将级牌更新传递给级牌显示器
    m_levelIndicator->updateLevels(team1Level, team2Level);
}

// 实现排行榜相关的槽函数
void LeftWidget::updateRanking(const QStringList& rankedPlayerNames)
{
    for (int i = 0; i < rankedPlayerNames.size() && i < m_rankLabels.size(); ++i) {
        m_rankLabels[i]->setText(QString("第 %1 名: %2").arg(i + 1).arg(rankedPlayerNames[i]));
        
        // 为不同名次设置不同的样式
        switch (i) {
        case 0: // 第一名
            m_rankLabels[i]->setStyleSheet("color: #FFD700; font-weight: bold;"); // 金色
            break;
        case 1: // 第二名
            m_rankLabels[i]->setStyleSheet("color: #C0C0C0; font-weight: bold;"); // 银色
            break;
        case 2: // 第三名
            m_rankLabels[i]->setStyleSheet("color: #CD7F32;"); // 铜色
            break;
        default: // 第四名
            m_rankLabels[i]->setStyleSheet("color: #E0E0E0;"); // 浅灰色
            break;
        }
    }
}

void LeftWidget::clearRanking()
{
    for (int i = 0; i < m_rankLabels.size(); ++i) {
        m_rankLabels[i]->setText(QString("第 %1 名: -").arg(i + 1));
        m_rankLabels[i]->setStyleSheet("color: #E0E0E0; font-weight: normal;"); // 重置样式
    }
} 