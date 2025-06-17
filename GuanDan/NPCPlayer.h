#pragma once

//AI出牌的具体实现分为两步：找牌算法(findValidPlays)和选牌策略

#include "Player.h"
#include "Cardcombo.h"
#include <QMap> 

// 前向声明
class GD_Controller;

// NPCPlayer 继承自 Player，用于实现AI出牌逻辑
class NPCPlayer : public Player
{
    Q_OBJECT
public:
    // 构造函数: name 玩家名字, id 玩家ID
    NPCPlayer(const QString& name, int id);

    // 通过AI的出牌逻辑返回可出牌型
    QVector<Card> getBestPlay(const CardCombo::ComboInfo& currentTableCombo);

    // 重写玩家回合行为，实现AI自动出牌
    void autoPlay(GD_Controller* controller, const CardCombo::ComboInfo& currentTableCombo) override;

private:
    // 将辅助函数声明为静态(static)，因为它们不依赖于特定NPCPlayer实例的状态，只是对传入的参数进行处理
    
    // 辅助函数：按点数对手牌进行分类
    static QMap<Card::CardPoint, QVector<Card>> classifyHandByPoint(const QVector<Card>& hand);
    
    // 辅助函数：找出所有可能的炸弹
    static QVector<QVector<Card>> findBombs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, const QVector<Card>& wild_cards);
    
    // 辅助函数：找出所有可能的三条
    static QVector<QVector<Card>> findTriples(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, const QVector<Card>& wild_cards);

	// 辅助函数：找出所有可能的对子
    static QVector<QVector<Card>> findPairs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, const QVector<Card>& wild_cards);

	// 辅助函数：找出所有单牌
    static QVector<QVector<Card>> findSingles(const QMap<Card::CardPoint, QVector<Card>>& pointGroups);
    
    // 辅助函数：找出所有可能的顺子
    static QVector<QVector<Card>> findStraights(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, int requiredLength = 5);
    
    // 辅助函数：找出所有可能的连对 (DoubleSequence)
    static QVector<QVector<Card>> findDoubleSequences(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, int requiredLength = 3);
    
    // 辅助函数：找出所有可能的三带二 (TripleWithPair)
    static QVector<QVector<Card>> findTripleWithPairs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, const QVector<Card>& wild_cards);

	// 辅助函数：找出所有可能的钢板 (TripleSequence)
    static QVector<QVector<Card>> findTripleSequences(const QMap<Card::CardPoint, QVector<Card>>& pointGroups);

	// 核心算法函数：找出所有可能的合法出牌组合
    QVector<CardCombo::ComboInfo> findValidPlays(const QVector<Card>& hand, const CardCombo::ComboInfo& tableCombo);
};

