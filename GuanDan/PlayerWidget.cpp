#include "PlayerWidget.h"
#include "CardWidget.h" 
#include <QPainter>
#include <QDebug>
#include <algorithm> // std::sort
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPropertyAnimation>
#include <QtMath>
#include <QDir>
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
}

void PlayerWidget::updateCards(const QVector<Card>& cards)
{
    // 清理现有卡片
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_cardStacks.clear();
    
    // 创建新的卡片视图
    for (const Card& card : cards) {
        CardWidget* cardWidget = createCardWidget(card);
        m_cardWidgets.append(cardWidget);
        
        // 按点数分组
        m_cardStacks[card.point()].append(cardWidget);
    }
    
    // 对卡片进行排序
    sortCards();
    
    // 重新布局
    relayoutCards();
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
    // 计算卡片区域
    QRect cardsArea = rect().adjusted(CARDS_MARGIN, 2 * NAME_LABEL_HEIGHT + 10,
                                    -CARDS_MARGIN, -CARDS_MARGIN);
    
    // 计算每组牌的位置
    int currentX = cardsArea.left();
    int baseY = cardsArea.top();
    
    // 计算总宽度，确保居中显示
    int totalWidth = 0;
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        if (!it.value().isEmpty()) {
            totalWidth += CARD_WIDGET_WIDTH;
            if (it != m_cardStacks.begin()) {
                totalWidth -= CARD_OVERLAP_HORIZONTAL;
            }
        }
    }
    
    // 计算起始X坐标，使卡片居中显示
    currentX = cardsArea.left() + (cardsArea.width() - totalWidth) / 2;
    
    // 按点数顺序布局卡片
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        QVector<CardWidget*>& stack = it.value();
        if (stack.isEmpty()) continue;
        
        // 计算这组牌的垂直堆叠
        for (int i = 0; i < stack.size(); ++i) {
            CardWidget* card = stack[i];
            
            // 确保卡片大小正确
            card->setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            card->setRotation(0); // 确保卡片没有旋转
            
            // 计算位置
            int x = currentX;
            int y = baseY + i * CARD_OVERLAP_VERTICAL;
            
            // 如果不是最上面的牌，稍微往下移动一点
            if (i < stack.size() - 1) {
                y += 5; // 添加一点垂直偏移，使堆叠效果更明显
            }
            
            // 创建动画移动到新位置
            QPropertyAnimation* animation = new QPropertyAnimation(card, "pos");
            animation->setDuration(200);
            animation->setStartValue(card->pos());
            animation->setEndValue(QPoint(x, y));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
            
            // 确保选中的牌显示在最上面
            if (card->isSelected()) {
                card->raise();
            }
        }
        
        // 移动到下一组牌的位置
        currentX += CARD_WIDGET_WIDTH - CARD_OVERLAP_HORIZONTAL;
    }
    
    // 更新所有卡片的Z顺序，确保选中的牌在最上面
    for (CardWidget* card : m_cardWidgets) {
        if (card->isSelected()) {
            card->raise();
        }
    }
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
    // 对每个堆叠中的牌进行排序
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        QVector<CardWidget*>& stack = it.value();
        std::sort(stack.begin(), stack.end(), [](CardWidget* a, CardWidget* b) {
            return a->getCard() < b->getCard();
        });
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
        m_statusLabel->setText(QString("剩余牌数：%1").arg(m_player->getHandCards().size()));
        qDebug() << "更新玩家信息:" << m_player->getName() 
                 << "剩余牌数:" << m_player->getHandCards().size();
    }
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
    // 首先将所有卡片的Z值重置
    for (CardWidget* card : m_cardWidgets) {
        card->stackUnder(nullptr);
    }
    
    // 然后按照堆叠顺序设置Z值
    for (auto it = m_cardStacks.begin(); it != m_cardStacks.end(); ++it) {
        QVector<CardWidget*>& stack = it.value();
        for (int i = 0; i < stack.size(); ++i) {
            if (i > 0) {
                stack[i]->stackUnder(stack[i-1]);
            }
            if (stack[i]->isSelected()) {
                stack[i]->raise();
            }
        }
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
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setContentsMargins(0, 5, 0, 5);
    m_buttonLayout->setSpacing(10);
    
    m_playButton = new QPushButton("出牌", this);
    m_skipButton = new QPushButton("跳过", this);
    
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
    m_skipButton->setStyleSheet(buttonStyle);
    
    // 添加快捷键
    m_playButton->setShortcut(Qt::Key_Return);  // 回车键出牌
    m_skipButton->setShortcut(Qt::Key_Space);   // 空格键跳过
    
    // 连接信号
    connect(m_playButton, &QPushButton::clicked, this, &PlayerWidget::playCardsRequested);
    connect(m_skipButton, &QPushButton::clicked, this, &PlayerWidget::skipTurnRequested);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_playButton);
    m_buttonLayout->addWidget(m_skipButton);
    m_buttonLayout->addStretch();
    
    // 初始状态下禁用按钮
    updateButtonsState();
}

void PlayerWidget::updateButtonsState()
{
    if (m_playButton && m_skipButton) {
        bool hasSelectedCards = !getSelectedCards().isEmpty();
        m_playButton->setEnabled(m_isEnabled && hasSelectedCards);
        m_skipButton->setEnabled(m_isEnabled);
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