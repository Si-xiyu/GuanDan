#ifndef CARDCOMBO_H
#define CARDCOMBO_H

#include <QVector>
#include <QString>

#include "Card.h"     // 包含 Card 类定义

// 前向声明 Player 和 Team
class Player;
class Team;

// 定义牌型常量
namespace CardComboType {
    const int Invalid = -1;
    const int Single = 1;
    const int Pair = 2;
    const int Triple = 3;
    const int TripleWithPair = 4;
    const int Straight = 5;
    const int DoubleSequence = 6;
    const int TripleSequence = 7;
    const int Bomb = 8;
}

class CardCombo {
public:
    // 牌型信息结构体
    struct ComboInfo {
        int type = CardComboType::Invalid;
        int level = -1;
        QVector<Card> cards_in_combo;
        QVector<Card> original_cards;  // 存储构成该牌型的原始手牌（包括癞子牌）
        int wild_cards_used = 0;
        bool is_flush_straight_bomb = false;

        bool isValid() const { return type != CardComboType::Invalid; }
        QString getDescription() const; // 返回牌型描述
    };

    CardCombo() = delete; // 静态类，只使用方法而不实例化
    ~CardCombo() = delete; // 禁止析构

    // 评估一组确定的牌的牌型
    // 在getAllPossibleValidPlays函数中用于处理所有可能的组合
    static ComboInfo evaluateConcreteCombo(const QVector<Card>& concrete_cards,
        Player* current_player_context);

    // 枚举函数，获取所有可能的合法出牌组合QVector<ComboInfo>
    static QVector<ComboInfo> getAllPossibleValidPlays(
        const QVector<Card>& selected_cards,
        Player* current_player_context,
        int current_table_combo_type,
        int current_table_combo_level);

    // 处理在顺子中的顺序值，对A特殊处理
    static int getSequentialOrder(Card::CardPoint p, bool ace_as_high_in_straight);

    // 判断输入的点数是否可以组成顺子或类似的连续结构（如连对、钢板等）
    // 通过引用参数leading_point_for_level返回连续结构中的最大点数
    static bool checkConsecutive(const QVector<Card::CardPoint>& distinct_points_input,
        Card::CardPoint& leading_point_for_level);

    // 生成牌组的指纹Qstring，用于唯一标识牌组
    static QString generateComboFingerprint(const QVector<Card>& cards);


private:
    // 判断玩家现在的牌型是否可以打败当前桌面上的牌型
    static bool canBeat(const ComboInfo& play_combo,
        int current_table_combo_type,
        int current_table_combo_level);

    // 递归地处理癞子牌的各种替换方式，生成所有可能的具体合法牌组
    static void findCombinationsWithWildsRecursive(
        const QVector<Card>& current_concrete_cards,
        const QVector<Card>& original_wild_cards_remaining,
        const QVector<Card>& original_selection,
        Player* player_context,
        QVector<ComboInfo>& valid_combos_found,
        QMap<QString, bool>& visited_combo_fingerprints,
        int current_table_combo_type,
        int current_table_combo_level,
        int total_original_wild_cards_in_selection);
};

#endif // CARDCOMBO_H
