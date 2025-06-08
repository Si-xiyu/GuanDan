#pragma once
#include "Player.h"
#include "Cardcombo.h"

// NPCPlayer 继承自 Player，用于实现AI出牌逻辑
class NPCPlayer : public Player
{
public:
    // 构造函数: name 玩家名字, id 玩家ID
    NPCPlayer(const QString& name, int id);

    // 根据当前桌面牌型选择要出牌的卡牌列表
    // 如果返回空列表，则表示选择过牌
    QVector<Card> choosePlay(const CardCombo::ComboInfo& currentTableCombo) const;
};

