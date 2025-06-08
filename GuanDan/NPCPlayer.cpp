#include "NPCPlayer.h"
#include "Cardcombo.h"
#include <algorithm>

// 构造函数
NPCPlayer::NPCPlayer(const QString& name, int id)
    : Player(name, id) {}

// AI出牌逻辑
QVector<Card> NPCPlayer::choosePlay(const CardCombo::ComboInfo& currentTableCombo) const
{
    // 获取手牌
    QVector<Card> hand = getHandCards();

    // 1. 如果桌面没有牌，出最小的单牌
    if (currentTableCombo.type == CardComboType::Invalid) {
        if (!hand.isEmpty()) {
            // 排序后最小的单牌是手牌中的第一张
            std::sort(hand.begin(), hand.end());
            return QVector<Card>{ hand.first() };
        }
        return {};
    }
    // 2. 桌面有牌，获取所有可以打过桌面牌型的合法组合
    QVector<CardCombo::ComboInfo> possible = CardCombo::getAllPossibleValidPlays(
        hand, const_cast<NPCPlayer*>(this), currentTableCombo.type, currentTableCombo.level);
    if (possible.isEmpty()) {
        // 没有牌可出，过牌
        return {};
    }
    // 优先寻找单张出牌
    QVector<CardCombo::ComboInfo> singles;
    for (const auto& combo : possible) {
        if (combo.type == CardComboType::Single) {
            singles.append(combo);
        }
    }
    if (!singles.isEmpty()) {
        // 按点数升序选择最小单牌
        std::sort(singles.begin(), singles.end(), [](auto& a, auto& b) {
            return a.cards_in_combo.first().point() < b.cards_in_combo.first().point();
        });
        return singles.first().cards_in_combo;
    }
    // 否则选择第一个合法组合
    return possible.first().cards_in_combo;
}
