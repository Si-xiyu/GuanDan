#ifndef CARD_H
#define CARD_H

#include <QString>

class Player; // 前向定义，使用Player和Team的指针
class team;

class Card
{
    friend bool operator==(const Card& card1, const Card& card2);
    friend bool operator<(const Card& card1, const Card& card2);
    friend bool operator>(const Card& card1, const Card& card2);

public:
    //花色
    enum CardSuit {
        Diamond,   //方块
        Club,   //梅花
        Heart,   //红桃
        Spade,   //黑桃
        Joker   //双王
    };

    enum CardPoint {
        Card_2 = 2,
        Card_3,   //第一轮中3最小，2为级数
        Card_4,
        Card_5,
        Card_6,
        Card_7,
        Card_8,
        Card_9,
        Card_10,
        Card_J,
        Card_Q,
        Card_K,
        Card_A,
        Card_LJ,   //Little Joker
        Card_BJ    //Big Joker
    };

    int getComparisonValue() const; // 辅助函数，通过级牌得到比较值大小
    bool isWildCard() const; // 判断是否为癞子牌

    Card();
    Card(CardPoint point, CardSuit suit, Player* owner = nullptr);

    // 读写花色与点数的函数
    void setPoint(CardPoint point);
    void setSuit(CardSuit suit);
    CardPoint point() const;
    CardSuit suit() const;

    // 读写卡片所属对象
    void setOwner(Player* owner);
    Player* getOwner() const;

    // 获取字符--用于载入对应图片
    QString SuitToString() const;
    QString PointToString() const;
    QString getImageFilename() const;

    //Card类数据成员为点数、花色、卡牌所有者
private:
    CardPoint m_point;
    CardSuit m_suit;
    Player* m_owner = nullptr; // 指向拥有这张牌的玩家

    CardPoint getCurrentOwnerLevelRank() const; // 内部调用辅助函数
};

#endif // CARD_H
