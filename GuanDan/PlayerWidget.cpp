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

PlayerWidget::PlayerWidget(Player* player, PlayerPosition position, bool isCurrentPlayer, QWidget* parent)
    : QWidget(parent)
    , m_player(player)
    , m_position(position)
    , m_isCurrentPlayer(isCurrentPlayer)
    , m_isEnabled(false)
    , m_isHighlighted(false)
    , m_useCustomBackground(false)
    , m_playButton(nullptr)
    , m_skipButton(nullptr)
    , m_buttonLayout(nullptr)
{
    // 加载默认资源
    loadDefaultResources();
    
    // 根据位置设置合适的尺寸
    QSize preferredSize = calculatePreferredSize();
    setMinimumSize(preferredSize);
    resize(preferredSize);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // 创建头像和信息区域
    QHBoxLayout* infoLayout = new QHBoxLayout();
    
    // 创建头像标签
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(AVATAR_SIZE, AVATAR_SIZE);
    m_avatarLabel->setScaledContents(true);
    setDefaultAvatar();
    infoLayout->addWidget(m_avatarLabel);
    
    // 创建名称和状态标签的垂直布局
    QVBoxLayout* labelsLayout = new QVBoxLayout();
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_nameLabel->setStyleSheet("QLabel { color: white; font-size: 14px; font-weight: bold; }");
    labelsLayout->addWidget(m_nameLabel);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    labelsLayout->addWidget(m_statusLabel);
    
    infoLayout->addLayout(labelsLayout);
    infoLayout->addStretch();
    
    mainLayout->addLayout(infoLayout);
    
    // 如果是当前玩家（底部位置），添加按钮
    if (m_position == PlayerPosition::Bottom) {
        setupButtons();
        mainLayout->addLayout(m_buttonLayout);
    }
    
    // 更新玩家信息
    updatePlayerInfo();

    // 连接玩家手牌更新信号
    if (m_player) {
        connect(m_player, &Player::cardsUpdated, this, &PlayerWidget::updatePlayerInfo);
    }
    
    // 设置背景色
    setStyleSheet("PlayerWidget { background-color: rgba(0, 100, 0, 180); border-radius: 10px; }");
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

void PlayerWidget::clearSelection()
{
    for (CardWidget* widget : m_cardWidgets) {
        widget->setSelected(false);
    }
}

void PlayerWidget::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
    for (CardWidget* widget : m_cardWidgets) {
        widget->setEnabled(enabled);
    }
    updateButtonsState();
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
    
    // 更新头像位置
    int avatarMargin = 5;
    if (m_position == PlayerPosition::Bottom || m_position == PlayerPosition::Top) {
        m_avatarLabel->move(avatarMargin, avatarMargin);
    } else {
        m_avatarLabel->move(avatarMargin, height() / 2 - AVATAR_SIZE / 2);
    }
    
    // 更新标签位置
    if (m_position == PlayerPosition::Bottom || m_position == PlayerPosition::Top) {
        m_nameLabel->setGeometry(AVATAR_SIZE + 2 * avatarMargin, 5, width() - AVATAR_SIZE - 3 * avatarMargin, NAME_LABEL_HEIGHT);
        m_statusLabel->setGeometry(AVATAR_SIZE + 2 * avatarMargin, NAME_LABEL_HEIGHT + 5, width() - AVATAR_SIZE - 3 * avatarMargin, NAME_LABEL_HEIGHT);
    } else {
        m_nameLabel->setGeometry(AVATAR_SIZE + 2 * avatarMargin, height() / 2 - NAME_LABEL_HEIGHT, width() - AVATAR_SIZE - 3 * avatarMargin, NAME_LABEL_HEIGHT);
        m_statusLabel->setGeometry(AVATAR_SIZE + 2 * avatarMargin, height() / 2, width() - AVATAR_SIZE - 3 * avatarMargin, NAME_LABEL_HEIGHT);
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
    if (m_cardWidgets.isEmpty()) return;
    
    switch (m_position) {
        case PlayerPosition::Bottom:
            layoutCardsForBottom();
            break;
        case PlayerPosition::Left:
        case PlayerPosition::Right:
            layoutCardsForSide();
            break;
        case PlayerPosition::Top:
            layoutCardsForTop();
            break;
    }
}

void PlayerWidget::layoutCardsForBottom()
{
    if (m_cardWidgets.isEmpty()) return;

    // 计算卡片区域
    QRect cardsArea = rect().adjusted(CARDS_MARGIN, 2 * NAME_LABEL_HEIGHT + 10,
        -CARDS_MARGIN, -CARDS_MARGIN);

    if (cardsArea.width() <= 0 || cardsArea.height() <= 0) {
        return; // 避免无效区域
    }

    // 按点数分组计算布局
    QVector<Card::CardPoint> sortedPoints = m_cardStacks.keys().toVector();
    std::sort(sortedPoints.begin(), sortedPoints.end());

    // 计算总宽度
    int totalWidth = 0;
    for (Card::CardPoint point : sortedPoints) {
        if (!m_cardStacks[point].isEmpty()) {
            totalWidth += CARD_WIDGET_WIDTH;
            if (totalWidth > CARD_WIDGET_WIDTH) { // 不是第一组
                totalWidth -= CARD_OVERLAP_HORIZONTAL;
            }
        }
    }

    // 如果卡片总宽度超过可用区域，调整重叠度
    int actualOverlap = CARD_OVERLAP_HORIZONTAL;
    if (totalWidth > cardsArea.width() && sortedPoints.size() > 1) {
        int excessWidth = totalWidth - cardsArea.width();
        int additionalOverlap = excessWidth / (sortedPoints.size() - 1);
        actualOverlap = qMin(CARD_OVERLAP_HORIZONTAL + additionalOverlap,
            CARD_WIDGET_WIDTH - 10); // 最大重叠不超过卡片宽度-10

        // 重新计算总宽度
        totalWidth = sortedPoints.size() * CARD_WIDGET_WIDTH -
            (sortedPoints.size() - 1) * actualOverlap;
    }

    // 计算起始X坐标，使卡片居中显示
    int currentX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
    int baseY = cardsArea.top();

    // 按点数顺序布局卡片
    for (Card::CardPoint point : sortedPoints) {
        QVector<CardWidget*>& stack = m_cardStacks[point];
        if (stack.isEmpty()) continue;

        // 布局这一组牌
        for (int i = 0; i < stack.size(); ++i) {
            CardWidget* card = stack[i];

            // 确保卡片属性正确
            card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            card->setRotation(0);
            card->setParent(this);

            // 计算位置 - 同点数的牌向上堆叠（y坐标减小）
            int x = currentX;
            int y = baseY - i * CARD_OVERLAP_VERTICAL;

            // 使用动画移动到新位置
            QPropertyAnimation* animation = new QPropertyAnimation(card, "pos");
            animation->setDuration(150); // 缩短动画时间
            animation->setStartValue(card->pos());
            animation->setEndValue(QPoint(x, y));
            animation->setEasingCurve(QEasingCurve::OutCubic);

            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }

        // 移动到下一组牌的位置
        currentX += CARD_WIDGET_WIDTH - actualOverlap;
    }

    // 在动画完成后更新Z顺序
    QTimer::singleShot(200, this, &PlayerWidget::updateCardZOrder);
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
    connect(widget, &CardWidget::clicked, this, [this](CardWidget* clickedWidget) {
        // 发送点击信号
        emit cardClicked(clickedWidget);
        
        // 如果启用了选择功能，发送选中牌的信号并更新按钮状态
        if (m_isEnabled) {
            emit cardsSelected(getSelectedCards());
            updateButtonsState();
        }
    });
    
    return widget;
}

void PlayerWidget::updatePlayerInfo()
{
    if (m_player) {
        m_nameLabel->setText(m_player->getName());
        
        // 获取当前实际手牌数量
        int handCardCount = m_cardWidgets.size();
        // 也可以从Player对象获取，但要确保同步
        // int handCardCount = m_player->getHandCards().size();
        
        m_statusLabel->setText(QString("剩余牌数：%1").arg(handCardCount));
        
        qDebug() << "更新玩家信息:" << m_player->getName() 
                 << "剩余牌数:" << handCardCount;
    }
    
    // 更新按钮状态
    updateButtonsState();
    
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

    // TODO: 实现打出牌的布局逻辑
    // 这部分可以根据实际需求来实现，比如水平排列或特定的展示方式
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
    // 更新手牌显示
    updateCards(handCards);
    setCardsVisible(showCardFronts);
}

void PlayerWidget::setPlayerName(const QString& name)
{
    m_nameLabel->setText(name);
}

void PlayerWidget::highlightTurn(bool isCurrentTurn)
{
    m_isCurrentTurn = isCurrentTurn;
    setHighlighted(isCurrentTurn);
    
    // 更新状态显示
    if (isCurrentTurn) {
        setPlayerStatus("轮到你出牌");
    } else {
        updatePlayerInfo(); // 恢复显示剩余牌数
    }
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
    // 加载默认头像
    QString defaultAvatarPath = ":/pic/res/default_avatar.png";
    if (!m_defaultAvatarPixmap.load(defaultAvatarPath)) {
        qWarning() << "Failed to load default avatar:" << defaultAvatarPath;
        // 创建一个默认的头像
        m_defaultAvatarPixmap = QPixmap(AVATAR_SIZE, AVATAR_SIZE);
        m_defaultAvatarPixmap.fill(Qt::gray);
    }
    
    // 加载默认背景
    QString defaultBackgroundPath = ":/pic/res/player_bg.png";
    if (!m_defaultBackgroundPixmap.load(defaultBackgroundPath)) {
        qWarning() << "Failed to load default background:" << defaultBackgroundPath;
        m_defaultBackgroundPixmap = QPixmap(); // 使用空背景，会fallback到纯色背景
    }
}

void PlayerWidget::setPlayerAvatar(const QString& avatarPath)
{
    if (avatarPath.isEmpty()) {
        setDefaultAvatar();
        return;
    }
    
    QPixmap newAvatar;
    if (newAvatar.load(avatarPath)) {
        m_avatarPixmap = newAvatar;
        m_avatarLabel->setPixmap(m_avatarPixmap.scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        qWarning() << "Failed to load avatar from path:" << avatarPath;
        setDefaultAvatar();
    }
}

void PlayerWidget::setDefaultAvatar()
{
    m_avatarPixmap = m_defaultAvatarPixmap;
    m_avatarLabel->setPixmap(m_avatarPixmap.scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

void PlayerWidget::setupButtons()
{
    if (!m_buttonLayout) {
        m_buttonLayout = new QHBoxLayout();
        m_buttonLayout->setContentsMargins(0, 5, 0, 5);
        m_buttonLayout->setSpacing(10);
    }
    
    if (!m_playButton) {
        m_playButton = new QPushButton("出牌", this);
        // 设置按钮样式
        QString buttonStyle = "QPushButton {"
                            "    background-color: #4CAF50;"
                            "    color: white;"
                            "    border: none;"
                            "    padding: 5px 15px;"
                            "    border-radius: 4px;"
                            "    font-size: 14px;"
                            "}"
                            "QPushButton:hover {"
                            "    background-color: #45a049;"
                            "}"
                            "QPushButton:disabled {"
                            "    background-color: #cccccc;"
                            "    color: #666666;"
                            "}";
        m_playButton->setStyleSheet(buttonStyle);
        m_playButton->setShortcut(Qt::Key_Return);  // 回车键出牌
        connect(m_playButton, &QPushButton::clicked, this, &PlayerWidget::playCardsRequested);
    }
    
    if (!m_skipButton) {
        m_skipButton = new QPushButton("跳过", this);
        m_skipButton->setStyleSheet(m_playButton->styleSheet());
        m_skipButton->setShortcut(Qt::Key_Space);   // 空格键跳过
        connect(m_skipButton, &QPushButton::clicked, this, &PlayerWidget::skipTurnRequested);
    }
    
    // 清空现有布局
    while (m_buttonLayout->count() > 0) {
        QLayoutItem* item = m_buttonLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
        }
        delete item;
    }
    
    // 重新添加按钮
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_playButton);
    m_buttonLayout->addWidget(m_skipButton);
    m_buttonLayout->addStretch();
    
    // 显示按钮
    m_playButton->show();
    m_skipButton->show();
    
    // 初始状态下禁用按钮
    updateButtonsState();
}

void PlayerWidget::updateButtonsState()
{
    if (m_playButton && m_skipButton) {
        bool hasSelectedCards = !getSelectedCards().isEmpty();
        m_playButton->setEnabled(m_isEnabled && hasSelectedCards);
        m_skipButton->setEnabled(m_isEnabled);
        
        // 确保按钮可见性
        m_playButton->setVisible(m_position == PlayerPosition::Bottom);
        m_skipButton->setVisible(m_position == PlayerPosition::Bottom);
    }
}

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

// 新增：停止所有动画的方法
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
        QRect targetGeometry = QRect(
            currentGeometry.center().x() - 5,
            currentGeometry.center().y() - 5,
            10, 10
        );
        scaleOut->setStartValue(currentGeometry);
        scaleOut->setEndValue(targetGeometry);

        // 动画完成后删除widget
        connect(fadeOut, &QPropertyAnimation::finished, [widget]() {
            widget->deleteLater();
            });

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
        QRect targetGeometry = QRect(
            widget->x(), widget->y(),
            CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT
        );
        QRect startGeometry = QRect(
            targetGeometry.center().x() - 5,
            targetGeometry.center().y() - 5,
            10, 10
        );
        scaleIn->setStartValue(startGeometry);
        scaleIn->setEndValue(targetGeometry);

        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        scaleIn->start(QAbstractAnimation::DeleteWhenStopped);
    }
}