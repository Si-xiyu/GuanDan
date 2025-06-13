#include "PlayerWidget.h"
#include "CardWidget.h" 
#include <QPainter>
#include <QDebug>
#include <algorithm> // std::sort
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPropertyAnimation>
#include <QtMath>
#include <QTimer>
#include <QContextMenuEvent>
#include <QMenu>

#include "SoundManager.h"

PlayerWidget::PlayerWidget(Player* player, PlayerPosition position, bool isCurrentPlayer, QWidget* parent)
    : QWidget(parent)
    , m_player(player)
    , m_position(position)
    , m_isCurrentPlayer(isCurrentPlayer)
    , m_isEnabled(true)
    , m_isHighlighted(false)
    , m_useCustomBackground(true)
{
    // 加载默认资源
    loadDefaultResources();
    
    // 根据位置设置合适的尺寸
    QSize preferredSize = calculatePreferredSize();
    setMinimumSize(preferredSize);
    resize(preferredSize);
    
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // 创建信息区域
    QHBoxLayout* infoLayout = new QHBoxLayout();
    
    // 创建名称和状态标签的垂直布局
    QVBoxLayout* labelsLayout = new QVBoxLayout();
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("QLabel { color: white; font-size: 14px; font-weight: bold; }");
    labelsLayout->addWidget(m_nameLabel);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    labelsLayout->addWidget(m_statusLabel);
    
    if (m_position == PlayerPosition::Left) {
        infoLayout->addStretch();
        infoLayout->addLayout(labelsLayout);
        m_nameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    } else {
        infoLayout->addLayout(labelsLayout);
        infoLayout->addStretch();
        m_nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    
    m_mainLayout->addLayout(infoLayout);
    
    // 更新玩家信息
    updatePlayerInfo();
    
    // 设置背景色
    setStyleSheet("PlayerWidget { background-color: rgba(0, 100, 0, 180); border-radius: 10px; }");
    
    qDebug() << "PlayerWidget构造完成 - 玩家:" << (m_player ? m_player->getName() : "无名") 
             << "位置:" << static_cast<int>(m_position)
             << "当前玩家:" << m_isCurrentPlayer;
}

PlayerWidget::~PlayerWidget()
{
    // 清理所有卡片视图
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_cardStacks.clear();

    // 清理已打出的牌
    qDeleteAll(m_playedCardWidgets);
    m_playedCardWidgets.clear();
}
void PlayerWidget::updateCards(const QVector<Card>& cards)
{
    // 停止所有正在进行的动画
    stopAllAnimations();

    // 清理现有卡片
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_cardStacks.clear();

    // 创建新的卡片视图
    for (const Card& card : cards) {
        CardWidget* cardWidget = createCardWidget(card);
        m_cardWidgets.append(cardWidget);
        m_cardStacks[card.point()].append(cardWidget);
    }

    // 对卡片进行排序
    sortCards();

    // 重新布局
    relayoutCards();

    // 更新玩家信息
    updatePlayerInfo();
}
void PlayerWidget::removeCards(const QVector<Card>& cards)
{
    for (const Card& card : cards) {
        // 查找并移除对应的卡片视图
        for (int i = 0; i < m_cardWidgets.size(); ++i) {
            if (m_cardWidgets[i]->getCard() == card) {
                // 从堆叠中移除
                m_cardStacks[card.point()].removeOne(m_cardWidgets[i]);
                if (m_cardStacks[card.point()].isEmpty()) {
                    m_cardStacks.remove(card.point());
                }
                
                // 删除卡片视图
                delete m_cardWidgets[i];
                m_cardWidgets.removeAt(i);
                break;
            }
        }
    }
    
    // 重新布局
    relayoutCards();
}

void PlayerWidget::addCards(const QVector<Card>& cards)
{
    for (const Card& card : cards) {
        CardWidget* cardWidget = createCardWidget(card);
        m_cardWidgets.append(cardWidget);
        m_cardStacks[card.point()].append(cardWidget);
    }
    
    // 对卡片进行排序
    sortCards();
    
    // 重新布局
    relayoutCards();
}

QVector<Card> PlayerWidget::getSelectedCards() const
{
    QVector<Card> selectedCards;
    for (CardWidget* widget : m_cardWidgets) {
        if (widget->isSelected()) {
            selectedCards.append(widget->getCard());
        }
    }
    return selectedCards;
}

void PlayerWidget::selectCards(const QVector<Card>& cardsToSelect)
{
    qDebug() << "PlayerWidget::selectCards: 收到选择" << cardsToSelect.size() << "张牌的请求。";

    // 1. 先清除所有旧的选择
    for (CardWidget* widget : m_cardWidgets) {
        widget->setSelected(false);
    }

    // 2. 统计需要选中的每种牌的数量
    // 为了让Card能做QMap的key，Card.h中必须有 operator< 的重载，你的代码已经有了，这很好！
    QMap<Card, int> selectionCounts;
    for (const Card& card : cardsToSelect) {
        selectionCounts[card]++;
    }

    // 3. 遍历UI上的所有牌控件，精确地选中所需数量的牌
    for (CardWidget* widget : m_cardWidgets) {
        const Card& widgetCard = widget->getCard();

        // 检查这张牌是否在我们的待选列表中，并且数量还 > 0
        if (selectionCounts.contains(widgetCard) && selectionCounts[widgetCard] > 0) {
            widget->setSelected(true);
            // 将该牌的待选数量减一
            selectionCounts[widgetCard]--;
        }
    }

    // 4. 通知外部选择已更新，这会触发按钮状态的更新
    emit cardsSelected(getSelectedCards());

    qDebug() << "PlayerWidget: 最终选中了" << getSelectedCards().size() << "张牌。";
	// setSelected内部会调用update()
}

void PlayerWidget::clearSelection()
{
    for (CardWidget* widget : m_cardWidgets) {
        widget->setSelected(false);
    }
}

void PlayerWidget::setEnabled(bool enabled)
{
    qDebug() << "PlayerWidget::setEnabled - 玩家:" << (m_player ? m_player->getName() : "无名") 
             << "启用状态改变:" << m_isEnabled << "->" << enabled;
    
    m_isEnabled = enabled;
    
    // 更新卡片的启用状态
    for (CardWidget* widget : m_cardWidgets) {
        widget->setEnabled(enabled);
    }
    
    // 更新整个控件的外观
    if (enabled) {
        // 当前玩家回合时的高亮样式
        setStyleSheet("PlayerWidget { background-color: rgba(0, 150, 0, 200); border-radius: 10px; }");
    } else {
        // 非当前玩家回合时的样式
        setStyleSheet("PlayerWidget { background-color: rgba(0, 100, 0, 180); border-radius: 10px; }");
    }
    
    update();
}

void PlayerWidget::setCardsVisible(bool visible)
{
    for (CardWidget* widget : m_cardWidgets) {
        widget->setFrontSide(visible);
    }
}

void PlayerWidget::setPlayerStatus(const QString& status)
{
    m_statusLabel->setText(status);
}

void PlayerWidget::setHighlighted(bool highlighted)
{
    m_isHighlighted = highlighted;
    update();
}

void PlayerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // 更新标签位置
    if (m_position == PlayerPosition::Bottom || m_position == PlayerPosition::Top) {
        m_nameLabel->setGeometry(5, 5, width() - 10, NAME_LABEL_HEIGHT);
        m_statusLabel->setGeometry(5, NAME_LABEL_HEIGHT + 5, width() - 10, NAME_LABEL_HEIGHT);
    } else {
        m_nameLabel->setGeometry(5, height() / 2 - NAME_LABEL_HEIGHT, width() - 10, NAME_LABEL_HEIGHT);
        m_statusLabel->setGeometry(5, height() / 2, width() - 10, NAME_LABEL_HEIGHT);
    }
    
    // 重新布局卡片
    relayoutCards();
}

void PlayerWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    if (m_useCustomBackground && !m_backgroundPixmap.isNull()) {
        painter.drawPixmap(rect(), m_backgroundPixmap);
    } else if (!m_defaultBackgroundPixmap.isNull()) {
        painter.drawPixmap(rect(), m_defaultBackgroundPixmap);
    } else {
        QColor bgColor = m_isHighlighted ? QColor(0, 150, 0, 180) : QColor(0, 100, 0, 180);
        painter.fillRect(rect(), bgColor);
    }
    
    // 如果是当前玩家，绘制边框
    if (m_isCurrentPlayer) {
        QPen pen(Qt::yellow, 2);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void PlayerWidget::relayoutCards()
{
    qDebug() << "PlayerWidget::relayoutCards - 开始重新布局卡牌";
    
    // 根据位置使用不同的布局方法
    switch (m_position) {
    case PlayerPosition::Bottom:
        layoutCardsForBottom();
        break;
    case PlayerPosition::Top:
        layoutCardsForTop();
        break;
    case PlayerPosition::Left:
    case PlayerPosition::Right:
        layoutCardsForSide();
        break;
    }
    
    // 更新所有卡片的Z顺序
    updateCardZOrder();
    
    qDebug() << "卡牌布局完成 - 总计:" << m_cardWidgets.size() << "张牌";
}

void PlayerWidget::layoutCardsForBottom()
{
    if (m_cardWidgets.isEmpty()) return;

    // 计算卡片区域，预留更多空间给手牌区域
    QRect cardsArea = rect().adjusted(CARDS_MARGIN, 2 * NAME_LABEL_HEIGHT + 20,
        -CARDS_MARGIN, -CARDS_MARGIN);

    if (cardsArea.width() <= 0 || cardsArea.height() <= 0) {
        return; // 避免无效区域
    }

    // 按点数分组计算布局
    QVector<Card::CardPoint> sortedPoints = m_cardStacks.keys().toVector();
    std::sort(sortedPoints.begin(), sortedPoints.end());

    // 计算总宽度
    int totalWidth = 0;
    if (!sortedPoints.isEmpty()) {
        totalWidth = (sortedPoints.size() - 1) * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL) + CARD_WIDGET_WIDTH;
    }

    // 如果卡片总宽度超过可用区域，调整重叠度
    int actualOverlap = CARD_OVERLAP_HORIZONTAL;
    if (totalWidth > cardsArea.width() && sortedPoints.size() > 1) {
        int excessWidth = totalWidth - cardsArea.width();
        int additionalOverlap = excessWidth / (sortedPoints.size() - 1);
        actualOverlap = qMin(CARD_OVERLAP_HORIZONTAL + additionalOverlap, CARD_WIDGET_WIDTH - 20);
        totalWidth = (sortedPoints.size() - 1) * (CARD_WIDGET_WIDTH - actualOverlap) + CARD_WIDGET_WIDTH;
    }

    // 计算起始X坐标，使卡片居中显示
    int currentX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
    // 【关键修复】: 将Y轴的基准点设置到卡片区域的底部，并留出足够空间
    int baseY = cardsArea.bottom() - CARD_WIDGET_HEIGHT;

    // 获取窗口中心点在本控件坐标系中的起始位置
    QPoint globalCenter = window()->geometry().center();
    QPoint startPos = mapFromGlobal(globalCenter);

    // 按点数顺序布局卡片
    for (Card::CardPoint point : sortedPoints) {
        QVector<CardWidget*>& stack = m_cardStacks[point];
        if (stack.isEmpty()) continue;

        // 布局这一组牌
        for (int i = 0; i < stack.size(); ++i) {
            CardWidget* card = stack[i];
            card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            card->setRotation(0);
            card->setParent(this);

            // 【关键修复】: 计算目标位置，从底部向上堆叠
            int x = currentX;
            int y = baseY - i * CARD_OVERLAP_VERTICAL;

            // 直接设置位置，不使用动画，避免回合开始时的位置错误
            card->move(x, y);
        }
        currentX += CARD_WIDGET_WIDTH - actualOverlap;
    }

    // 更新Z顺序
    updateCardZOrder();
}

void PlayerWidget::layoutCardsForSide()
{
    // 计算卡片区域
    QRect cardsArea = rect().adjusted(CARDS_MARGIN, NAME_LABEL_HEIGHT * 2 + 10,
                                    -CARDS_MARGIN, -CARDS_MARGIN);
    
    // 计算总高度，确保居中显示
    int totalHeight = (m_cardWidgets.size() - 1) * SIDE_CARD_OVERLAP + CARD_WIDGET_HEIGHT;
    int startY = cardsArea.top() + (cardsArea.height() - totalHeight) / 2;
    
    // 确定中心X坐标
    int centerX = width() / 2;
    if (m_position == PlayerPosition::Left) {
        centerX = CARDS_MARGIN + CARD_WIDGET_HEIGHT / 2;
    } else {
        centerX = width() - CARDS_MARGIN - CARD_WIDGET_HEIGHT / 2;
    }
    
    // 对所有卡片进行布局
    for (int i = 0; i < m_cardWidgets.size(); ++i) {
        CardWidget* card = m_cardWidgets[i];
        
        // 确保卡片大小正确
        card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
        
        // 设置旋转角度
        int rotation = (m_position == PlayerPosition::Left) ? -90 : 90;
        card->setRotation(rotation);
        
        // 计算位置
        int y = startY + i * SIDE_CARD_OVERLAP;
        
        // 创建动画
        QPropertyAnimation* animation = new QPropertyAnimation(card, "pos");
        animation->setDuration(200);
        animation->setStartValue(card->pos());
        // 由于卡片旋转了90度，需要调整位置以确保正确显示
        QPoint targetPos(centerX - CARD_WIDGET_HEIGHT/2, y);
        animation->setEndValue(targetPos);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void PlayerWidget::layoutCardsForTop()
{
    // 计算卡片区域
    QRect cardsArea = rect().adjusted(CARDS_MARGIN, NAME_LABEL_HEIGHT * 2 + 10,
                                    -CARDS_MARGIN, -CARDS_MARGIN);
    
    // 计算总宽度，确保居中显示
    int totalWidth = (m_cardWidgets.size() - 1) * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL) + CARD_WIDGET_WIDTH;
    int startX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
    int centerY = cardsArea.top() + (cardsArea.height() - CARD_WIDGET_HEIGHT) / 2;
    
    // 对所有卡片进行布局
    for (int i = 0; i < m_cardWidgets.size(); ++i) {
        CardWidget* card = m_cardWidgets[i];
        
        // 确保卡片大小正确
        card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
        card->setRotation(0); // 确保卡片没有旋转
        
        // 计算位置
        int x = startX + i * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL);
        
        // 创建动画
        QPropertyAnimation* animation = new QPropertyAnimation(card, "pos");
        animation->setDuration(200);
        animation->setStartValue(card->pos());
        animation->setEndValue(QPoint(x, centerY));
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void PlayerWidget::sortCards()
{
    // 清空现有的堆栈
    m_cardStacks.clear();

    // 重新按点数分组
    for (CardWidget* widget : m_cardWidgets) {
        Card::CardPoint point = widget->getCard().point();
        m_cardStacks[point].append(widget);
    }

    // 在每个堆栈内部按花色排序
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
            [](CardWidget* a, CardWidget* b) {
                const Card& cardA = a->getCard();
                const Card& cardB = b->getCard();

                // 首先按点数排序
                if (cardA.point() != cardB.point()) {
                    return cardA.point() < cardB.point();
                }

                // 点数相同则按花色排序
                return cardA.suit() < cardB.suit();
            });
    }

    // 重建m_cardWidgets数组，保持正确顺序
    m_cardWidgets.clear();
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        for (CardWidget* widget : it.value()) {
            m_cardWidgets.append(widget);
        }
    }
}


CardWidget* PlayerWidget::createCardWidget(const Card& card)
{
    CardWidget* widget = new CardWidget(card, m_player, this);
    widget->setFrontSide(m_isCurrentPlayer);
    widget->setEnabled(m_isEnabled);
    widget->show();
    
    // 连接点击信号
    connect(widget, &CardWidget::clicked, this, &PlayerWidget::cardClicked);
    
    return widget;
}

void PlayerWidget::updatePlayerInfo()
{
    if (m_player) {
        m_nameLabel->setText(m_player->getName());
        
        // 获取当前实际手牌数量
        // 从Player对象获取手牌数量，确保数据源的权威性
        int handCardCount = m_player->getHandCards().size();
        
        m_statusLabel->setText(QString("剩余牌数：%1").arg(handCardCount));
        
        qDebug() << "更新玩家信息:" << m_player->getName() 
                 << "剩余牌数:" << handCardCount;
    }
    
    // 清空选择状态（因为手牌发生了变化）
    clearSelection();
}

QPoint PlayerWidget::calculateCardPosition(int index, int stackIndex) const
{
    return QPoint(index * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL),
                 stackIndex * CARD_OVERLAP_VERTICAL);
}

int PlayerWidget::getStackSize(const Card& card) const
{
    auto it = m_cardStacks.find(card.point());
    return it != m_cardStacks.end() ? it.value().size() : 0;
}

void PlayerWidget::clearPlayedCardsArea()
{
    // 清理已打出的牌
    qDeleteAll(m_playedCardWidgets);
    m_playedCardWidgets.clear();
}

void PlayerWidget::displayPlayedCombo(const QVector<Card>& cards)
{
    // 清理之前显示的牌
    clearPlayedCardsArea();

    // 创建新的卡片视图显示打出的牌
    for (const Card& card : cards) {
        CardWidget* cardWidget = new CardWidget(card, m_player, this);
        cardWidget->setFrontSide(true);  // 打出的牌总是显示正面
        cardWidget->setEnabled(false);    // 打出的牌不能被选择
        cardWidget->show();
        m_playedCardWidgets.append(cardWidget);
    }
}

void PlayerWidget::setPlayer(Player* player)
{
    m_player = player;
    updatePlayerInfo();
    
    // 如果有手牌，更新显示
    if (m_player) {
        updateCards(m_player->getHandCards());
    }
}

Player* PlayerWidget::getPlayer() const
{
    return m_player;
}

void PlayerWidget::updateHandDisplay(const QVector<Card>& handCards, bool showCardFronts)
{
    qDebug() << "PlayerWidget::updateHandDisplay - 玩家:" << (m_player ? m_player->getName() : "无名")
             << "卡牌数量:" << handCards.size()
             << "显示正面:" << showCardFronts;
    // 停止所有动画
    stopAllAnimations();

    // 清理现有卡片
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_cardStacks.clear();

    // 创建新的卡片视图
    for (const Card& card : handCards) {
        CardWidget* cardWidget = createCardWidget(card);
        cardWidget->setFrontSide(showCardFronts);
        cardWidget->setEnabled(m_isEnabled && showCardFronts);
        m_cardWidgets.append(cardWidget);
        m_cardStacks[card.point()].append(cardWidget);
        
        // 注意：不需要在这里再次连接信号，因为createCardWidget已经连接了
    }

    // 对卡片进行排序
    sortCards();

    // 重新布局
    relayoutCards();

    qDebug() << "手牌显示更新完成 - 创建了" << m_cardWidgets.size() << "个卡片控件";
}

void PlayerWidget::updateHandDisplayNoAnimation(const QVector<Card>& handCards, bool showCardFronts)
{
    qDebug() << "PlayerWidget::updateHandDisplayNoAnimation - 玩家:" << (m_player ? m_player->getName() : "无名")
             << "卡牌数量:" << handCards.size()
             << "显示正面:" << showCardFronts;
    // 停止所有动画
    stopAllAnimations();

    // 清理现有卡片
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_cardStacks.clear();

    // 创建新的卡片视图
    for (const Card& card : handCards) {
        CardWidget* cardWidget = createCardWidget(card);
        cardWidget->setFrontSide(showCardFronts);
        cardWidget->setEnabled(m_isEnabled && showCardFronts);
        m_cardWidgets.append(cardWidget);
        m_cardStacks[card.point()].append(cardWidget);
    }

    // 对卡片进行排序并静态布局
    sortCards();
    relayoutCardsStatic();

    qDebug() << "手牌静态更新完成 - 总计:" << m_cardWidgets.size() << "张牌";
}

// 添加静态布局函数，无动画
void PlayerWidget::relayoutCardsStatic()
{
    switch (m_position) {
    case PlayerPosition::Bottom: {
        QRect cardsArea = rect().adjusted(CARDS_MARGIN, 2 * NAME_LABEL_HEIGHT + 20,
            -CARDS_MARGIN, -CARDS_MARGIN);
        if (cardsArea.width() <= 0 || cardsArea.height() <= 0) return;
        QVector<Card::CardPoint> sortedPoints = m_cardStacks.keys().toVector();
        std::sort(sortedPoints.begin(), sortedPoints.end());
        int totalWidth = 0;
        if (!sortedPoints.isEmpty()) {
            totalWidth = (sortedPoints.size() - 1) * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL) + CARD_WIDGET_WIDTH;
        }
        int actualOverlap = CARD_OVERLAP_HORIZONTAL;
        if (totalWidth > cardsArea.width() && sortedPoints.size() > 1) {
            int excessWidth = totalWidth - cardsArea.width();
            int additionalOverlap = excessWidth / (sortedPoints.size() - 1);
            actualOverlap = qMin(CARD_OVERLAP_HORIZONTAL + additionalOverlap, CARD_WIDGET_WIDTH - 20);
            totalWidth = (sortedPoints.size() - 1) * (CARD_WIDGET_WIDTH - actualOverlap) + CARD_WIDGET_WIDTH;
        }
        int currentX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
        // 【关键修复】: 将Y轴的基准点设置到卡片区域的底部
        int baseY = cardsArea.bottom() - CARD_WIDGET_HEIGHT;
        for (auto point : sortedPoints) {
            auto& stack = m_cardStacks[point];
            for (int i = 0; i < stack.size(); ++i) {
                CardWidget* card = stack[i];
                card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
                card->setRotation(0);
                card->setParent(this);
                int x = currentX;
                int y = baseY - i * CARD_OVERLAP_VERTICAL;
                card->move(x, y);
            }
            currentX += CARD_WIDGET_WIDTH - actualOverlap;
        }
        break;
    }
    case PlayerPosition::Left:
    case PlayerPosition::Right: {
        QRect cardsArea = rect().adjusted(CARDS_MARGIN, NAME_LABEL_HEIGHT * 2 + 10,
                                          -CARDS_MARGIN, -CARDS_MARGIN);
        int totalHeight = (m_cardWidgets.size() - 1) * SIDE_CARD_OVERLAP + CARD_WIDGET_HEIGHT;
        int startY = cardsArea.top() + (cardsArea.height() - totalHeight) / 2;
        int centerX = width() / 2;
        if (m_position == PlayerPosition::Left)
            centerX = CARDS_MARGIN + CARD_WIDGET_HEIGHT / 2;
        else
            centerX = width() - CARDS_MARGIN - CARD_WIDGET_HEIGHT / 2;
        for (int i = 0; i < m_cardWidgets.size(); ++i) {
            CardWidget* card = m_cardWidgets[i];
            card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            card->setRotation(m_position == PlayerPosition::Left ? -90 : 90);
            int y = startY + i * SIDE_CARD_OVERLAP;
            card->move(centerX - CARD_WIDGET_HEIGHT/2, y);
        }
        break;
    }
    case PlayerPosition::Top: {
        QRect cardsArea = rect().adjusted(CARDS_MARGIN, NAME_LABEL_HEIGHT * 2 + 10,
                                          -CARDS_MARGIN, -CARDS_MARGIN);
        int totalWidth = (m_cardWidgets.size() - 1) * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL) + CARD_WIDGET_WIDTH;
        int startX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
        int centerY = cardsArea.top() + (cardsArea.height() - CARD_WIDGET_HEIGHT) / 2;
        for (int i = 0; i < m_cardWidgets.size(); ++i) {
            CardWidget* card = m_cardWidgets[i];
            card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            card->setRotation(0);
            int x = startX + i * (CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL);
            card->move(x, centerY);
        }
        break;
    }
    }
    updateCardZOrder();
}

void PlayerWidget::setPlayerName(const QString& name)
{
    m_nameLabel->setText(name);
}

void PlayerWidget::setPosition(PlayerPosition position)
{
    if (m_position != position) {
        m_position = position;
        QSize newSize = calculatePreferredSize();
        setMinimumSize(newSize);
        resize(newSize);
        relayoutCards();
    }
}

QSize PlayerWidget::calculatePreferredSize() const
{
    int height = PLAYER_WIDGET_MIN_HEIGHT;
    if (m_position == PlayerPosition::Bottom) {
        height += 40; // 为按钮预留空间
    }
    
    switch (m_position) {
        case PlayerPosition::Left:
        case PlayerPosition::Right:
            return QSize(CARD_WIDGET_HEIGHT + 2 * CARDS_MARGIN,
                       PLAYER_WIDGET_MIN_HEIGHT * 2);
        case PlayerPosition::Top:
            return QSize(PLAYER_WIDGET_MIN_WIDTH,
                       CARD_WIDGET_HEIGHT + NAME_LABEL_HEIGHT * 2 + CARDS_MARGIN * 2);
        case PlayerPosition::Bottom:
        default:
            return QSize(PLAYER_WIDGET_MIN_WIDTH, height);
    }
}

// 添加一个辅助函数来处理卡片的Z顺序
void PlayerWidget::updateCardZOrder()
{
    // 获取排序后的点数列表
    QVector<Card::CardPoint> sortedPoints = m_cardStacks.keys().toVector();
    std::sort(sortedPoints.begin(), sortedPoints.end());
    
    // 收集所有卡片及其Z顺序信息
    QVector<QPair<CardWidget*, int>> cardZOrders;

    // 按照从左到右的顺序计算Z值
    for (int groupIndex = 0; groupIndex < sortedPoints.size(); ++groupIndex) {
        Card::CardPoint point = sortedPoints[groupIndex];
        QVector<CardWidget*>& stack = m_cardStacks[point];

        if (stack.isEmpty()) continue;

        // 在组内部，按照y坐标设置Z值
        // 需要注意：在layoutCardsForBottom中，cardIndex=0对应的是y坐标最大的牌（最下方）
        // 而我们希望y坐标最大的牌在Z轴上层
        for (int cardIndex = 0; cardIndex < stack.size(); ++cardIndex) {
            CardWidget* card = stack[cardIndex];

            // 组间基础层级：每个组比前一个组高1000层，确保组间不会重叠
            int baseZLevel = groupIndex * 1000;

            // 组内层级：cardIndex=0是y坐标最大的牌（最下方），应该在Z轴最上层
            // 所以cardIndex越小，Z层级越高
            int inGroupZLevel = (stack.size() - 1 - cardIndex) * 10;

            int totalZLevel = baseZLevel + inGroupZLevel;
            cardZOrders.append(qMakePair(card, totalZLevel));

        }
    }

    // 按Z顺序从低到高排序
    std::sort(cardZOrders.begin(), cardZOrders.end(),
        [](const QPair<CardWidget*, int>& a, const QPair<CardWidget*, int>& b) {
            return a.second < b.second;
        });

    // 先将所有卡片降到底层
    for (const auto& pair : cardZOrders) {
        pair.first->lower();
    }

    // 按照计算的顺序逐个提升
    for (const auto& pair : cardZOrders) {
        pair.first->raise();
    }
}


void PlayerWidget::loadDefaultResources()
{
    // 加载默认背景
    QString defaultBackgroundPath = ":/pic/res/player_bg.png";
    if (!m_defaultBackgroundPixmap.load(defaultBackgroundPath)) {
        qWarning() << "Failed to load default background:" << defaultBackgroundPath;
        m_defaultBackgroundPixmap = QPixmap(); // 使用空背景，会fallback到纯色背景
    }
}

void PlayerWidget::setPlayerBackground(const QString& backgroundPath)
{
    if (backgroundPath.isEmpty()) {
        setDefaultBackground();
        return;
    }
    
    if (m_backgroundPixmap.load(backgroundPath)) {
        m_useCustomBackground = true;
        update();
    } else {
        qWarning() << "Failed to load background from path:" << backgroundPath;
        setDefaultBackground();
    }
}

void PlayerWidget::setDefaultBackground()
{
    m_useCustomBackground = false;
    update();
}

void PlayerWidget::cardClicked(CardWidget* clickedWidget)
{
    qDebug() << "PlayerWidget::cardClicked - 玩家:" << (m_player ? m_player->getName() : "无名")
             << "点击卡牌:" << clickedWidget->getCard().PointToString()
             << clickedWidget->getCard().SuitToString()
             << "启用状态:" << m_isEnabled
             << "位置:" << static_cast<int>(m_position)
             << "卡牌位置:" << clickedWidget->pos()
             << "卡牌父控件:" << (clickedWidget->parentWidget() ? clickedWidget->parentWidget()->metaObject()->className() : "无");
             
    if (!m_isEnabled) {
        qDebug() << "忽略卡牌点击 - 控件未启用";
        return;
    }
    
    if (m_position != PlayerPosition::Bottom) {
        qDebug() << "忽略卡牌点击 - 非底部玩家";
        return;
    }

    // 确保卡牌确实属于这个玩家
    bool cardFound = false;
    for (CardWidget* card : m_cardWidgets) {
        if (card == clickedWidget) {
            cardFound = true;
            break;
        }
    }
    
    if (!cardFound) {
        qDebug() << "忽略卡牌点击 - 卡牌不属于该玩家";
        return;
    }

    SoundManager::instance().playButtonClickSound();

    // 切换卡片的选中状态
    clickedWidget->setSelected(!clickedWidget->isSelected());
    qDebug() << "切换卡片选中状态 - 当前状态:" << clickedWidget->isSelected();

    // 发送选中卡牌信号
    QVector<Card> selectedCards = getSelectedCards();
    qDebug() << "发送选中卡牌信号 - 选中数量:" << selectedCards.size();
    emit cardsSelected(selectedCards);
}

// 保留右键菜单事件处理，以便无按钮时仍可通过菜单出牌或过牌
void PlayerWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_position == PlayerPosition::Bottom && m_isEnabled) {
        QMenu menu(this);
        QAction* playAction = menu.addAction("出牌");
        QAction* skipAction = menu.addAction("跳过");
        
        // 如果没有选中的牌，禁用出牌选项
        playAction->setEnabled(!getSelectedCards().isEmpty());
        
        QAction* selectedAction = menu.exec(event->globalPos());
        if (selectedAction == playAction) {
            emit playCardsRequested();
        } else if (selectedAction == skipAction) {
            emit skipTurnRequested();
        }
    }
}

// 停止所有动画的方法
void PlayerWidget::stopAllAnimations()
{
    // 停止所有卡片的位置动画
    for (CardWidget* widget : m_cardWidgets) {
        QPropertyAnimation* animation = widget->findChild<QPropertyAnimation*>();
        if (animation) {
            animation->stop();
        }
    }
    // 取消所有延迟的定时器
    for (QTimer* timer : findChildren<QTimer*>()) {
        if (timer->isSingleShot()) {
            timer->stop();
        }
    }
}

// 新增：卡片移除动画
void PlayerWidget::animateCardsRemoval(const QVector<CardWidget*>& widgets)
{
    for (CardWidget* widget : widgets) {
        // 创建淡出动画
        QPropertyAnimation* fadeOut = new QPropertyAnimation(widget, "windowOpacity");
        fadeOut->setDuration(200);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);

        // 创建缩放动画
        QPropertyAnimation* scaleOut = new QPropertyAnimation(widget, "geometry");
        scaleOut->setDuration(200);
        QRect currentGeometry = widget->geometry();
        QRect targetGeometry(currentGeometry.center().x() - 5,
                              currentGeometry.center().y() - 5,
                              10, 10);
        scaleOut->setStartValue(currentGeometry);
        scaleOut->setEndValue(targetGeometry);
        connect(fadeOut, &QPropertyAnimation::finished, [widget]() { widget->deleteLater(); });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        scaleOut->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

// 新增：卡片添加动画
void PlayerWidget::animateCardsAddition(const QVector<CardWidget*>& widgets)
{
    for (CardWidget* widget : widgets) {
        widget->show();

        // 创建淡入动画
        QPropertyAnimation* fadeIn = new QPropertyAnimation(widget, "windowOpacity");
        fadeIn->setDuration(300);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);

        // 创建从小到大的缩放动画
        QPropertyAnimation* scaleIn = new QPropertyAnimation(widget, "geometry");
        scaleIn->setDuration(300);
        QRect targetGeometry(widget->x(), widget->y(), CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
        QRect startGeometry(targetGeometry.center().x() - 5,
                            targetGeometry.center().y() - 5,
                            10, 10);
        scaleIn->setStartValue(startGeometry);
        scaleIn->setEndValue(targetGeometry);

        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        scaleIn->start(QAbstractAnimation::DeleteWhenStopped);
    }
}