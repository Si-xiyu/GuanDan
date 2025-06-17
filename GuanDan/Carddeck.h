#ifndef CARDDECK_H
#define CARDDECK_H

#include "Card.h" // 包含 Card 类的定义

#include <QVector>

class CardDeck
{
public:
    CardDeck(); // 构造函数，内部固定创建两副牌

    void shuffle();                     // 洗牌
    bool isEmpty() const;               // 检查牌堆是否为空
    void resetDeck();                   // 重置牌库并洗牌
    QVector<Card> getDeckCards() const; // 获取当前牌库中的所有牌

private:
    void initializeDecks();      // 初始化牌库

    QVector<Card> m_cards; // 存储牌的容器
};

#endif // CARDDECK_H
