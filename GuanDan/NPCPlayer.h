#pragma once
#include "Player.h"
#include "Cardcombo.h"

// NPCPlayer 继承自 Player，用于实现AI出牌逻辑
class NPCPlayer : public Player
{
    Q_OBJECT
public:
    // 构造函数: name 玩家名字, id 玩家ID
    NPCPlayer(const QString& name, int id);

    // 重写玩家回合行为，实现AI自动出牌
    void autoPlay(GD_Controller* controller, const CardCombo::ComboInfo& currentTableCombo) override;

private:
    // 递归辅助函数，用于找出给定手牌的所有可能子集
    void findSubsets(const QVector<Card>& hand, int index, QVector<Card>& currentSubset, 
                     QVector<QVector<Card>>& allSubsets);
};

