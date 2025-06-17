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

    // 设置初始级牌为红桃2
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
	// 水平主布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(2, 2, 2, 2); // 减小边距

	// Team1布局 (我方级牌) 
    QVBoxLayout* team1Layout = new QVBoxLayout();
    team1Layout->setSpacing(2); // 减小间距
    
    m_team1CardWidget = new CardWidget(this);
    m_team1CardWidget->setFixedSize(50, 70); // 设置卡牌尺寸
    m_team1Label = new QLabel("我方级牌", this);
	m_team1Label->setAlignment(Qt::AlignCenter); // 居中对齐
    m_team1Label->setStyleSheet("color: gold; font-weight: bold; background-color: transparent;");

	// 布局加入卡牌和标签
    team1Layout->addWidget(m_team1CardWidget, 0, Qt::AlignCenter);
    team1Layout->addWidget(m_team1Label, 0, Qt::AlignCenter);

	// Team2布局 (对方级牌)
    QVBoxLayout* team2Layout = new QVBoxLayout();
    team2Layout->setSpacing(2); // 减小间距
    
    m_team2CardWidget = new CardWidget(this);
    m_team2CardWidget->setFixedSize(50,70); // 设置卡牌尺寸
    m_team2Label = new QLabel("对方级牌", this);
    m_team2Label->setAlignment(Qt::AlignCenter);
    m_team2Label->setStyleSheet("color: gold; font-weight: bold; background-color: transparent;");

    // 布局加入卡牌和标签
    team2Layout->addWidget(m_team2CardWidget, 0, Qt::AlignCenter);
    team2Layout->addWidget(m_team2Label, 0, Qt::AlignCenter);

	// 将两队的布局添加到主布局中
    mainLayout->addLayout(team1Layout);
    mainLayout->addLayout(team2Layout);

    // 设置为显示正面
    m_team1CardWidget->setFrontSide(true);
    m_team2CardWidget->setFrontSide(true);

    // 禁用卡牌的交互
    m_team1CardWidget->setEnabled(false);
    m_team2CardWidget->setEnabled(false);
}

void LevelIndicatorWidget::updateLevels(Card::CardPoint team1Level, Card::CardPoint team2Level)
{
    // 设置级牌显示中新的级牌
    Card team1Card(team1Level, Card::Heart);
    m_team1CardWidget->setCard(team1Card);
	m_team1CardWidget->loadCardImages(); // 调用后自动刷新卡牌图片

    Card team2Card(team2Level, Card::Heart);
    m_team2CardWidget->setCard(team2Card);
    m_team2CardWidget->loadCardImages(); // 调用后自动刷新卡牌图片
}
