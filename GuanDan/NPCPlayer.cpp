#include "NPCPlayer.h"
#include "Cardcombo.h"
#include "GD_Controller.h"
#include <algorithm>
#include <QTimer>

// 构造函数
NPCPlayer::NPCPlayer(const QString& name, int id)
    : Player(name, id) {}

// 递归辅助函数，用于找出给定手牌的所有可能子集
void NPCPlayer::findSubsets(const QVector<Card>& hand, int index, QVector<Card>& currentSubset, 
                           QVector<QVector<Card>>& allSubsets)
{
    // 基本情况：已经处理完所有牌
    if (index >= hand.size()) {
        // 只添加非空子集
        if (!currentSubset.isEmpty()) {
            allSubsets.append(currentSubset);
        }
        return;
    }

    // 不选当前牌，直接递归到下一张
    findSubsets(hand, index + 1, currentSubset, allSubsets);

    // 选当前牌，然后递归到下一张
    currentSubset.append(hand[index]);
    findSubsets(hand, index + 1, currentSubset, allSubsets);
    currentSubset.removeLast(); // 回溯，移除最后添加的牌
}

// AI玩家自动行为：在回合开始时由控制器调用
void NPCPlayer::autoPlay(GD_Controller* controller, const CardCombo::ComboInfo& currentTableCombo)
{
    // 获取手牌
    QVector<Card> hand = getHandCards();
    
    // 如果手牌为空，直接过牌
    if (hand.isEmpty()) {
        int playerId = getID();
        QTimer::singleShot(500, [controller, playerId]() {
            controller->onPlayerPass(playerId);
        });
        return;
    }

    // 生成所有可能的手牌子集
    QVector<QVector<Card>> allSubsets;
    QVector<Card> currentSubset;
    findSubsets(hand, 0, currentSubset, allSubsets);
    
    // 限制子集数量，防止组合爆炸
    // 对子集按照牌数量排序，优先考虑较小的子集
    std::sort(allSubsets.begin(), allSubsets.end(), [](const QVector<Card>& a, const QVector<Card>& b) {
        return a.size() < b.size();
    });
    
    // 如果子集数量过多，只保留前1000个
    const int MAX_SUBSETS = 1000;
    if (allSubsets.size() > MAX_SUBSETS) {
        allSubsets.resize(MAX_SUBSETS);
    }

    // 收集所有可行的出牌方案
    QVector<CardCombo::ComboInfo> allPossiblePlays;
    
    // 遍历所有子集
    for (const auto& subset : allSubsets) {
        // 分析当前子集是否能构成合法牌型
        QVector<CardCombo::ComboInfo> possiblePlays = CardCombo::getAllPossibleValidPlays(
            subset, this, currentTableCombo.type, currentTableCombo.level);
        
        // 将所有可行方案添加到总列表中
        allPossiblePlays.append(possiblePlays);
    }
    
    // 如果没有可行方案，过牌
    if (allPossiblePlays.isEmpty()) {
        int playerId = getID();
        QTimer::singleShot(500, [controller, playerId]() {
            controller->onPlayerPass(playerId);
        });
        return;
    }
    
    // 对所有可行方案进行智能排序
    // 1. 非炸弹优先于炸弹
    // 2. 等级（level）低的优先
    // 3. 使用癞子少的优先
    // 4. 牌数少的优先
    std::sort(allPossiblePlays.begin(), allPossiblePlays.end(), 
        [](const CardCombo::ComboInfo& a, const CardCombo::ComboInfo& b) {
            bool a_is_bomb = (a.type == CardComboType::Bomb);
            bool b_is_bomb = (b.type == CardComboType::Bomb);
            if (a_is_bomb != b_is_bomb) return !a_is_bomb;
            if (a.level != b.level) return a.level < b.level;
            if (a.wild_cards_used != b.wild_cards_used) return a.wild_cards_used < b.wild_cards_used;
            return a.original_cards.size() < b.original_cards.size();
        });
    
    // 选择最优方案
    CardCombo::ComboInfo bestPlay = allPossiblePlays.first();
    QVector<Card> chosenCards = bestPlay.original_cards;
    
    // 执行出牌操作
    int playerId = getID();
    QTimer::singleShot(500, [controller, playerId, chosenCards]() {
        controller->onPlayerPlay(playerId, chosenCards);
    });
}
