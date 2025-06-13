#include "ShowCardWidget.h"
#include <QPainter>
#include <QResizeEvent>
#include <QDebug>
#include <algorithm>

ShowCardWidget::ShowCardWidget(QWidget* parent)
    : QWidget(parent)
    , m_cardFrame(nullptr)
{
    // 设置最小尺寸
    setMinimumSize(SHOW_CARD_MIN_WIDTH, SHOW_CARD_MIN_HEIGHT);
    
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // 创建卡牌框架，用于容纳卡牌
    m_cardFrame = new QFrame(this);
    m_cardFrame->setStyleSheet("QFrame { background-color: transparent; }");
    m_mainLayout->addWidget(m_cardFrame, 1); // 占主要空间
    
    // 设置背景色
    setStyleSheet("ShowCardWidget { background-color: rgba(0, 80, 0, 150); border-radius: 5px; }");
}

ShowCardWidget::~ShowCardWidget()
{
    clearDisplay();
}

void ShowCardWidget::updateDisplay(const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards)
{
    // 清空现有显示
    clearDisplay();
    
    // 保存当前牌型信息
    m_currentCombo = combo;
    m_originalCards = originalCards;
    
    // 如果是无效牌型，直接返回
    if (combo.type == CardComboType::Invalid) {
        return;
    }
    
    // 查找原始牌中的癞子
    QMap<Card, Card> wildToRealCardMap;
    for (const Card& card : originalCards) {
        // 如果这张牌是癞子
        if (card.isWildCard()) {
            // 查找这张癞子在combo中变成了什么牌
            for (const Card& comboCard : combo.cards_in_combo) {
                // 找到癞子变成的牌(排除combo中原本就存在的牌)
                bool foundInOriginal = false;
                for (const Card& origCard : originalCards) {
                    if (!origCard.isWildCard() && origCard == comboCard) {
                        foundInOriginal = true;
                        break;
                    }
                }
                
                if (!foundInOriginal) {
                    wildToRealCardMap[card] = comboCard;
                    break;
                }
            }
        }
    }
    
    // 添加combo中的牌
    for (const Card& card : combo.cards_in_combo) {
        CardWidget* cardWidget = createCardWidget(card);
        m_cardWidgets.append(cardWidget);
    }
    
    // 添加癞子并建立堆叠关系
    for (auto it = wildToRealCardMap.begin(); it != wildToRealCardMap.end(); ++it) {
        // 创建癞子牌控件
        CardWidget* wildCardWidget = createCardWidget(it.key());
        m_cardWidgets.append(wildCardWidget);
        
        // 查找这个癞子变成的牌的控件
        for (CardWidget* widget : m_cardWidgets) {
            if (widget->getCard() == it.value()) {
                // 建立癞子到变牌的映射关系
                m_wildToRepresentMap[wildCardWidget] = widget;
                break;
            }
        }
    }
    
    // 重新布局
    relayoutCards();
}

void ShowCardWidget::clearDisplay()
{
    // 清理卡片视图
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();
    m_wildToRepresentMap.clear();
    
    // 清空牌型信息
    m_currentCombo.type = CardComboType::Invalid;
    m_currentCombo.cards_in_combo.clear();
    m_originalCards.clear();
    
    update();
}

CardWidget* ShowCardWidget::createCardWidget(const Card& card)
{
    // 父窗口改为m_cardFrame
    CardWidget* cardWidget = new CardWidget(card, nullptr, m_cardFrame);
    cardWidget->setFrontSide(true);
    cardWidget->setEnabled(false); // 禁用交互
    cardWidget->setFixedSize(DefaultCardWidth, DefaultCardHeight);
    cardWidget->show();
    return cardWidget;
}

void ShowCardWidget::relayoutCards()
{
    if (m_cardWidgets.isEmpty()) {
        return;
    }
    
    // 对卡片按点数排序
    std::sort(m_cardWidgets.begin(), m_cardWidgets.end(), 
        [](CardWidget* a, CardWidget* b) {
            return a->getCard().point() < b->getCard().point();
        });
    
    // 计算卡片区域，现在使用m_cardFrame的矩形区域
    QRect cardsArea = m_cardFrame->rect();
    int cardCount = m_cardWidgets.size();
    
    // 计算总宽度
    int totalWidth = DefaultCardWidth + (cardCount - 1) * SHOW_CARD_OVERLAP_HORIZONTAL;
    
    // 计算起始位置，使牌组在m_cardFrame中居中显示
    int startX = (cardsArea.width() - totalWidth) / 2;
    int startY = (cardsArea.height() - DefaultCardHeight) / 2;
    
    // 为每张牌设置位置
    for (int i = 0; i < cardCount; ++i) {
        CardWidget* cardWidget = m_cardWidgets[i];
        
        // 默认堆叠索引为0
        int stackIndex = 0;
        
        // 检查是否为癞子牌，如果是则计算堆叠索引
        if (m_wildToRepresentMap.contains(cardWidget)) {
            stackIndex = 1;
        }
        
        // 计算位置
        int x = startX + i * SHOW_CARD_OVERLAP_HORIZONTAL;
        int y = startY - stackIndex * SHOW_CARD_OVERLAP_VERTICAL; // 癞子牌向上堆叠
        
        cardWidget->move(x, y);
    }
    
    // 更新Z顺序，确保堆叠效果正确
    updateCardZOrder();
}

QPoint ShowCardWidget::calculateCardPosition(int index, int stackIndex) const
{
    // 这个方法已经被新的布局逻辑替代，但保留以兼容旧代码
    int x = index * SHOW_CARD_OVERLAP_HORIZONTAL;
    int y = -stackIndex * SHOW_CARD_OVERLAP_VERTICAL; // 负值使癞子牌向上堆叠
    return QPoint(x, y);
}

void ShowCardWidget::updateCardZOrder()
{
    // 首先设置所有卡片的Z值为其索引
    for (int i = 0; i < m_cardWidgets.size(); ++i) {
        m_cardWidgets[i]->raise();
    }
    
    // 然后处理癞子牌的堆叠
    for (auto it = m_wildToRepresentMap.begin(); it != m_wildToRepresentMap.end(); ++it) {
        // 癞子牌应该在下面
        CardWidget* wildCard = it.key();
        CardWidget* representCard = it.value();
        wildCard->lower();
        // 变成的牌应该在上面
        representCard->raise();
    }
}

void ShowCardWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayoutCards();
}

void ShowCardWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    // 可以在这里添加额外的绘制，如背景效果等
}
