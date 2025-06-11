#include "PlayerAreaWidget.h"
#include <QResizeEvent>
#include <QDebug>

PlayerAreaWidget::PlayerAreaWidget(Player* player, PlayerPosition position, bool isCurrentPlayer, QWidget* parent)
    : QWidget(parent)
    , m_position(position)
    , m_mainLayout(nullptr)
{
    // 创建玩家控件
    m_playerWidget = new PlayerWidget(player, position, isCurrentPlayer, this);
    
    // 创建显示出牌控件
    m_showCardWidget = new ShowCardWidget(this);
    
    // 连接信号
    connect(m_playerWidget, &PlayerWidget::cardsSelected, this, &PlayerAreaWidget::cardsSelected);
    connect(m_playerWidget, &PlayerWidget::playCardsRequested, this, &PlayerAreaWidget::playCardsRequested);
    connect(m_playerWidget, &PlayerWidget::skipTurnRequested, this, &PlayerAreaWidget::skipTurnRequested);
    
    // 根据位置布局组件
    arrangeComponents();
}

PlayerAreaWidget::~PlayerAreaWidget()
{
    // 控件将随父窗口析构
}

void PlayerAreaWidget::arrangeComponents()
{
    // 删除旧布局
    if (m_mainLayout) {
        delete m_mainLayout;
        m_mainLayout = nullptr;
    }
    
    // 根据位置决定布局方向
    switch (m_position) {
    case PlayerPosition::Bottom:
    case PlayerPosition::Top:
        // 水平布局
        m_mainLayout = new QHBoxLayout(this);
        break;
    case PlayerPosition::Left:
    case PlayerPosition::Right:
        // 垂直布局
        m_mainLayout = new QVBoxLayout(this);
        break;
    }
    
    m_mainLayout->setContentsMargins(2, 2, 2, 2);
    m_mainLayout->setSpacing(5);
    
    // 根据位置决定控件顺序，让ShowCardWidget总是靠近屏幕中心
    switch (m_position) {
    case PlayerPosition::Bottom:
        // 底部玩家：ShowCardWidget在左，PlayerWidget在右
        m_mainLayout->addWidget(m_showCardWidget, 1);
        m_mainLayout->addWidget(m_playerWidget, 4);
        break;
    case PlayerPosition::Top:
        // 顶部玩家：PlayerWidget在左，ShowCardWidget在右
        m_mainLayout->addWidget(m_playerWidget, 4);
        m_mainLayout->addWidget(m_showCardWidget, 1);
        break;
    case PlayerPosition::Left:
        // 左侧玩家：PlayerWidget在上，ShowCardWidget在下
        m_mainLayout->addWidget(m_playerWidget, 4);
        m_mainLayout->addWidget(m_showCardWidget, 1);
        break;
    case PlayerPosition::Right:
        // 右侧玩家：ShowCardWidget在上，PlayerWidget在下
        m_mainLayout->addWidget(m_showCardWidget, 1);
        m_mainLayout->addWidget(m_playerWidget, 4);
        break;
    }
    
    // 调整大小以适应内容
    adjustSize();
}

void PlayerAreaWidget::updatePlayedCards(const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards)
{
    m_showCardWidget->updateDisplay(combo, originalCards);
}

void PlayerAreaWidget::clearPlayedCards()
{
    m_showCardWidget->clearDisplay();
}

void PlayerAreaWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 可以在这里添加额外的调整逻辑
}

// 代理PlayerWidget的方法实现
void PlayerAreaWidget::setPlayer(Player* player)
{
    m_playerWidget->setPlayer(player);
}

Player* PlayerAreaWidget::getPlayer() const
{
    return m_playerWidget->getPlayer();
}

void PlayerAreaWidget::setPlayerName(const QString& name)
{
    m_playerWidget->setPlayerName(name);
}

void PlayerAreaWidget::updatePlayerInfo()
{
    m_playerWidget->updatePlayerInfo();
}

void PlayerAreaWidget::updateCards(const QVector<Card>& cards)
{
    m_playerWidget->updateCards(cards);
}

void PlayerAreaWidget::removeCards(const QVector<Card>& cards)
{
    m_playerWidget->removeCards(cards);
}

void PlayerAreaWidget::addCards(const QVector<Card>& cards)
{
    m_playerWidget->addCards(cards);
}

void PlayerAreaWidget::clearSelection()
{
    m_playerWidget->clearSelection();
}

QVector<Card> PlayerAreaWidget::getSelectedCards() const
{
    return m_playerWidget->getSelectedCards();
}

void PlayerAreaWidget::setCardsVisible(bool visible)
{
    m_playerWidget->setCardsVisible(visible);
}

void PlayerAreaWidget::setHighlighted(bool highlighted)
{
    m_playerWidget->setHighlighted(highlighted);
}

void PlayerAreaWidget::setEnabled(bool enabled)
{
    m_playerWidget->setEnabled(enabled);
}

void PlayerAreaWidget::setPlayerStatus(const QString& status)
{
    m_playerWidget->setPlayerStatus(status);
}

void PlayerAreaWidget::setPosition(PlayerPosition position)
{
    if (m_position != position) {
        m_position = position;
        m_playerWidget->setPosition(position);
        arrangeComponents();
    }
}

PlayerPosition PlayerAreaWidget::getPosition() const
{
    return m_position;
}

void PlayerAreaWidget::setPlayerAvatar(const QString& avatarPath)
{
    m_playerWidget->setPlayerAvatar(avatarPath);
}

void PlayerAreaWidget::setDefaultAvatar()
{
    m_playerWidget->setDefaultAvatar();
}

void PlayerAreaWidget::setPlayerBackground(const QString& backgroundPath)
{
    m_playerWidget->setPlayerBackground(backgroundPath);
}

void PlayerAreaWidget::setDefaultBackground()
{
    m_playerWidget->setDefaultBackground();
}

void PlayerAreaWidget::updateHandDisplay(const QVector<Card>& cards, bool showFront)
{
    m_playerWidget->updateHandDisplay(cards, showFront);
}

void PlayerAreaWidget::updateHandDisplayNoAnimation(const QVector<Card>& cards, bool showFront)
{
    m_playerWidget->updateHandDisplayNoAnimation(cards, showFront);
} 