#include "Player.h"
#include "Cardcombo.h"

void Player::setName(QString name)
{
    m_name = name;
}

QString Player::getName() const
{
    return m_name;
}

void Player::setType(Type type)
{
    m_type = type;
}

void Player::setId(int Id)
{
    m_id = Id;
}

Player::Type Player::getType() const
{
    return m_type;
}

void Player::addCards(const QVector<Card>& cards)
{
    // 在手牌数组中加入cards
    m_handCards.append(cards);
    // 整理手牌
    std::sort(m_handCards.begin(), m_handCards.end());
    // 发送手牌更新信号
    emit cardsUpdated();
}

void Player::removeCards(const QVector<Card>& cards)
{
    for (const auto& card : cards) {
        m_handCards.removeAll(card);
    }
    // 由于原本就是有序的，不需要整理
    // 发出手牌更新信号
    emit cardsUpdated();
}

QVector<Card> Player::getHandCards() const
{
    return m_handCards;
}

void Player::setTeam(team* team)
{
    m_team = team;
}

team* Player::getTeam() const
{
    return m_team;
}

void Player::setReady(bool ready)
{
    m_isReady = ready;
}

bool Player::isReady() const
{
    return m_isReady;
}

bool Player::canPlayCards(const QVector<Card>& cards,
    int current_table_combo_type,
    int current_table_combo_level) const
{
    // 1. 调用 getAllPossibleValidPlays 获取所有合法出牌组合
    QVector<CardCombo::ComboInfo> valid_plays =
        CardCombo::getAllPossibleValidPlays(
            cards,
            const_cast<Player*>(this), // 当前玩家上下文
            current_table_combo_type,
            current_table_combo_level);

    // 2. 判断是否存在至少一个合法出牌组合
    return !valid_plays.isEmpty();
}


