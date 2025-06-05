#include <QDebug>
#include <QSet>
#include <QtAlgorithms>
#include <algorithm>
#include <functional>

#include "Cardcombo.h"


// CardCombo::ComboInfo中的getDescription方法实现(调试函数)
QString CardCombo::ComboInfo::getDescription() const {
    if (!isValid()) return QStringLiteral("无效牌型");

    // 生成牌型描述
    QString desc;
    switch (type) {
    case CardComboType::Single: desc = QStringLiteral("单张"); break;
    case CardComboType::Pair: desc = QStringLiteral("对子"); break;
    case CardComboType::Triple: desc = QStringLiteral("三条"); break;
    case CardComboType::TripleWithPair: desc = QStringLiteral("三带二"); break;
    case CardComboType::Straight: desc = QStringLiteral("顺子"); break;
    case CardComboType::DoubleSequence: desc = QStringLiteral("三连对"); break;
    case CardComboType::TripleSequence: desc = QStringLiteral("钢板"); break;
    case CardComboType::Bomb:
        if (is_flush_straight_bomb) desc = QStringLiteral("同花顺");
        else desc = QStringLiteral("炸弹");
        break;
    default: desc = QStringLiteral("未知牌型 (%1)").arg(static_cast<int>(type)); break;
    }

    // 添加等级和癞子信息
    desc += QStringLiteral(" (等级: %1").arg(level);
    if (wild_cards_used > 0) {
        desc += QStringLiteral(", %1癞子").arg(wild_cards_used);
    }
    desc += QStringLiteral("): ");

    QVector<Card> sorted_display_cards = cards_in_combo;
    // Card 类有 operator< 用于排序，可以使用 std::sort 或 qSort
    std::sort(sorted_display_cards.begin(), sorted_display_cards.end());

    // 生成卡牌字符，如6H 7H 8H 9H 10H
    for (const auto& card : sorted_display_cards) {
        QString suit_char;
        switch (card.suit()) {
        case Card::CardSuit::Heart: suit_char = "H"; break;
        case Card::CardSuit::Spade: suit_char = "S"; break;
        case Card::CardSuit::Diamond: suit_char = "D"; break;
        case Card::CardSuit::Club: suit_char = "C"; break;
        case Card::CardSuit::Joker: suit_char = ""; break;
        default: suit_char = "?"; break;
        }
        desc += card.PointToString() + suit_char + " ";
    }
    return desc.trimmed(); // 移除多余空白字符
}

// 生成牌组的指纹，用于唯一标识牌组
QString CardCombo::generateComboFingerprint(const QVector<Card>& cards) { // 返回 QString, 参数 QVector
    QString fingerprint; // 指纹字符串
    QVector<Card> sorted_cards = cards;
    std::sort(sorted_cards.begin(), sorted_cards.end());

    // 取点数和花色生成指纹
    for (const auto& card : sorted_cards) {
        fingerprint += QString::number(static_cast<int>(card.point())) + "-" +
            QString::number(static_cast<int>(card.suit())) + ";";
    }
    return fingerprint;
}

int CardCombo::getSequentialOrder(Card::CardPoint p, bool ace_as_high_in_straight)
{
    switch (p) {
        // 对A的特殊处理: 如果bool为真，则A作为最大点数；如果为假，则A作为最小点数
    case Card::CardPoint::Card_A: return ace_as_high_in_straight ? 14 : 1;
    case Card::CardPoint::Card_2: return 2;
    case Card::CardPoint::Card_3: return 3;
    case Card::CardPoint::Card_4: return 4;
    case Card::CardPoint::Card_5: return 5;
    case Card::CardPoint::Card_6: return 6;
    case Card::CardPoint::Card_7: return 7;
    case Card::CardPoint::Card_8: return 8;
    case Card::CardPoint::Card_9: return 9;
    case Card::CardPoint::Card_10: return 10;
    case Card::CardPoint::Card_J: return 11;
    case Card::CardPoint::Card_Q: return 12;
    case Card::CardPoint::Card_K: return 13;
    case Card::CardPoint::Card_LJ: return 0;
    case Card::CardPoint::Card_BJ: return 0;
    default: return 0;
    }
}

// 判断输入的点数是否可以组成顺子或类似的连续结构（如连对、钢板等）
bool CardCombo::checkConsecutive(const QVector<Card::CardPoint>& distinct_points_input, Card::CardPoint& leading_point_for_level)
{ // distinct_points_input表示不同的点数
    // leading_point_for_level表示连续结构的最高点

    // 如果不同的点数为空，直接返回false
    if (distinct_points_input.empty()) return false;

    // 检查是否包含小王或大王，若有则不能组成任何连续牌型
    for (Card::CardPoint p : distinct_points_input) {
        if (p == Card::CardPoint::Card_LJ || p == Card::CardPoint::Card_BJ) {
            return false;
        }
    }

    // 对点数进行排序
    QVector<Card::CardPoint> points = distinct_points_input;

    std::sort(points.begin(), points.end(), [&](Card::CardPoint a, Card::CardPoint b) {
        // 默认将 A 作为最小点数
        return getSequentialOrder(a, false) < getSequentialOrder(b, false);
        });
    // 或者 qSort(points.begin(), points.end(), ...);

    // 1. A作为最小点数时的连续性
    bool consecutive_A_low = true;
    for (qsizetype i = 0; i < points.size() - 1; ++i) { // qsizetype for QVector
        // 检查相邻点数的顺序差是否为1 (A作为最小点数)
        if (getSequentialOrder(points[i + 1], false) - getSequentialOrder(points[i], false) != 1) {
            consecutive_A_low = false;
            break;
        }
    }
    // 如果点数连续且A作为最小点数，则返回true (显然A不能又最大又最小)
    if (consecutive_A_low) {
        leading_point_for_level = points.back();
        return true;
    }

    // 2. A作为最大点数时的连续性
    // 如果A最小时没有返回true，判断牌组中是否有A
    bool has_A = false;
    for (auto it = points.begin(); it != points.end(); ++it) {
        if (*it == Card::CardPoint::Card_A) { // 使用迭代器，否则Qt容器可能出错
            has_A = true;
            break;
        }
    }

    // 如果有A，则检查A作为最大点数的连续性
    if (has_A) {
        std::sort(points.begin(), points.end(), [&](Card::CardPoint a, Card::CardPoint b) {
            return getSequentialOrder(a, true) < getSequentialOrder(b, true);
            });
        bool consecutive_A_high = true;
        for (qsizetype i = 0; i < points.size() - 1; ++i) {
            if (getSequentialOrder(points[i + 1], true) - getSequentialOrder(points[i], true) != 1) {
                consecutive_A_high = false;
                break;
            }
        }

        // 如果A作为最大点数时点数连续,则返回true
        if (consecutive_A_high) {
            leading_point_for_level = Card::CardPoint::Card_A;
            return true;
        }
    }
    // 如果以上两种情况都不满足，则返回false
    return false;
}


// 匿名命名空间，给evaluateConcreteCombo提供辅助函数
namespace {

    // 获取点数的比较值
    int get_point_comparison_value(Card::CardPoint p, Player* ctx, Card::CardSuit suit_for_card_obj = Card::CardSuit::Spade)
    { // 只获得点数的比较值，取花色相同即可
        if (!ctx) {
            qWarning() << "get_point_comparison_value: Null player context!";
            return -1;
        }
        // 通过构造临时卡牌对象来得到比较值
        return Card(p, suit_for_card_obj, ctx).getComparisonValue();
    }

    int get_point_comparison_value(Card& card)
    {
        // 通过卡片本身得到比较值
        return card.getComparisonValue();
    }

    // 评估同点数牌型组合
    CardCombo::ComboInfo tryEvaluateSamePointCombos(const QVector<Card>& cards_with_context,
        const QMap<Card::CardPoint, int>& point_counts, // 记录每种点数的牌的数量
        Player* player_context,
        // 计算炸弹等级
        const std::function<int(int, Card::CardPoint, Card::CardSuit, bool, bool)>& get_bomb_level_func)
    {
        // 如果点数计数的大小不为1，说明不是同点数牌型
        CardCombo::ComboInfo info;
        if (point_counts.size() != 1) return info; // info默认为非法

        // 初始化牌型信息
        info.cards_in_combo = cards_with_context; // 记录当前牌组
        int num_total_cards = cards_with_context.size();
        Card::CardPoint combo_point = cards_with_context[0].point();
        Card::CardSuit representative_suit = cards_with_context[0].suit();

        // 判断牌型
        // 单牌
        if (num_total_cards == 1) {
            info.type = CardComboType::Single;
            info.level = get_point_comparison_value(combo_point, player_context, representative_suit);
        }
        // 对子
        else if (num_total_cards == 2) {
            info.type = CardComboType::Pair;
            info.level = get_point_comparison_value(combo_point, player_context, representative_suit);
        }
        // 三条
        else if (num_total_cards == 3) {
            info.type = CardComboType::Triple;
            info.level = get_point_comparison_value(combo_point, player_context, representative_suit);
        }
        // 炸弹
        else if (num_total_cards >= 4) {
            info.type = CardComboType::Bomb;
            info.level = get_bomb_level_func(num_total_cards, combo_point, representative_suit, false, false);
        }
        return info;
    }

    // 判断输入的牌组是否可以组成顺子或类似的连续结构
    CardCombo::ComboInfo tryEvaluateSequenceCombos(const QVector<Card>& cards_with_context,
        const QMap<Card::CardPoint, int>& point_counts,
        const QVector<Card::CardPoint>& distinct_points_vec,
        Player* player_context,
        const std::function<int(int, Card::CardPoint, Card::CardSuit, bool, bool)>& get_bomb_level_func) {
        // 1. 初始化牌型信息
        CardCombo::ComboInfo info;
        info.cards_in_combo = cards_with_context;
        int num_total_cards = cards_with_context.size();

        // 2. 判断大小王
        for (const auto& card : cards_with_context) {
            if (card.suit() == Card::CardSuit::Joker) {
                return info; // 默认非法
            }
        }

        // 3. 判断点数是否连续
        Card::CardPoint leading_point_for_seq_level = Card::CardPoint::Card_2; // 随便给一个初始化的最大点
        bool is_points_consecutive_structure = CardCombo::checkConsecutive(distinct_points_vec, leading_point_for_seq_level);
        // 如果连续，将最高值赋给leading_point_for_seq_level
        if (!is_points_consecutive_structure) return info; // 默认非法

        // 4. 计算顺序结构的等级和花色
        Card::CardSuit seq_level_card_suit = cards_with_context[0].suit(); // 取第一个牌的花色作为顺子的代表花色
        // 计算顺序结构的等级
        int sequence_final_level = get_point_comparison_value(leading_point_for_seq_level, player_context, seq_level_card_suit);

        // 5. 判断具体牌型
        // 顺子或同花顺
        if (num_total_cards == 5 && distinct_points_vec.size() == 5) {
            bool is_flush = true;
            Card::CardSuit first_suit = cards_with_context[0].suit();
            // 检查是否为同花顺
            for (qsizetype i = 1; i < cards_with_context.size(); ++i) {
                if (cards_with_context[i].suit() != first_suit) {
                    is_flush = false;
                    break;
                }
            }
            // 如果是同花顺，设置牌型为特殊炸弹
            if (is_flush) {
                info.type = CardComboType::Bomb;
                info.is_flush_straight_bomb = true;
                info.level = get_bomb_level_func(5, leading_point_for_seq_level, first_suit, true, false);
            }
            else {
                info.type = CardComboType::Straight;
                info.level = sequence_final_level;
            }
        }
        // 连对
        else if (num_total_cards == 6 && distinct_points_vec.size() == 3) {
            bool all_pairs = true;
            // 检查每种点数是否都是2张
            for (auto it = point_counts.constBegin(); it != point_counts.constEnd(); ++it) {
                if (it.value() != 2) { all_pairs = false; break; }
            }
            if (all_pairs) {
                info.type = CardComboType::DoubleSequence;
                info.level = sequence_final_level;
            }
        }
        // 钢板
        else if (num_total_cards == 6 && distinct_points_vec.size() == 2) {
            bool all_triples = true;
            // 检查每种点数是否都是3张
            for (auto it = point_counts.constBegin(); it != point_counts.constEnd(); ++it) {
                if (it.value() != 3) { all_triples = false; break; }
            }
            if (all_triples) {
                info.type = CardComboType::TripleSequence;
                info.level = sequence_final_level;
            }
        }
        return info;
    }

    // 判断三带二牌型
    CardCombo::ComboInfo tryEvaluateWithKickerCombos(const QVector<Card>& cards_with_context,
        const QMap<Card::CardPoint, int>& point_counts, Player* player_context) { // QMap
        // 1. 初始化牌型
        CardCombo::ComboInfo info;
        info.cards_in_combo = cards_with_context;
        int num_total_cards = cards_with_context.size();
        int num_distinct_points = point_counts.size();

        // 2. 判断三带二条件
        // 总牌数为5，且有2种不同点数
        if (num_total_cards == 5 && num_distinct_points == 2) {
            Card::CardPoint triple_pt = Card::CardPoint::Card_2;
            Card::CardSuit triple_suit = Card::CardSuit::Spade;
            bool found_triple = false, found_pair = false;

            // 寻找三条
            for (const auto& card : cards_with_context) {
                if (point_counts.value(card.point()) == 3) {
                    triple_suit = card.suit();
                    break;
                }
            }

            // 寻找对子
            for (auto it = point_counts.constBegin(); it != point_counts.constEnd(); ++it) {
                if (it.value() == 3) { triple_pt = it.key(); found_triple = true; }
                else if (it.value() == 2) { found_pair = true; }
            }
            if (found_triple && found_pair) {
                info.type = CardComboType::TripleWithPair;
                // 用三条来计算三带二的等级
                info.level = get_point_comparison_value(triple_pt, player_context, triple_suit);
            }
        }
        return info;
    }

    // 判断天王炸
    CardCombo::ComboInfo tryEvaluateSpecialBombs(const QVector<Card>& cards_with_context,
        const QMap<Card::CardPoint, int>& point_counts, Player* player_context,
        const std::function<int(int, Card::CardPoint, Card::CardSuit, bool, bool)>& get_bomb_level_func) {
        // 初始化牌型信息
        CardCombo::ComboInfo info;
        info.cards_in_combo = cards_with_context;
        int num_total_cards = cards_with_context.size();

        // 判断是否为天王炸
        if (num_total_cards == 4 && point_counts.size() == 2 &&
            point_counts.contains(Card::CardPoint::Card_LJ) && point_counts.value(Card::CardPoint::Card_LJ) == 2 &&
            point_counts.contains(Card::CardPoint::Card_BJ) && point_counts.value(Card::CardPoint::Card_BJ) == 2) {
            info.type = CardComboType::Bomb;
            info.level = get_bomb_level_func(4, Card::CardPoint::Card_BJ, Card::CardSuit::Joker, false, true);
        }
        return info;
    }

} // end namespace

// 评估确定的牌组能否组成有效牌型
CardCombo::ComboInfo CardCombo::evaluateConcreteCombo(const QVector<Card>& concrete_cards, Player* current_player_context) {
    if (concrete_cards.empty() || !current_player_context) {
        return ComboInfo();
    } // 空牌组返回非法

    QVector<Card> cards_with_context = concrete_cards;
    // 确保每张牌都有玩家上下文(即使该逻辑在发牌模块Deck中已经处理过)
    for (auto& card : cards_with_context) {
        if (!card.getOwner() || card.getOwner() != current_player_context) {
            card.setOwner(current_player_context);
        }
    }

    // 利用QMap统计点数分布
    QMap<Card::CardPoint, int> point_counts;
    for (const auto& card : cards_with_context) {
        point_counts[card.point()]++;
    }

    // 获得所有不同的点数
    QVector<Card::CardPoint> distinct_points_vec;
    distinct_points_vec.reserve(point_counts.size());  // 预先分配内存
    for (auto it = point_counts.constBegin(); it != point_counts.constEnd(); ++it) {
        distinct_points_vec.push_back(it.key()); // 添加键
    }

    // 定义炸弹等级计算函数
    auto get_bomb_level_lambda =
        [&](int num_cards_in_bomb, Card::CardPoint representative_point, Card::CardSuit representative_suit, bool is_flush_straight, bool is_king_bomb) -> int {
        int type_priority = 0;
        // 天王炸最高级
        if (is_king_bomb) type_priority = 300000;
        // 同花顺炸弹
        else if (is_flush_straight) type_priority = 200000;
        // 普通炸弹
        else type_priority = 100000;

        int count_factor = num_cards_in_bomb * 1000; // 计算炸弹的牌数点数
        // 基础牌等级
        int base_card_level = get_point_comparison_value(representative_point, current_player_context, representative_suit);
        if (base_card_level < 0) base_card_level = 0;
        return type_priority + count_factor + base_card_level; // 利用不同数位来实现优先级判断
        };

    // 按顺序调用匿名空间中的函数
    ComboInfo result_info;

    // 评估天王炸
    result_info = tryEvaluateSpecialBombs(cards_with_context, point_counts, current_player_context, get_bomb_level_lambda);
    if (result_info.isValid()) return result_info;

    // 评估同点数牌型
    result_info = tryEvaluateSamePointCombos(cards_with_context, point_counts, current_player_context, get_bomb_level_lambda);
    if (result_info.isValid()) return result_info;

    // 评估顺子或类似的连续结构
    result_info = tryEvaluateSequenceCombos(cards_with_context, point_counts, distinct_points_vec, current_player_context, get_bomb_level_lambda);
    if (result_info.isValid()) return result_info;

    // 评估三带二
    result_info = tryEvaluateWithKickerCombos(cards_with_context, point_counts, current_player_context);
    if (result_info.isValid()) return result_info;

    return ComboInfo(); // 返回评估结果
}

// 递归地处理癞子牌的各种替换方式，生成所有可能的具体合法牌组
void CardCombo::findCombinationsWithWildsRecursive(
    const QVector<Card>& current_concrete_cards, //确定的牌组
    const QVector<Card>& original_wild_cards_remaining, // 当前的癞子牌数组
    Player* player_context, // 当前玩家
    QVector<ComboInfo>& valid_combos_found, // 存储找到的合法牌组
    QMap<QString, bool>& visited_combo_fingerprints, // 已经生成过的牌组指纹
    int current_table_combo_type, // 当前桌面牌型
    int current_table_combo_level, // 当前桌面牌型等级
    int total_original_wild_cards_in_selection) // 原始癞子牌的总数
{
    // 1. 基例： 如果没有剩余的癞子牌，那么就完成了全部替换
    if (original_wild_cards_remaining.empty()) {
        QVector<Card> final_cards = current_concrete_cards;
        // 评估确定的牌组
        ComboInfo combo = evaluateConcreteCombo(final_cards, player_context);

        // 如果生成的牌组是合法的且没有指纹
        if (combo.isValid()) {
            QString fingerprint = generateComboFingerprint(combo.cards_in_combo);
            if (!visited_combo_fingerprints.contains(fingerprint)) {
                // 如果能大过上家
                if (canBeat(combo, current_table_combo_type, current_table_combo_level)) {
                    combo.wild_cards_used = total_original_wild_cards_in_selection;
                    valid_combos_found.push_back(combo); // 加入到合法牌组数组
                }
                // 将指纹添加到已访问的集合中
                visited_combo_fingerprints.insert(fingerprint, true);
            }
        }
        return; // 结束递归
    }

    // 2. 递归部分
    QVector<Card> next_wild_cards_remaining = original_wild_cards_remaining;
    next_wild_cards_remaining.removeFirst(); // 移除第一个癞子牌

    // 试将癞子牌替换为每种可能的具体牌 (点数和花色)
    for (int p_val_int = static_cast<int>(Card::CardPoint::Card_2); p_val_int <= static_cast<int>(Card::CardPoint::Card_A); ++p_val_int) {
        Card::CardPoint point_to_try = static_cast<Card::CardPoint>(p_val_int);
        if (point_to_try == Card::CardPoint::Card_LJ || point_to_try == Card::CardPoint::Card_BJ) {
            continue;
        }
        for (int s_val_int = static_cast<int>(Card::CardSuit::Diamond); s_val_int <= static_cast<int>(Card::CardSuit::Spade); ++s_val_int) {
            Card::CardSuit suit_to_try = static_cast<Card::CardSuit>(s_val_int);
            QVector<Card> next_concrete_cards = current_concrete_cards;
            next_concrete_cards.push_back(Card(point_to_try, suit_to_try, player_context));
            findCombinationsWithWildsRecursive(next_concrete_cards, next_wild_cards_remaining,
                player_context, valid_combos_found, visited_combo_fingerprints,
                current_table_combo_type, current_table_combo_level,
                total_original_wild_cards_in_selection);
        }
    }
}

// 核心函数：得到所有合法牌型组合
QVector<CardCombo::ComboInfo> CardCombo::getAllPossibleValidPlays( // Return QVector
    const QVector<Card>& selected_cards, // 选中的手牌
    Player* current_player_context, // 玩家上下文
    int current_table_combo_type, // 当前桌面牌型类型
    int current_table_combo_level) // 当前桌面牌型等级
{
    QVector<ComboInfo> all_valid_plays; // 存储所有合法牌型组合的数组
    if (selected_cards.empty() || !current_player_context) {
        return all_valid_plays;
    }

    // 1. 分离普通牌和癞子牌
    QVector<Card> initial_concrete_cards; // 初始普通牌
    QVector<Card> initial_wild_cards_to_substitute; // 初始癞子牌

    for (const auto& orig_card : selected_cards) {
        Card card = orig_card;
        // 确保每张牌都有玩家上下文,增加安全性(若测试时性能过低可考虑移除类似代码)
        if (!card.getOwner()) card.setOwner(current_player_context);
        if (card.isWildCard()) {
            initial_wild_cards_to_substitute.push_back(card);
        }
        else {
            initial_concrete_cards.push_back(card);
        }
    }

    QMap<QString, bool> visited_fingerprints; // 用于存储已经生成过的牌组指纹
    int total_wilds_in_selection = initial_wild_cards_to_substitute.size();

    // 2. 如果没有癞子牌，直接评估当前的具体牌组
    if (initial_wild_cards_to_substitute.empty()) {
        QVector<Card> temp_concrete = initial_concrete_cards; // Use QVector for consistency
        ComboInfo combo = evaluateConcreteCombo(temp_concrete, current_player_context);
        if (combo.isValid() && canBeat(combo, current_table_combo_type, current_table_combo_level)) {
            all_valid_plays.push_back(combo);
        }
    }
    // 3. 如果有癞子牌，调用findCombinationsWithWildsRecursive函数
    else {
        findCombinationsWithWildsRecursive(initial_concrete_cards,
            initial_wild_cards_to_substitute,
            current_player_context,
            all_valid_plays,
            visited_fingerprints,
            current_table_combo_type,
            current_table_combo_level,
            total_wilds_in_selection);
    }
    // 返回all_valid_plays

    // 对所有合法牌型组合进行排序 (方便展示)
    std::sort(all_valid_plays.begin(), all_valid_plays.end(), [](const ComboInfo& a, const ComboInfo& b) {
        bool a_is_bomb = (a.type == CardComboType::Bomb);
        bool b_is_bomb = (b.type == CardComboType::Bomb);
        if (a_is_bomb != b_is_bomb) return a_is_bomb;
        if (a.level != b.level) return a.level > b.level;
        if (a.wild_cards_used != b.wild_cards_used) return a.wild_cards_used < b.wild_cards_used;
        return false;
        });

    return all_valid_plays; // 返回所有合法牌型组合
}

// 辅助函数，判断传入的ComboInfo是否可以大过上家
bool CardCombo::canBeat(const CardCombo::ComboInfo& play_combo,
    int current_table_combo_type,
    int current_table_combo_level)
{
    // 检查特殊情况
    if (!play_combo.isValid()) return false;
    if (current_table_combo_type == CardComboType::Invalid) return true;

    // 炸弹逻辑
    if (play_combo.type == CardComboType::Bomb) {
        // 如果场上是炸弹，拼点
        if (current_table_combo_type == CardComboType::Bomb) {
            return play_combo.level > current_table_combo_level;
        }
        // 不是炸弹，炸弹就可以出
        else {
            return true;
        }
    }

    // 常规牌型逻辑
    return (play_combo.type == current_table_combo_type &&
        play_combo.level > current_table_combo_level);
}
