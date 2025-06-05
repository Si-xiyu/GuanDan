#include "Card.h"
#include "Player.h"
#include "Team.h"

#include <QDebug>

// 用于返回绝对点数大小
int Card::getComparisonValue() const
{
    CardPoint currentLevel = getCurrentOwnerLevelRank(); // 使用内部辅助函数
    // 1. Big Joker (Card_BJ)
    if (m_point == CardPoint::Card_BJ) {
        return 16; // Highest
    }
    // 2. Little Joker (Card_LJ)
    if (m_point == CardPoint::Card_LJ) {
        return 15;
    }

    // 3. CurrentLevel (级牌)
    if (m_point == currentLevel) {
        return 14;
    }

    // 4. Regular cards (其他牌)
    // The order is A, K, Q, J, 10, 9, 8, 7, 6, 5, 4, 3, 2
    switch (m_point) {
    case CardPoint::Card_A:  return 13;
    case CardPoint::Card_K:  return 12;
    case CardPoint::Card_Q:  return 11;
    case CardPoint::Card_J:  return 10;
    case CardPoint::Card_10: return 9;
    case CardPoint::Card_9:  return 8;
    case CardPoint::Card_8:  return 7;
    case CardPoint::Card_7:  return 6;
    case CardPoint::Card_6:  return 5;
    case CardPoint::Card_5:  return 4;
    case CardPoint::Card_4:  return 3;
    case CardPoint::Card_3:  return 2;
    case CardPoint::Card_2:  return 1;
    default: return -1;
    }
}

// 判断是否为癞子牌
bool Card::isWildCard() const
{
    if (!m_owner || !m_owner->getTeam()) {
        return false;
    } // 保证牌是有对象的
    CardPoint currentLevel = m_owner->getTeam()->getCurrentLevelRank(); // 通过Team类方法得到级数
    return (m_point == currentLevel && m_suit == CardSuit::Heart);
}

Card::Card() {}

Card::Card(CardPoint point, CardSuit suit, Player* owner)
    : m_point(point), m_suit(suit), m_owner(owner) {
}

void Card::setPoint(CardPoint point)
{
    m_point = point;
}

void Card::setSuit(CardSuit suit)
{
    m_suit = suit;
}

void Card::setOwner(Player* owner)
{
    m_owner = owner;
}

Player* Card::getOwner() const
{
    return m_owner;
}

// 获取卡牌的花色文件名
QString Card::SuitToString() const
{
    switch (m_suit) {
    case CardSuit::Heart: return "Hearts";
    case CardSuit::Spade: return "Spades";
    case CardSuit::Diamond: return "Diamonds";
    case CardSuit::Club: return "Clubs";
    case CardSuit::Joker: return "Joker";
    }
}

// 获取卡片的点数文件名
QString Card::PointToString() const
{
    switch (m_point) {
    case CardPoint::Card_2: return "2";
    case CardPoint::Card_3: return "3";
    case CardPoint::Card_4: return "4";
    case CardPoint::Card_5: return "5";
    case CardPoint::Card_6: return "6";
    case CardPoint::Card_7: return "7";
    case CardPoint::Card_8: return "8";
    case CardPoint::Card_9: return "9";
    case CardPoint::Card_10: return "10";
    case CardPoint::Card_A: return "ACE";
    case CardPoint::Card_J: return "J";
    case CardPoint::Card_Q: return "Q";
    case CardPoint::Card_K: return "K";
    case CardPoint::Card_LJ: return "2";
    case CardPoint::Card_BJ: return "1";
    }
}

QString Card::getImageFilename() const
{
    return SuitToString() + "_" + PointToString();
}

Card::CardPoint Card::getCurrentOwnerLevelRank() const
{
    // 返回目前所属队伍的级数
    if (m_owner && m_owner->getTeam()) {
        return m_owner->getTeam()->getCurrentLevelRank();
    } // 返回Team类中的级数

    qWarning() << "Card::getCurrentOwnerLevelRank: Card has no owner or owner has no team. "
        << "Returning Card_2 as fallback level.";
    return Card::CardPoint::Card_2; // 回退默认值
}

Card::CardSuit Card::suit() const
{
    return m_suit;
}

Card::CardPoint Card::point() const
{
    return m_point;
}

bool operator==(const Card& card1, const Card& card2)
{
    return (card1.point() == card2.point()) &&
        (card1.suit() == card2.suit());
}

bool operator!=(const Card& card1, const Card& card2)
{
	return !(card1 == card2);
}

bool operator<(const Card& card1, const Card& card2)
{
    if (card1.getComparisonValue() != card2.getComparisonValue()) {
        return card1.getComparisonValue() < card2.getComparisonValue();
    }
    else {
        return card1.m_suit < card2.m_suit;
    }
}

bool operator>(const Card& card1, const Card& card2)
{
    if (card1.getComparisonValue() != card2.getComparisonValue()) {
        return card1.getComparisonValue() > card2.getComparisonValue();
    }
    else {
        return card1.m_suit > card2.m_suit;
    }
}
