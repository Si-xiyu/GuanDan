#ifndef NPCPLAYER_H
#define NPCPLAYER_H

#include <QObject>
#include "Player.h"
#include "Cardcombo.h"
#include "Card.h"

class NPCplayer : public Player
{
    Q_OBJECT
public:
    explicit NPCplayer(QString name, int id, Team* team = nullptr, QObject* parent = nullptr);
    
    // AI决策相关方法
    QVector<Card> decideCardsToPlay(const CardCombo::ComboInfo& currentTable);  // 决定要出的牌
    bool shouldPass(const CardCombo::ComboInfo& currentTable);                  // 决定是否要跳过
    
    // 策略评估方法
    double evaluateHand() const;                           // 评估手牌强度
    double evaluateCombo(const QVector<Card>& cards) const;// 评估特定牌组合的价值
    
    // 牌型分析方法
    QVector<QVector<Card>> findAllPossibleCombos() const;  // 找出所有可能的牌型组合
    QVector<Card> findBestCounter(const CardCombo::ComboInfo& currentTable) const;  // 找出最佳应对牌型
    
    // AI难度设置
    enum class Difficulty { Easy, Normal, Hard };
    void setDifficulty(Difficulty level);
    Difficulty getDifficulty() const;

private:
    // 辅助决策方法
    bool isWinningHand(const QVector<Card>& cards) const;  // 判断是否为必胜手牌
    bool shouldKeepCombo(const QVector<Card>& combo) const;// 判断是否应该保留某个牌型
    
    // AI策略参数
    double m_aggressiveness;    // AI的激进程度 (0-1)
    double m_riskTolerance;     // AI的风险承受度 (0-1)
    Difficulty m_difficulty;    // AI难度等级
    
    // 游戏局势分析
    void analyzeGameState();    // 分析当前游戏状态
    void updateStrategy();      // 更新策略参数
    
signals:
    void thinkingStarted();     // AI开始思考信号
    void thinkingFinished();    // AI完成思考信号
};

#endif // NPCPLAYER_H