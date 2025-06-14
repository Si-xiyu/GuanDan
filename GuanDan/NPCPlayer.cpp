#include "NPCPlayer.h"
#include "Cardcombo.h"
#include "GD_Controller.h"
#include <algorithm>
#include <QTimer>
#include <QDebug>
#include <QSet>

// 构造函数
NPCPlayer::NPCPlayer(const QString& name, int id)
    : Player(name, id) {}

QVector<Card> NPCPlayer::getBestPlay(const CardCombo::ComboInfo& currentTableCombo)
{
    // 获取当前手牌
    QVector<Card> hand = getHandCards();

    // 如果手牌为空，直接返回空列表
    if (hand.isEmpty()) {
        qDebug() << "NPCPlayer::getBestPlay: Hand is empty, cannot play.";
        return {};
    }

    // 调用内部的 findValidPlays 方法来找出所有能打的牌
    QVector<CardCombo::ComboInfo> validPlays = findValidPlays(hand, currentTableCombo);

    // 如果找不到任何可以出的牌
    if (validPlays.isEmpty()) {
        qDebug() << "NPCPlayer::getBestPlay: No valid plays found.";

        // 如果是跟牌阶段，返回空列表是正确的（表示“要不起”）
        if (currentTableCombo.type != CardComboType::Invalid) {
            return {};
        }
        // 如果是自由出牌阶段,返回最小的一张单牌
        else {
            QVector<Card> sortedHand = hand;
            // Card类已重载<运算符，可以直接排序
            std::sort(sortedHand.begin(), sortedHand.end());
            if (!sortedHand.isEmpty()) {
                qDebug() << "NPCPlayer::getBestPlay: Forcing smallest single card play.";
                return { sortedHand.first() };
            }
            return {}; // 极端情况，手牌排序后还是空的
        }
    }

    // --- AI 决策核心：对所有可行的出牌组合进行排序，选出最优解 ---
    // 排序策略：
    // 1. 非炸弹 优先于 炸弹（避免轻易浪费炸弹）
    // 2. 牌力等级（level）低的 优先于 等级高的（先出小牌）
    // 3. 使用癞子（wild_cards_used）少的 优先于 多的（节省万能牌）
    // 4. 牌数（original_cards.size()）少的 优先于 多的（尽快打出手牌）
    std::sort(validPlays.begin(), validPlays.end(),
        [](const CardCombo::ComboInfo& a, const CardCombo::ComboInfo& b) {
            bool a_is_bomb = (a.type == CardComboType::Bomb);
            bool b_is_bomb = (b.type == CardComboType::Bomb);

            // 规则1：非炸弹优先
            if (a_is_bomb != b_is_bomb) {
                return !a_is_bomb; // 如果a不是炸弹而b是，a排在前面 (return true)
            }

            // 规则2：牌力等级低的优先
            if (a.level != b.level) {
                return a.level < b.level;
            }

            // 规则3：使用癞子少的优先
            if (a.wild_cards_used != b.wild_cards_used) {
                return a.wild_cards_used < b.wild_cards_used;
            }

            // 规则4：牌数少的优先
            if (a.original_cards.size() != b.original_cards.size()) {
                return a.original_cards.size() < b.original_cards.size();
            }

            // 如果所有条件都相同，保持原有顺序
            return false;
        });

    // 排序后，第一个元素就是最优选择
    CardCombo::ComboInfo bestPlay = validPlays.first();

    qDebug() << "NPCPlayer::getBestPlay: Found" << validPlays.size() << "valid plays. Best choice:" << bestPlay.getDescription();

    // 返回最优组合的原始卡牌（包含癞子）
    return bestPlay.original_cards;
}

// 辅助函数：按点数对手牌进行分类
QMap<Card::CardPoint, QVector<Card>> NPCPlayer::classifyHandByPoint(const QVector<Card>& hand) {
    QMap<Card::CardPoint, QVector<Card>> pointGroups;
    for (const Card& card : hand) {
        pointGroups[card.point()].append(card);
    }
    return pointGroups;
}

// 辅助函数：找出所有可能的炸弹
QVector<QVector<Card>> NPCPlayer::findBombs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups) {
    QVector<QVector<Card>> bombs;
    for (auto it = pointGroups.constBegin(); it != pointGroups.constEnd(); ++it) {
        if (it.value().size() >= 4) {
            // NOTE: 一个点数可以构成多种炸弹（如5张K可以出4张K或5张K）
            // 为简化，这里只找出所有牌构成的最大炸弹。
            bombs.append(it.value());
        }
    }
    return bombs;
}

// 辅助函数：找出所有可能的三条
QVector<QVector<Card>> NPCPlayer::findTriples(const QMap<Card::CardPoint, QVector<Card>>& pointGroups) {
    QVector<QVector<Card>> triples;
    for (auto it = pointGroups.constBegin(); it != pointGroups.constEnd(); ++it) {
        if (it.value().size() >= 3) {
            triples.append(QVector<Card>(it.value().begin(), it.value().begin() + 3));
        }
    }
    return triples;
}

// 辅助函数：找出所有可能的对子
QVector<QVector<Card>> NPCPlayer::findPairs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups) {
    QVector<QVector<Card>> pairs;
    for (auto it = pointGroups.constBegin(); it != pointGroups.constEnd(); ++it) {
        if (it.value().size() >= 2) {
            pairs.append(QVector<Card>(it.value().begin(), it.value().begin() + 2));
        }
    }
    return pairs;
}

// 辅助函数：找出所有单牌
QVector<QVector<Card>> NPCPlayer::findSingles(const QMap<Card::CardPoint, QVector<Card>>& pointGroups) {
    QVector<QVector<Card>> singles;
    for (auto it = pointGroups.constBegin(); it != pointGroups.constEnd(); ++it) {
        singles.append(QVector<Card>{it.value().first()});
    }
    return singles;
}

// 辅助函数：找出所有可能的顺子
QVector<QVector<Card>> NPCPlayer::findStraights(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, int requiredLength) {
    QVector<QVector<Card>> straights;
    QVector<Card::CardPoint> points = pointGroups.keys().toVector();
    std::sort(points.begin(), points.end());

    if (points.size() < requiredLength) return straights;

    for (int i = 0; i <= points.size() - requiredLength; ++i) {
        bool isConsecutive = true;
        // NOTE: 这个判断对A作为顺子一部分的情况处理不完善，但能处理大部分情况
        for (int j = 0; j < requiredLength - 1; ++j) {
            if (static_cast<int>(points[i + j + 1]) != static_cast<int>(points[i + j]) + 1) {
                isConsecutive = false;
                break;
            }
        }
        if (isConsecutive) {
            QVector<Card> straight;
            for (int j = 0; j < requiredLength; ++j) {
                straight.append(pointGroups[points[i + j]].first());
            }
            straights.append(straight);
        }
    }
    return straights;
}

// 辅助函数：找出所有可能的连对
QVector<QVector<Card>> NPCPlayer::findDoubleSequences(const QMap<Card::CardPoint, QVector<Card>>& pointGroups, int requiredLength) {
    QVector<QVector<Card>> sequences;
    QVector<Card::CardPoint> validPoints;
    for (auto it = pointGroups.constBegin(); it != pointGroups.constEnd(); ++it) {
        if (it.value().size() >= 2) {
            validPoints.append(it.key());
        }
    }
    std::sort(validPoints.begin(), validPoints.end());

    if (validPoints.size() < requiredLength) return sequences;

    for (int i = 0; i <= validPoints.size() - requiredLength; ++i) {
        bool isConsecutive = true;
        for (int j = 0; j < requiredLength - 1; ++j) {
            if (static_cast<int>(validPoints[i + j + 1]) != static_cast<int>(validPoints[i + j]) + 1) {
                isConsecutive = false;
                break;
            }
        }
        if (isConsecutive) {
            QVector<Card> sequence;
            for (int j = 0; j < requiredLength; ++j) {
                sequence.append(pointGroups[validPoints[i + j]][0]);
                sequence.append(pointGroups[validPoints[i + j]][1]);
            }
            sequences.append(sequence);
        }
    }
    return sequences;
}

// 辅助函数：找出所有可能的三带二
QVector<QVector<Card>> NPCPlayer::findTripleWithPairs(const QMap<Card::CardPoint, QVector<Card>>& pointGroups) {
    QVector<QVector<Card>> result;
    auto triples = findTriples(pointGroups);
    auto pairs = findPairs(pointGroups);

    if (triples.isEmpty() || pairs.isEmpty()) return result;

    for (const auto& triple : triples) {
        Card::CardPoint triplePoint = triple[0].point();
        for (const auto& pair : pairs) {
            if (pair[0].point() != triplePoint) {
                QVector<Card> combined = triple;
                combined.append(pair);
                result.append(combined);
            }
        }
    }
    return result;
}

// 辅助函数：找出所有可能的合法出牌组合
QVector<CardCombo::ComboInfo> NPCPlayer::findValidPlays(const QVector<Card>& hand, const CardCombo::ComboInfo& tableCombo) {
    QVector<CardCombo::ComboInfo> allValidPlays;
    auto pointGroups = classifyHandByPoint(hand);
    QVector<QVector<Card>> potentialPlays;

    if (tableCombo.type == CardComboType::Invalid) {
        potentialPlays.append(findSingles(pointGroups));
        potentialPlays.append(findPairs(pointGroups));
        potentialPlays.append(findTriples(pointGroups));
        potentialPlays.append(findStraights(pointGroups));
        potentialPlays.append(findDoubleSequences(pointGroups));
        potentialPlays.append(findTripleWithPairs(pointGroups));
        potentialPlays.append(findBombs(pointGroups));
    } else {
        // FIX: CardComboType 名称修正
        if (tableCombo.type == CardComboType::Single) {
            potentialPlays.append(findSingles(pointGroups));
        } else if (tableCombo.type == CardComboType::Pair) {
            potentialPlays.append(findPairs(pointGroups));
        } else if (tableCombo.type == CardComboType::Triple) {
            potentialPlays.append(findTriples(pointGroups));
                 } else if (tableCombo.type == CardComboType::Straight) {
             potentialPlays.append(findStraights(pointGroups, tableCombo.cards_in_combo.size()));
        } else if (tableCombo.type == CardComboType::DoubleSequence) {
            potentialPlays.append(findDoubleSequences(pointGroups, tableCombo.cards_in_combo.size() / 2));
        } else if (tableCombo.type == CardComboType::TripleWithPair) {
            potentialPlays.append(findTripleWithPairs(pointGroups));
        }
        // 任何情况下都可以出炸弹来压
        potentialPlays.append(findBombs(pointGroups));
    }

    QSet<QString> foundSignatures; // 用于防止重复添加完全相同的牌组
    for (const auto& play : potentialPlays) {
        // FIX: 调用正确的4参数版本的 getAllPossibleValidPlays
        QVector<CardCombo::ComboInfo> combos = CardCombo::getAllPossibleValidPlays(
            play, this, tableCombo.type, tableCombo.level);
        
        for(const auto& combo : combos) {
            if(combo.isValid()) {
                QString signature = CardCombo::generateComboFingerprint(combo.original_cards);
                if (!foundSignatures.contains(signature)) {
                    allValidPlays.append(combo);
                    foundSignatures.insert(signature);
                }
            }
        }
    }
    return allValidPlays;
}

// AI玩家自动行为：在回合开始时由控制器调用
void NPCPlayer::autoPlay(GD_Controller* controller, const CardCombo::ComboInfo& currentTableCombo) {
    // 延迟执行以模拟思考，并避免UI卡顿
    QTimer::singleShot(500, [this, controller, currentTableCombo]() {
        QVector<Card> hand = getHandCards();
        if (hand.isEmpty()) {
            if (controller && currentTableCombo.type != CardComboType::Invalid) {
                 controller->onPlayerPass(getID());
            }
            return;
        }

        QVector<CardCombo::ComboInfo> validPlays = findValidPlays(hand, currentTableCombo);

        if (validPlays.isEmpty()) {
            if (controller && currentTableCombo.type != CardComboType::Invalid) {
                controller->onPlayerPass(getID());
            } else if (controller) {
                // 如果是自己领出，但找不到任何牌（极端情况），打出最小的单张
                QVector<Card> sortedHand = hand;
                std::sort(sortedHand.begin(), sortedHand.end());
                controller->onPlayerPlay(getID(), {sortedHand.first()});
            }
            return;
        }

        // 策略排序
        std::sort(validPlays.begin(), validPlays.end(), 
            [](const CardCombo::ComboInfo& a, const CardCombo::ComboInfo& b) {
                bool a_is_bomb = (a.type == CardComboType::Bomb);
                bool b_is_bomb = (b.type == CardComboType::Bomb);
                if (a_is_bomb != b_is_bomb) return !a_is_bomb; // 非炸弹优先
                if (a.level != b.level) return a.level < b.level; // level低的优先
                if (a.wild_cards_used != b.wild_cards_used) return a.wild_cards_used < b.wild_cards_used; // 癞子少的优先
                return a.original_cards.size() < b.original_cards.size(); // 牌数少的优先
            });

        CardCombo::ComboInfo bestPlay = validPlays.first();
        controller->onPlayerPlay(getID(), bestPlay.original_cards);
    });
}
