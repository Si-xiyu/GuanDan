#include "LevelIndicatorWidget.h"

LevelIndicatorWidget::LevelIndicatorWidget(QWidget* parent)
    : QWidget(parent)
    , m_team1CardWidget(nullptr)
    , m_team2CardWidget(nullptr)
    , m_team1Label(nullptr)
    , m_team2Label(nullptr)
{
    // 设置整体背景为透明
    setStyleSheet("background-color: transparent;");
    
    setupUI();

    // Set initial level cards (default to 2)
    Card team1Card(Card::Card_2, Card::Heart);
    Card team2Card(Card::Card_2, Card::Heart);

    m_team1CardWidget->setCard(team1Card);
    m_team1CardWidget->loadCardImages();

    m_team2CardWidget->setCard(team2Card);
    m_team2CardWidget->loadCardImages();
}

LevelIndicatorWidget::~LevelIndicatorWidget()
{
    
}

void LevelIndicatorWidget::setupUI()
{
    // Main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(2, 2, 2, 2); // 减小边距

    // Team 1 (My Team) layout
    QVBoxLayout* team1Layout = new QVBoxLayout();
    team1Layout->setSpacing(2); // 减小间距
    
    m_team1CardWidget = new CardWidget(this);
    m_team1CardWidget->setFixedSize(50, 70); // 缩小卡牌尺寸
    m_team1Label = new QLabel("我方级牌", this);
    m_team1Label->setAlignment(Qt::AlignCenter);
    m_team1Label->setStyleSheet("color: gold; font-weight: bold; background-color: transparent;");

    team1Layout->addWidget(m_team1CardWidget, 0, Qt::AlignCenter);
    team1Layout->addWidget(m_team1Label, 0, Qt::AlignCenter);

    // Team 2 (Opponent Team) layout
    QVBoxLayout* team2Layout = new QVBoxLayout();
    team2Layout->setSpacing(2); // 减小间距
    
    m_team2CardWidget = new CardWidget(this);
    m_team2CardWidget->setFixedSize(50,70); // 缩小卡牌尺寸
    m_team2Label = new QLabel("对方级牌", this);
    m_team2Label->setAlignment(Qt::AlignCenter);
    m_team2Label->setStyleSheet("color: gold; font-weight: bold; background-color: transparent;");

    team2Layout->addWidget(m_team2CardWidget, 0, Qt::AlignCenter);
    team2Layout->addWidget(m_team2Label, 0, Qt::AlignCenter);

    // Add both team layouts to main layout
    mainLayout->addLayout(team1Layout);
    mainLayout->addLayout(team2Layout);

    // Set cards to front side
    m_team1CardWidget->setFrontSide(true);
    m_team2CardWidget->setFrontSide(true);

    // Disable card selection
    m_team1CardWidget->setEnabled(false);
    m_team2CardWidget->setEnabled(false);
}

void LevelIndicatorWidget::updateLevels(Card::CardPoint team1Level, Card::CardPoint team2Level)
{
    // Create temporary cards with the specified levels
    Card team1Card(team1Level, Card::Heart);
    m_team1CardWidget->setCard(team1Card);
    m_team1CardWidget->loadCardImages(); // Must call to refresh the card image

    Card team2Card(team2Level, Card::Heart);
    m_team2CardWidget->setCard(team2Card);
    m_team2CardWidget->loadCardImages(); // Must call to refresh the card image
}
