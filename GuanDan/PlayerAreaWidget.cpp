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
    // 删除旧布局，防止内存泄漏
    if (m_mainLayout) {
        // 先将布局中的所有项移除，防止它们被一同删除
        while (QLayoutItem* item = m_mainLayout->takeAt(0)) {
            // 不删除 item->widget()，因为它们的父窗口是PlayerAreaWidget，会被自动管理
            delete item;
        }
        delete m_mainLayout;
        m_mainLayout = nullptr;
    }

    switch (m_position) {
    case PlayerPosition::Bottom:
    case PlayerPosition::Top:
        // 顶部和底部玩家，其内部控件是上下排列的，所以用QVBoxLayout
        m_mainLayout = new QVBoxLayout(this);
        break;
    case PlayerPosition::Left:
    case PlayerPosition::Right:
        // 左右玩家，其内部控件是左右排列的，所以用QHBoxLayout
        m_mainLayout = new QHBoxLayout(this);
        break;
    }

    // 调整边距，减少空间浪费
    m_mainLayout->setContentsMargins(2, 2, 2, 2);
    m_mainLayout->setSpacing(5);

    // 根据位置决定控件顺序，让ShowCardWidget总是靠近屏幕中心
    // 现在 m_mainLayout 是一个有效的指针，可以安全地添加控件
    switch (m_position) {
    case PlayerPosition::Bottom:
        // 底部玩家: 出牌区(ShowCardWidget)在上，手牌区(PlayerWidget)在下
        m_mainLayout->addWidget(m_showCardWidget, 1); // 占1份空间
        m_mainLayout->addWidget(m_playerWidget, 4);   // 占4份空间
        break;
    case PlayerPosition::Top:
        // 顶部玩家: 手牌区在上，出牌区在下
        m_mainLayout->addWidget(m_playerWidget, 1);     // 从0.5调整到1，给手牌区合理空间
        m_mainLayout->addWidget(m_showCardWidget, 4);   // 从6调整到4，保持出牌区足够大但不过分
        break;
    case PlayerPosition::Left:
        // 左侧玩家: 手牌区在左，出牌区在右
        m_mainLayout->addWidget(m_playerWidget, 2);   // 从1份增加到2份，增大手牌区
        m_mainLayout->addWidget(m_showCardWidget, 1); // 从2份减少到1份，减小出牌区
        break;
    case PlayerPosition::Right:
        // 右侧玩家: 出牌区在左，手牌区在右
        m_mainLayout->addWidget(m_showCardWidget, 1); // 从2份减少到1份，减小出牌区
        m_mainLayout->addWidget(m_playerWidget, 2);   // 从1份增加到2份，增大手牌区
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