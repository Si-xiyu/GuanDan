#include "LevelStatusWidget.h"
#include <QDebug>

LevelStatusWidget::LevelStatusWidget(QWidget* parent)
    : QWidget(parent)
    , m_team0NameLabel(nullptr)
    , m_team0LevelLabel(nullptr)
    , m_team1NameLabel(nullptr)
    , m_team1LevelLabel(nullptr)
    , m_mainLayout(nullptr)
    , m_team0Layout(nullptr)
    , m_team1Layout(nullptr)
    , m_team0Effect(nullptr)
    , m_team1Effect(nullptr)
    , m_team0Animation(nullptr)
    , m_team1Animation(nullptr)
    , m_animationTimer(nullptr)
{
    setupUI();
    setupAnimations();

    // 设置默认大小和背景
    setFixedSize(300, 80);
    setStyleSheet("LevelStatusWidget { background-color: rgba(255, 255, 255, 230); "
        "border: 2px solid #333; border-radius: 8px; }");
}

void LevelStatusWidget::setupUI()
{
    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    m_mainLayout->setSpacing(20);

    // 创建队伍0的布局和标签
    m_team0Layout = new QVBoxLayout();
    m_team0NameLabel = new QLabel("队伍0", this);
    m_team0LevelLabel = new QLabel("♠2", this);

    // 创建队伍1的布局和标签
    m_team1Layout = new QVBoxLayout();
    m_team1NameLabel = new QLabel("队伍1", this);
    m_team1LevelLabel = new QLabel("♠2", this);

    // 设置字体样式
    QFont nameFont("Arial", 12, QFont::Bold);
    QFont levelFont("Arial", 16, QFont::Bold);

    // 队伍名称样式
    m_team0NameLabel->setFont(nameFont);
    m_team0NameLabel->setAlignment(Qt::AlignCenter);
    m_team0NameLabel->setStyleSheet("color: #2E86C1; font-weight: bold;");

    m_team1NameLabel->setFont(nameFont);
    m_team1NameLabel->setAlignment(Qt::AlignCenter);
    m_team1NameLabel->setStyleSheet("color: #E74C3C; font-weight: bold;");

    // 级牌标签样式
    m_team0LevelLabel->setFont(levelFont);
    m_team0LevelLabel->setAlignment(Qt::AlignCenter);
    m_team0LevelLabel->setStyleSheet("color: black; background-color: rgba(255, 255, 255, 180); "
        "border: 1px solid #333; border-radius: 4px; "
        "padding: 2px 8px; min-width: 30px;");

    m_team1LevelLabel->setFont(levelFont);
    m_team1LevelLabel->setAlignment(Qt::AlignCenter);
    m_team1LevelLabel->setStyleSheet("color: black; background-color: rgba(255, 255, 255, 180); "
        "border: 1px solid #333; border-radius: 4px; "
        "padding: 2px 8px; min-width: 30px;");

    // 组装布局
    m_team0Layout->addWidget(m_team0NameLabel);
    m_team0Layout->addWidget(m_team0LevelLabel);
    m_team0Layout->setAlignment(Qt::AlignCenter);

    m_team1Layout->addWidget(m_team1NameLabel);
    m_team1Layout->addWidget(m_team1LevelLabel);
    m_team1Layout->setAlignment(Qt::AlignCenter);

    // 添加到主布局
    m_mainLayout->addLayout(m_team0Layout);
    m_mainLayout->addWidget(createSeparator());
    m_mainLayout->addLayout(m_team1Layout);

    setLayout(m_mainLayout);
}

QWidget* LevelStatusWidget::createSeparator()
{
    QLabel* separator = new QLabel("|", this);
    separator->setAlignment(Qt::AlignCenter);
    separator->setStyleSheet("color: #7F8C8D; font-size: 20px; font-weight: bold;");
    return separator;
}

void LevelStatusWidget::setupAnimations()
{
    // 创建透明度效果
    m_team0Effect = new QGraphicsOpacityEffect(this);
    m_team1Effect = new QGraphicsOpacityEffect(this);

    m_team0LevelLabel->setGraphicsEffect(m_team0Effect);
    m_team1LevelLabel->setGraphicsEffect(m_team1Effect);

    // 创建动画
    m_team0Animation = new QPropertyAnimation(m_team0Effect, "opacity", this);
    m_team1Animation = new QPropertyAnimation(m_team1Effect, "opacity", this);

    // 设置动画参数
    m_team0Animation->setDuration(200);
    m_team1Animation->setDuration(200);

    // 创建定时器用于控制动画序列
    m_animationTimer = new QTimer(this);
    m_animationTimer->setSingleShot(true);
}

void LevelStatusWidget::setTeamNames(const QString& team0Name, const QString& team1Name)
{
    m_team0NameLabel->setText(team0Name);
    m_team1NameLabel->setText(team1Name);

    qDebug() << "LevelStatusWidget::setTeamNames -" << team0Name << "vs" << team1Name;
}

void LevelStatusWidget::resetLevels()
{
    // 重置为初始级牌2
    updateTeamLevel(0, Card::Card_2);
    updateTeamLevel(1, Card::Card_2);

    qDebug() << "LevelStatusWidget::resetLevels - 级牌已重置为2";
}

void LevelStatusWidget::updateTeamLevel(int teamId, Card::CardPoint newLevel)
{
    qDebug() << "LevelStatusWidget::updateTeamLevel - 队伍" << teamId
        << "级牌更新为" << levelToString(newLevel);

    // 更新显示
    updateTeamDisplay(teamId, newLevel);

    // 播放动画效果
    playLevelChangeAnimation(teamId);
}

void LevelStatusWidget::updateTeamDisplay(int teamId, Card::CardPoint level)
{
    QString levelText = getLevelIcon(level) + levelToString(level);
    QString color = getLevelColor(level);

    if (teamId == 0) {
        m_team0LevelLabel->setText(levelText);
        m_team0LevelLabel->setStyleSheet(QString("color: %1; background-color: rgba(255, 255, 255, 180); "
            "border: 1px solid #333; border-radius: 4px; "
            "padding: 2px 8px; min-width: 30px; font-weight: bold;")
            .arg(color));
    }
    else if (teamId == 1) {
        m_team1LevelLabel->setText(levelText);
        m_team1LevelLabel->setStyleSheet(QString("color: %1; background-color: rgba(255, 255, 255, 180); "
            "border: 1px solid #333; border-radius: 4px; "
            "padding: 2px 8px; min-width: 30px; font-weight: bold;")
            .arg(color));
    }
}

QString LevelStatusWidget::levelToString(Card::CardPoint level) const
{
    switch (level) {
    case Card::Card_2: return "2";
    case Card::Card_3: return "3";
    case Card::Card_4: return "4";
    case Card::Card_5: return "5";
    case Card::Card_6: return "6";
    case Card::Card_7: return "7";
    case Card::Card_8: return "8";
    case Card::Card_9: return "9";
    case Card::Card_10: return "10";
    case Card::Card_J: return "J";
    case Card::Card_Q: return "Q";
    case Card::Card_K: return "K";
    case Card::Card_A: return "A";
    case Card::Card_LJ: return "小王";
    case Card::Card_BJ: return "大王";
    default: return QString::number(static_cast<int>(level));
    }
}

QString LevelStatusWidget::getLevelIcon(Card::CardPoint level) const
{
    // 根据级牌显示不同的花色图标
    switch (level) {
    case Card::Card_2:
    case Card::Card_6:
    case Card::Card_10:
    case Card::Card_A:
        return "♠"; // 黑桃
    case Card::Card_3:
    case Card::Card_7:
    case Card::Card_J:
        return "♣"; // 梅花
    case Card::Card_4:
    case Card::Card_8:
    case Card::Card_Q:
        return "♦"; // 方块
    case Card::Card_5:
    case Card::Card_9:
    case Card::Card_K:
        return "♥"; // 红桃
    case Card::Card_LJ:
    case Card::Card_BJ:
        return "★"; // 王牌用星号
    default:
        return "♠";
    }
}

QString LevelStatusWidget::getLevelColor(Card::CardPoint level) const
{
    // 红色花色使用红色，黑色花色使用黑色，王牌使用金色
    switch (level) {
    case Card::Card_4:  // 方块
    case Card::Card_8:
    case Card::Card_Q:
    case Card::Card_5:  // 红桃
    case Card::Card_9:
    case Card::Card_K:
        return "#E74C3C"; // 红色
    case Card::Card_LJ: // 小王
    case Card::Card_BJ: // 大王
        return "#F39C12"; // 金色
    default:
        return "#2C3E50"; // 黑色
    }
}

void LevelStatusWidget::playLevelChangeAnimation(int teamId)
{
    QPropertyAnimation* animation = (teamId == 0) ? m_team0Animation : m_team1Animation;

    // 停止之前的动画
    animation->stop();

    // 设置闪烁效果：完全不透明 -> 半透明 -> 完全不透明
    animation->setStartValue(1.0);
    animation->setKeyValueAt(0.5, 0.3);
    animation->setEndValue(1.0);

    // 启动动画
    animation->start();

    // 添加音效提示（如果需要）
    qDebug() << "LevelStatusWidget::playLevelChangeAnimation - 队伍" << teamId << "级牌变化动画播放";
}