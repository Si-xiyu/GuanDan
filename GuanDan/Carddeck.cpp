#include "carddeck.h"

#include <QDebug>
#include <algorithm> // for std::shuffle
#include <random>    // for std::mt19937

CardDeck::CardDeck()
{
    initializeDecks();
    shuffle(); // 构造时默认洗牌
}

void CardDeck::initializeDecks()
{
    m_cards.clear(); // 清空牌库中现有的牌
    m_cards.reserve(108); // 为108张牌预分配内存空间，提高效率

    // 定义标准的四种花色 (不包括大小王使用的 Joker 花色)
    const QVector<Card::CardSuit> standardSuits = {
        Card::CardSuit::Diamond, // 方块
        Card::CardSuit::Club,    // 梅花
        Card::CardSuit::Heart,   // 红桃
        Card::CardSuit::Spade    // 黑桃
    };

    // 定义标准的点数 (从 2 到 A)
    const QVector<Card::CardPoint> standardPoints = {
        Card::CardPoint::Card_2, Card::CardPoint::Card_3, Card::CardPoint::Card_4,
        Card::CardPoint::Card_5, Card::CardPoint::Card_6, Card::CardPoint::Card_7,
        Card::CardPoint::Card_8, Card::CardPoint::Card_9, Card::CardPoint::Card_10,
        Card::CardPoint::Card_J, Card::CardPoint::Card_Q, Card::CardPoint::Card_K,
        Card::CardPoint::Card_A
    };

    // 循环创建两副牌
    for (int deckCount = 0; deckCount < 2; ++deckCount) {
        // 1. 添加普通牌 (2到A，四种花色)
        for (Card::CardSuit suit : standardSuits) {         // 遍历每一种标准花色
            for (Card::CardPoint point : standardPoints) {  // 遍历每一种标准点数
                // Card 构造函数 Card(CardPoint point, CardSuit suit, Player* owner = nullptr)
                // owner 默认为 nullptr，发牌时再设置
                m_cards.append(Card(point, suit));
            }
        }

        // 2. 添加大小王
        m_cards.append(Card(Card::CardPoint::Card_LJ, Card::CardSuit::Joker)); // 小王
        m_cards.append(Card(Card::CardPoint::Card_BJ, Card::CardSuit::Joker)); // 大王
    }

    qDebug() << "CardDeck 已初始化完毕，包含" << m_cards.size() << "张牌。";
}

// 洗牌逻辑
void CardDeck::shuffle()
{
    // 检查牌库是否为空
    if (m_cards.isEmpty()) {
        qWarning() << "CardDeck::shuffle: Deck is empty, cannot shuffle. Re-initializing.";
        initializeDecks(); // 重新初始化牌库
        if (m_cards.isEmpty()) return; // 如果初始化后仍然为空，直接返回(错误)
    }

    // 使用随机数生成器打乱牌库
    std::random_device rd; // 获取随机数种子
    std::mt19937 g(rd()); // 利用rd的随机数生成器
    std::shuffle(m_cards.begin(), m_cards.end(), g);
    qDebug() << "CardDeck shuffled.";
}

// 检查牌堆是否为空
bool CardDeck::isEmpty() const
{
    if (m_cards.size() == 0) {
        return true;
    }
    else {
        return false;
    }
}

// 重置牌库并洗牌
void CardDeck::resetDeck()
{
    initializeDecks();
    shuffle();
}

// 获取当前牌库中的所有牌
QVector<Card> CardDeck::getDeckCards() const
{
    return m_cards; // 返回当前牌库中的所有牌
}
