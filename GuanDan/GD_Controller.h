// 这部分代码太逆天了，主播要崩溃了

#ifndef GD_CONTROLLER_H
#define GD_CONTROLLER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QSet> // 用于记录已pass的玩家

#include "card.h"
#include "player.h"
#include "team.h"
#include "levelstatus.h"
#include "cardcombo.h" // 包含 CardCombo::ComboInfo 和 CardComboType

// 前向声明UI类，如果Controller需要直接与之交互（通常通过信号槽）
class GameWindow;
class PlayerWidget;

class GD_Controller : public QObject
{
    Q_OBJECT

public:
    explicit GD_Controller(QObject* parent = nullptr);
    ~GD_Controller();

    // --- 游戏初始化与设置 ---
    void setupNewGame(const QVector<Player*>& players, const QVector<Team*>& teams); // 传入已创建的玩家和队伍
    void startGame(); // 开始整个游戏（第一局）

public slots:
    // --- 来自UI的玩家操作 ---
    // 当玩家在UI上选择了一组牌，并点击了“出牌”按钮
    void onPlayerAttemptPlay(int playerId, const QVector<Card>& cardsToPlay);
    // 当玩家点击了“过牌”按钮
    void onPlayerPass(int playerId);
    // 当玩家点击了“提示”按钮 (可选，如果实现提示功能)
    void onPlayerRequestHint(int playerId);
    // 当玩家完成了进贡/还贡操作
    void onPlayerTributeCardSelected(int tributingPlayerId, const Card& tributeCard);


signals:
    // --- 通知UI更新 ---
    void sigGameStarted();

	// 新一局开始信号
    void sigNewRoundStarted(int roundNumber);
    void sigCardsDealt(int playerId, const QVector<Card>& hand); // 通知玩家手牌已经发好
    void sigUpdatePlayerHand(int playerId, const QVector<Card>& newHand); // 手牌变化时更新

	// Round结束信号
    void sigRoundOver(const QString& summary, const QVector<int>& playerRanks); // 一局结束，附带总结和排名

    // Game结束信号
    void sigGameOver(int winningTeamId, const QString& winningTeamName, const QString& finalMessage); // 整个游戏结束

	// 更新桌面牌的显示(新Round和Game开始时)
    void sigClearTableCards(); // 清空桌面的牌（新一圈开始）
    void sigUpdateTableCards(const CardCombo::ComboInfo& lastPlayedCombo, const QString& playerName); // 更新桌面显示的牌

	// 更新全局状态
	void sigSetCurrentTurnPlayer(int playerId, const QString& playerName); // 轮到谁出牌
    void sigEnablePlayerControls(int playerId, bool canPlay, bool canPass); // 控制玩家操作按钮的可用性
    void sigUpdateTeamLevel(int teamId, Card::CardPoint newLevel); // 更新队伍的级别

    // 消息显示
	void sigShowPlayerMessage(int playerId, const QString& message, bool isError = false); // 给特定玩家显示消息
    void sigBroadcastMessage(const QString& message); // 广播消息给所有玩家

    // 请求进贡或还贡
    void sigAskForTribute(int tributingPlayerId, const QString& tributingPlayerName, int receivingPlayerId, const QString& receivingPlayerName, bool isReturningTribute); 
	void sigTributePhaseEnded(); // 进贡阶段结束

private:
    // --- 游戏状态成员 ---
    QMap<int, Player*> m_players; // 通过ID映射玩家指针
    QMap<int, Team*> m_teams;     // 通过ID映射队伍指针

    LevelStatus m_levelStatus;    // 级别和胜负状态管理器

    int m_currentPlayerId;                 // 当前轮到出牌的玩家ID
    CardCombo::ComboInfo m_currentTableCombo; // 当前桌面上最后一手合法的牌
    int m_circleLeaderId;                  // 本圈第一个出牌的玩家ID (即m_currentTableCombo的所有者)
    QSet<int> m_passedPlayersInCircle;     // 本圈已经选择“不出”的玩家ID
    QVector<int> m_roundFinishOrder;       // 按顺序记录本局完成出牌的玩家ID
    int m_activePlayersInRound;            // 本局还剩多少玩家没打完牌

    int m_currentRoundNumber;              // 当前是第几局

    // --游戏阶段管理--
    enum class GamePhase {
		NotStarted, // 游戏未开始
		Dealing, // 发牌阶段
		Playing, // 出牌阶段
        TributeInput,  // 等待玩家选择进贡的牌
        TributeProcess, // 处理进贡逻辑
        RoundOver, // 回合结束
        GameOver // 游戏结束
    };
    GamePhase m_currentPhase;

    // --进贡相关状态 (用于多步进贡/还贡)--
    struct TributeInfo {
        int fromPlayerId;
        int toPlayerId;
        Card card;
        bool isReturn; // 是进贡还是还贡
    };
    QVector<TributeInfo> m_pendingTributes; // 需要处理的进贡/还贡列表
    int m_currentTributeIndex;


    // --- 内部游戏流程方法 ---
	void startNewRound(); // 开始新的一局 
	void dealCardsToPlayers(); // 给玩家发牌 Y
    void determineFirstPlayerForRound(); // 决定新一局谁先出牌 Y
	void nextPlayerTurn(); // 轮到下一个玩家出牌 Y

    bool canPlayerPlay(int playerId, const QVector<Card>& cardsToPlay, CardCombo::ComboInfo& outPlayedCombo);
    bool processPlayerPlay(int playerId, const CardCombo::ComboInfo& playedCombo);
	// 玩家是否可以跳过
    void processPlayerPass(int playerId);

    void checkCircleEndAndNextAction(); // 检查一圈是否结束，以及后续动作 Y
    void checkRoundEndAndNextAction();  // 检查一局是否结束
    void processRoundResults();         // 处理一局结束后的计分、升级等
    QString generateRoundSummary() const;

    void startTributePhaseLogic();      // 开始进贡/还贡的逻辑设定
    void processNextTributeAction();    // 处理当前待办的进贡/还贡项
    void completeTribute(const TributeInfo& tribute); // 完成一次贡牌/还牌的转移


    // --- 辅助方法 ---
    Player* getPlayerById(int id) const; // 通过ID获取玩家指针
    Team* getTeamOfPlayer(int playerId) const;
    bool isPlayerInGame(int playerId) const; // 玩家是否还未出完牌
    QVector<int> getActivePlayerIdsSorted() const; // 获取当前还未出完牌的玩家ID, 按座位顺序
    bool allOtherActivePlayersPassed(int currentPlayerId) const;
};

#endif // GD_CONTROLLER_H
// --- END OF FILE gd_controller.h ---