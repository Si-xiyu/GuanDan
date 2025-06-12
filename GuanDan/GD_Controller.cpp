// 这部分代码太逆天了，主播要崩溃了

#include "GD_Controller.h"
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>
#include <QTextStream>
#include <QTimer>

#include "carddeck.h"
#include "WildCardDialog.h"
#include "NPCPlayer.h"

GD_Controller::GD_Controller(QObject* parent)
    : QObject(parent)
    , m_currentPlayerId(-1)
    , m_circleLeaderId(-1)
    , m_activePlayersInRound(0)
    , m_currentRoundNumber(0)
    , m_currentPhase(GamePhase::NotStarted)
    , m_currentTributeIndex(0)
{
    // 初始化当前桌面牌型为空
    m_currentTableCombo.type = CardComboType::Invalid;
    m_currentTableCombo.cards_in_combo.clear();
}

GD_Controller::~GD_Controller()
{
    // 析构函数，资源清理由外部管理
}


void GD_Controller::setupNewGame(const QVector<Player*>& players, const QVector<Team*>& teams)
{
    // 清理之前的数据
    m_players.clear();
    m_teams.clear();
    m_passedPlayersInCircle.clear();
    m_roundFinishOrder.clear();
    m_pendingTributes.clear();

    // 验证玩家和队伍数量
    if (players.size() != 4 || teams.size() != 2) {
        qDebug() << "错误：掼蛋需要4个玩家和2个队伍";
        return;
    }

    // 设置玩家映射并验证玩家所属队伍
    for (Player* player : players) {
        if (!player) continue;
        if (!player->getTeam()) {
            qDebug() << "错误：玩家" << player->getName() << "没有所属队伍";
            return;
        }
        m_players[player->getID()] = player;
    }

    // 设置队伍映射并验证每个队伍的玩家数量
    for (Team* team : teams) {
        if (!team) continue;
        if (team->getPlayers().size() != 2) {
            qDebug() << "错误：每个队伍必须有2个玩家";
            return;
        }
        m_teams[team->getId()] = team;
    }

    // 初始化等级状态
    m_levelStatus.initializeGameLevels(*teams[0], *teams[1]);

    qDebug() << "游戏设置完成，玩家数量:" << m_players.size() << "，队伍数量:" << m_teams.size();
}

void GD_Controller::startGame()
{
    if (m_players.size() != 4 || m_teams.size() != 2) {
        qDebug() << "错误：需要4个玩家和2个队伍才能开始游戏";
        return;
    }

    // 只设置和游戏整体相关的状态
    m_currentRoundNumber = 1;

    emit sigGameStarted();
    emit sigBroadcastMessage("掼蛋游戏开始！");

    // 通过状态机启动第一轮
    startNewRound();
}

// ==================== 玩家操作槽函数 ====================

// 处理玩家出牌操作(总方法)
void GD_Controller::onPlayerPlay(int playerId, const QVector<Card>& cardsToPlay)
{
    qDebug() << "GD_Controller::onPlayerPlay: 玩家" << playerId << "尝试出牌，牌数:" << cardsToPlay.size();
    QString errorMsg;
    if (!canPerformAction(playerId, errorMsg)) {
        emit sigShowPlayerMessage(playerId, errorMsg, true);
        return;
    }

    m_lastPlayedCards = cardsToPlay;

    CardCombo::ComboInfo playedCombo;
    if (!PlayerPlay(playerId, cardsToPlay, playedCombo)) {
        emit sigShowPlayerMessage(playerId, "出牌不符合规则", true);
        qDebug() << "GD_Controller::onPlayerPlay: 玩家" << playerId << "出牌不符合规则 "
            << "当前桌面牌型为" << m_currentTableCombo.type
            << ",当前牌型等级为" << m_currentTableCombo.level;
        return;
    }

    // 核心修改：只执行出牌，然后转到下一位
    executePlay(playerId, playedCombo);
}

// 处理玩家过牌操作(总方法)
void GD_Controller::onPlayerPass(int playerId)
{
    QString errorMsg;
    if (!canPerformAction(playerId, errorMsg)) {
        emit sigShowPlayerMessage(playerId, errorMsg, true);
        return;
    }

    if (m_currentTableCombo.type == CardComboType::Invalid) {
        emit sigShowPlayerMessage(playerId, "您是第一个出牌，不能过牌", true);
        return;
    }

    // 只执行过牌，然后转到下一位
    executePass(playerId);
}

// 实现提示功能
void GD_Controller::onPlayerRequestHint(int playerId)
{
    if (m_currentPhase != GamePhase::Playing || playerId != m_currentPlayerId) {
        return;
    }

    Player* player = getPlayerById(playerId);
    if (!player) return;

    // 获取所有可能的出牌组合
    QVector<CardCombo::ComboInfo> possiblePlays = CardCombo::getAllPossibleValidPlays(
        player->getHandCards(),  // 玩家当前的所有手牌
        player,                  // 当前玩家
        m_currentTableCombo.type,  // 当前桌面牌型
        m_currentTableCombo.level  // 当前桌面牌型等级
    );

    if (possiblePlays.isEmpty()) { // 可出牌为空
        emit sigShowPlayerMessage(playerId, "没有找到可以出的牌，建议过牌", false);
        return;
    }

	// 随机选择一个建议的出牌组合
    int randomIndex = QRandomGenerator::global()->bounded(possiblePlays.size());
    CardCombo::ComboInfo suggestion = possiblePlays[randomIndex];
    QString hintMessage = QString("建议出牌：%1").arg(suggestion.getDescription());
    emit sigShowPlayerMessage(playerId, hintMessage, false);
}

void GD_Controller::onPlayerTributeCardSelected(int tributingPlayerId, const Card& tributeCard)
{
    if (m_currentTributeIndex >= m_pendingTributes.size()) {
        return;
    }

    const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
    if (tributingPlayerId != currentTribute.fromPlayerId) {
        return;
    }

    Player* fromPlayer = getPlayerById(currentTribute.fromPlayerId);
    Player* toPlayer = getPlayerById(currentTribute.toPlayerId);
    if (!fromPlayer || !toPlayer) {
        return;
    }

    bool isValid = false;
    QString errorMessage;

    if (currentTribute.isReturn) {
        // 还贡规则检查
        Team* fromTeam = getTeamOfPlayer(currentTribute.fromPlayerId);
        Team* toTeam = getTeamOfPlayer(currentTribute.toPlayerId);
        bool isTeammate = (fromTeam == toTeam);

        if (isTeammate && tributeCard.point() > Card::Card_10) {
            errorMessage = "还贡给队友的牌必须是10或以下的牌！";
        }
        else {
            isValid = true;
        }
    }
    else {
        // 进贡规则检查：必须是最大的牌
        QVector<Card> handCards = fromPlayer->getHandCards();
        bool isLargest = true;
        for (const Card& card : handCards) {
            if (card > tributeCard) {
                isLargest = false;
                break;
            }
        }

        if (!isLargest) {
            errorMessage = "进贡必须选择手牌中最大的牌！";
        }
        else {
            isValid = true;
        }
    }

    if (isValid) {
        // 执行进贡/还贡
        TributeInfo tribute = currentTribute;
        tribute.card = tributeCard;
        
        // 更改牌的所有者
        tribute.card.setOwner(getPlayerById(toPlayer->getID()));
        
        // 转移牌
        QVector<Card> cards;
        cards.append(tribute.card);
        fromPlayer->removeCards(cards);
        toPlayer->addCards(cards);

        // 更新UI
        emit sigUpdatePlayerHand(tribute.fromPlayerId, fromPlayer->getHandCards());
        emit sigUpdatePlayerHand(tribute.toPlayerId, toPlayer->getHandCards());

        QString actionName = tribute.isReturn ? "还贡" : "进贡";
        QString cardDesc = QString("%1%2").arg(tribute.card.SuitToString()).arg(tribute.card.PointToString());
        emit sigBroadcastMessage(QString("%1 向 %2 %3：%4")
            .arg(fromPlayer->getName())
            .arg(toPlayer->getName())
            .arg(actionName)
            .arg(cardDesc));

        m_currentTributeIndex++;
        processNextTributeAction();
    }
    else {
        // 显示错误消息并让玩家重新选择
        emit sigShowPlayerMessage(tributingPlayerId, errorMessage, true);
    }
}

// ==================== 内部游戏流程方法 ====================

void GD_Controller::startNewRound()
{
    qDebug() << "GD_Controller::startNewRound - 准备开始新一局，局数: " << m_currentRoundNumber;
    
    // 初始化记牌器数据
    initializeCardCounts();
    
    // 唯一的职责：启动发牌状态
    enterState(GamePhase::Dealing);
}

void GD_Controller::dealCardsToPlayers()
{
    qDebug() << "GD_Controller::dealCardsToPlayers - 开始发牌";

    // 创建牌组并自动初始化（deck构造函数中会创建两副牌并洗牌）
    CardDeck deck;
    QVector<Card> allCards = deck.getDeckCards(); // 得到乱序的牌组（注意，是无所有者的牌组！）

    // 检查牌组大小是否足够
    const int cardsPerPlayer = 4; // 测试
    //const int cardsPerPlayer = 27;
    const int totalPlayers = 4;
    if (allCards.size() < cardsPerPlayer * totalPlayers) {
        qWarning() << "错误：牌组大小不足，无法发牌";
        return;
    }

    // 直接从m_players获取所有玩家ID
    QVector<int> playerIds = m_players.keys().toVector();
    std::sort(playerIds.begin(), playerIds.end()); // 确保ID按顺序排列

    // 给四个玩家发牌
    for (int i = 0; i < playerIds.size() && i < totalPlayers; ++i) {
        QVector<Card> playerCards; // 添加到每个玩家的手牌数组
        for (int j = 0; j < cardsPerPlayer; ++j) {
            playerCards.append(allCards[i * cardsPerPlayer + j]);
        }
        
        // 获取玩家对象
        Player* player = getPlayerById(playerIds[i]);

        // 给playerCards找主人
        for (Card& card : playerCards) {
            card.setOwner(player); // 设置牌的所有者ID
        }
        
        // 如果玩家存在，则清空原有手牌并添加新牌
        if (player) {
            player->clearHandCards();  // 使用专门的清空方法，确保完全清空
            player->addCards(playerCards);
            emit sigCardsDealt(playerIds[i], playerCards); // 发送发牌完成信号
            qDebug() << "发牌完成 - 玩家:" << player->getName() << "牌数:" << playerCards.size();
        }
        else {
            qDebug() << "错误：玩家ID" << playerIds[i] << "不存在，无法发牌";
        }
    }
}

void GD_Controller::determineFirstPlayerForRound()
{
    qDebug() << "GD_Controller::determineFirstPlayerForRound() - 开始确定首个出牌玩家";
    qDebug() << "当前局数:" << m_currentRoundNumber;
    qDebug() << "上局排名数量:" << m_lastRoundFinishOrder.size();

    // 第一局：玩家0先出
    if (m_currentRoundNumber == 1) {
        m_currentPlayerId = 0;
        m_circleLeaderId = m_currentPlayerId;
        qDebug() << "第一局强制设置玩家ID:" << m_currentPlayerId << "作为首个出牌玩家";
        return;
    }

    // 非第一局：使用上一局最后一名作为首出玩家
    if (!m_lastRoundFinishOrder.isEmpty()) {
        m_currentPlayerId = m_lastRoundFinishOrder.last();
        m_circleLeaderId = m_currentPlayerId;
        qDebug() << "非第一局，选择上一局最后一名玩家ID:" << m_currentPlayerId << "作为首个出牌玩家";
    } else {
        // 如果没有上一局排名信息，使用默认玩家
        qWarning() << "错误：非第一局但没有上一局的排名信息，使用默认玩家0";
        m_currentPlayerId = 0;
        m_circleLeaderId = m_currentPlayerId;
    }
}

// 清空当前桌面牌型和过牌玩家列表，并发送清空桌面牌型的信号
void GD_Controller::resetTableCombo()
{
    m_currentTableCombo.type = CardComboType::Invalid;
    m_currentTableCombo.cards_in_combo.clear();
}

// 判断玩家是否可以出牌，如果可以出牌的话处理为具体牌型，并返回出牌的牌型信息 (WildCardDialog的调用！)
bool GD_Controller::PlayerPlay(int playerId, const QVector<Card>& cardsToPlay, CardCombo::ComboInfo& outPlayedCombo)
{
    Player* player = getPlayerById(playerId);

    // 验证玩家是否拥有这些牌
    for (const Card& card : cardsToPlay) {
        if (!player->getHandCards().contains(card)) {
			qDebug() << "玩家" << player->getName() << "没有手牌" << card.PointToString();
            return false;
        }
    }

    // 获取当前级牌
    Card::CardPoint currentLevel = m_levelStatus.getTeamPlayingLevel(getTeamOfPlayer(playerId)->getId());
    // 使用Player的canPlayCards方法验证牌型
    if (player->canPlayCards(cardsToPlay, m_currentTableCombo)) {

		// 可以出牌，则outPlayedCombo数组为玩家选中的牌的合法牌型 (处理癞子牌的情况)
        QVector<CardCombo::ComboInfo> possibleCombos = CardCombo::getAllPossibleValidPlays(
            cardsToPlay,           // 玩家选择要打出的这些牌
            player,
            m_currentTableCombo.type,
            m_currentTableCombo.level);

		// 如果只有一个可能的组合，则直接使用它
        if (possibleCombos.size() == 1) 
        {
			outPlayedCombo = possibleCombos[0];
            return true;
        }

		// 如果有多个可能的组合，则需要WildCardDialog处理
		if (possibleCombos.size() > 1)
		{
            // 多种可能组合处理：AI直接取第一，玩家仅在包含癞子时弹窗，否则也取第一
            if (player->getType() == Player::AI) {
                outPlayedCombo = possibleCombos.first();
                return true;
            }
            bool hasWild = false;
            for (const Card& card : cardsToPlay) {
                if (card.isWildCard()) { hasWild = true; break; }
            }
            if (!hasWild) {
                outPlayedCombo = possibleCombos.first();
                return true;
            }
            // 包含癞子，弹框让玩家选择具体牌型
            qDebug() << "GD_Controller::PlayerPlay：WildCardDialog被调用";
            WildCardDialog dialog(possibleCombos, nullptr);
            if (dialog.exec() == QDialog::Accepted && dialog.hasValidSelection()) {
                outPlayedCombo = dialog.getSelectedCombo();
                return true;
            }
            return false;
		}
    }
    // 不能出牌，返回false
    else
    {
        qDebug() << "当前场上牌型为 " << m_currentTableCombo.type << " ，当前牌型等级为 " << m_currentTableCombo.level;
        qDebug() << "GD_Controller::PlayerPlay： 玩家" << player->getName() << "选择的牌不符合出牌规则";
        return false;
	}
	qDebug() << "GD_Controller::PlayerPlay： 错误错误错误！！！";
    return false;
}

// 玩家选择手牌后出牌处理函数
void GD_Controller::processPlayerPlay(int playerId, const CardCombo::ComboInfo& playedCombo)
{
    // 如果出牌不合法
    if (playedCombo.type == CardComboType::Invalid) {
        return;
    }
    // 从玩家手牌中移除玩家选中的原始卡牌
    Player* player = getPlayerById(playerId);
    player->removeCards(m_lastPlayedCards);
    
    // 更新记牌器数据
    updateCardCounts(m_lastPlayedCards);
    
    m_lastPlayedCards.clear();

    // 更新桌面牌型
    m_currentTableCombo = playedCombo;
    m_circleLeaderId = playerId;
    m_passedPlayersInCircle.clear(); // 清空本圈已过牌的玩家

    // 通知UI更新
    emit sigUpdatePlayerHand(playerId, player->getHandCards());
    emit sigUpdateTableCards(playerId, playedCombo, m_lastPlayedCards);

    QString message = QString("%1 出牌：%2")
        .arg(player->getName())
        .arg(playedCombo.getDescription());
    emit sigBroadcastMessage(message);
}

// 玩家选择过牌处理函数
void GD_Controller::processPlayerPass(int playerId)
{
	// 在过牌玩家列表中添加玩家ID
    Player* player = getPlayerById(playerId);
    if (!player) {
		qDebug() << "GD_Controller::processPlayerPass: Player with ID" << playerId << "not found!";
        return;
    }

    m_passedPlayersInCircle.insert(playerId);

    QString message = QString("%1 选择不出").arg(player->getName());
    emit sigBroadcastMessage(message);
}

// 当玩家出完牌时判定
bool GD_Controller::handleRoundEnd()
{
    qDebug() << "GD_Controller::handleRoundEnd： 开始判断";
    
    // 先更新已完成出牌的玩家
    updateFinishedPlayers();
    
    // 检查是否只剩最后一名玩家
    if (isLastPlayerStanding()) {
        qDebug() << "GD_Controller::handleRoundEnd： 仅剩一名玩家";
        
        // 立刻改变游戏阶段，阻止任何新的出牌/过牌操作
        // m_currentPhase = GamePhase::RoundOver;
        
        // 将最后一名玩家添加到完成顺序中
        appendLastPlayer();
        
        qDebug() << "GD_Controller::handleRoundEnd： 回合确认结束，将调度处理结果";
        
        // 使用QTimer::singleShot来调度processRoundResults，确保它在下一个事件循环中执行
        // QTimer::singleShot(0, this, &GD_Controller::processRoundResults);
        enterState(GamePhase::RoundOver);
        
        return true;
    }
    
    qDebug() << "GD_Controller::handleRoundEnd： 回合未结束";
    return false;
}

void GD_Controller::processRoundResults()
{
    try {
        qDebug() << "processRoundResults - 开始处理回合结果";
        qDebug() << "当前回合玩家完成顺序:" << m_roundFinishOrder;
        
        if (m_roundFinishOrder.size() != 4) {
            qDebug() << "错误：回合结束时玩家完成顺序数量不正确:" << m_roundFinishOrder.size();
            return;
        }

        // 获取获胜队伍
        Team* winningTeam = nullptr;
        int winnerRank = -1;

        // 找到第一名玩家所在的队伍
        Player* winner = getPlayerById(m_roundFinishOrder.first());
        if (winner) {
            winningTeam = winner->getTeam();
            // 找到队友的排名
            for (int i = 1; i < m_roundFinishOrder.size(); ++i) {
                Player* player = getPlayerById(m_roundFinishOrder[i]);
                if (player && player->getTeam() == winningTeam) {
                    winnerRank = i + 1;
                    break;
                }
            }
        }

        if (!winningTeam || winnerRank == -1) {
            qDebug() << "错误：无法确定获胜队伍或队友排名";
            return;
        }

        qDebug() << "获胜队伍ID:" << winningTeam->getId() << "队友排名:" << winnerRank;

        // 将一基于1的名次转换为0基索引再传入升级逻辑
        int partnerIndex = winnerRank - 1;  // winnerRank 是 1=第一,2=第二,3=第三,4=第四
        m_levelStatus.updateLevelsAfterRound(winningTeam->getId(), partnerIndex,
            *m_teams[0], *m_teams[1]);

        // 生成本局总结
        QString summary = generateRoundSummary();
        
        // 保存本局的排名顺序，用于下一局的进贡判断（在发送信号前保存）
        m_lastRoundFinishOrder = m_roundFinishOrder;
        qDebug() << "已保存本局排名顺序:" << m_lastRoundFinishOrder;
        
        try {
            // 发送回合结束信号
            emit sigRoundOver(summary, m_roundFinishOrder);
        }
        catch (...) {
            qDebug() << "警告：发送回合结束信号时发生异常，但将继续处理";
        }

        // 检查游戏是否结束
        if (m_levelStatus.isGameOver()) {
            enterState(GamePhase::GameOver);
        }
        else {
            // 准备开始新的一局
            m_currentRoundNumber++;
            qDebug() << "开始新的一局，局数更新为:" << m_currentRoundNumber;
            
            // 开始新一局前清空本局出牌顺序（注意：m_lastRoundFinishOrder已经保存了上局顺序）
            m_roundFinishOrder.clear();
            qDebug() << "已清空当前局出牌顺序，准备开始新一局";
            
            // 开始新一局
            startNewRound();
        }
    }
    catch (const std::exception& e) {
        qDebug() << "processRoundResults发生异常:" << e.what();
        // 确保状态正确更新，即使发生异常
        m_lastRoundFinishOrder = m_roundFinishOrder;  // 保存本局排名
        m_roundFinishOrder.clear();                   // 清空当前局排名
        m_currentRoundNumber++;                       // 更新局数
        startNewRound();                              // 开始新一局
    }
    catch (...) {
        qDebug() << "processRoundResults发生未知异常";
        // 确保状态正确更新，即使发生异常
        m_lastRoundFinishOrder = m_roundFinishOrder;  // 保存本局排名
        m_roundFinishOrder.clear();                   // 清空当前局排名
        m_currentRoundNumber++;                       // 更新局数
        startNewRound();                              // 开始新一局
    }
}

// 生成一局总结
QString GD_Controller::generateRoundSummary() const
{
    QString summary;
    QTextStream stream(&summary);

    stream << QString("第%1局结束！\n").arg(m_currentRoundNumber);
    stream << "最终排名：\n";

    for (int i = 0; i < m_roundFinishOrder.size(); ++i) {
        Player* player = getPlayerById(m_roundFinishOrder[i]);
        if (player) {
            stream << QString("第%1名：%2\n").arg(i + 1).arg(player->getName());
        }
    }

    // 添加级牌变化信息
    for (auto it = m_teams.begin(); it != m_teams.end(); ++it) {
        Team* team = it.value();
        if (team) { // 进行空指针检查
            Card levelCard;
            levelCard.setPoint(team->getCurrentLevelRank());
            stream << QString("队伍%1当前级牌：%2\n")
                .arg(it.key() + 1)
                .arg(levelCard.PointToString());
        }
    }

    return summary;
}

void GD_Controller::startTributePhaseLogic()
{
    m_pendingTributes.clear();
    m_currentTributeIndex = 0;

    // 根据上局排名确定进贡情况
    if (m_lastRoundFinishOrder.size() < 4) {
        // 异常情况，直接进入游戏阶段
        enterState(GamePhase::Playing);
        return;
    }

    int firstPlayerId = m_lastRoundFinishOrder[0];   // 头游
    int secondPlayerId = m_lastRoundFinishOrder[1];  // 二游  
    int thirdPlayerId = m_lastRoundFinishOrder[2];   // 三游
    int fourthPlayerId = m_lastRoundFinishOrder[3];  // 末游

    Team* firstTeam = getTeamOfPlayer(firstPlayerId);
    Team* fourthTeam = getTeamOfPlayer(fourthPlayerId);
    Team* thirdTeam = getTeamOfPlayer(thirdPlayerId);

    bool isDoubleDown = (thirdTeam == fourthTeam); // 双下：三游四游同队

    // 检查抗贡条件
    Player* fourthPlayer = getPlayerById(fourthPlayerId);
    Player* thirdPlayer = getPlayerById(thirdPlayerId);

    bool canResistTribute = false;

    if (isDoubleDown) {
        // 双下情况：两人各有一张大王或其中一人有两张大王
        int BigJokerInThird = 0;
        int BigJokerInFourth = 0;

        for (const Card& card : thirdPlayer->getHandCards()) {
            if (card.point() == Card::Card_BJ) BigJokerInThird++;
        }
        for (const Card& card : fourthPlayer->getHandCards()) {
            if (card.point() == Card::Card_BJ) BigJokerInFourth++;
        }

        canResistTribute = (BigJokerInThird >= 1 && BigJokerInFourth >= 1) ||
            (BigJokerInThird >= 2) || (BigJokerInFourth >= 2);
    }
    else {
        // 单下情况：末游有两张大王
        int BigJokers = 0;
        for (const Card& card : fourthPlayer->getHandCards()) {
            if (card.point() == Card::Card_BJ) BigJokers++;
        }
        canResistTribute = (BigJokers >= 2);
    }

    if (canResistTribute) {
        // 抗贡成功，头游先出牌
        emit sigBroadcastMessage("抗贡成功！");
        m_currentPlayerId = firstPlayerId;
        enterState(GamePhase::Playing);
        emit sigSetCurrentTurnPlayer(m_currentPlayerId, getPlayerById(m_currentPlayerId)->getName());
        emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        return;
    }

    // 需要进贡
    if (isDoubleDown) {
        // 双下：两人都要进贡
        TributeInfo tribute1;
        tribute1.fromPlayerId = thirdPlayerId;
        tribute1.toPlayerId = firstPlayerId;
        tribute1.isReturn = false;
        m_pendingTributes.append(tribute1);

        TributeInfo tribute2;
        tribute2.fromPlayerId = fourthPlayerId;
        tribute2.toPlayerId = secondPlayerId;
        tribute2.isReturn = false;
        m_pendingTributes.append(tribute2);

        // 还贡
        TributeInfo return1;
        return1.fromPlayerId = firstPlayerId;
        return1.toPlayerId = thirdPlayerId;
        return1.isReturn = true;
        m_pendingTributes.append(return1);

        TributeInfo return2;
        return2.fromPlayerId = secondPlayerId;
        return2.toPlayerId = fourthPlayerId;
        return2.isReturn = true;
        m_pendingTributes.append(return2);

        emit sigBroadcastMessage("双下！需要进贡");
    }
    else {
        // 单下：末游向头游进贡
        TributeInfo tribute;
        tribute.fromPlayerId = fourthPlayerId;
        tribute.toPlayerId = firstPlayerId;
        tribute.isReturn = false;
        m_pendingTributes.append(tribute);

        // 还贡
        TributeInfo returnTribute;
        returnTribute.fromPlayerId = firstPlayerId;
        returnTribute.toPlayerId = fourthPlayerId;
        returnTribute.isReturn = true;
        m_pendingTributes.append(returnTribute);

        emit sigBroadcastMessage("单下！需要进贡");
    }

    processNextTributeAction();
}

void GD_Controller::processNextTributeAction()
{
    if (m_currentTributeIndex >= m_pendingTributes.size()) {
        // 所有进贡/还贡完成
        emit sigTributePhaseEnded();

        // 确定首出玩家
        // 默认情况下，都由上一局的头游先出牌
        if (!m_lastRoundFinishOrder.isEmpty()) {
            m_currentPlayerId = m_lastRoundFinishOrder.first();
            qDebug() << "进贡阶段结束，确定首出玩家为上一局的头游: " << m_currentPlayerId;
        }
        else {
            m_currentPlayerId = 0; // 备用逻辑
            qWarning() << "无法获取上局排名，默认玩家0先出";
        }

        // 特殊情况：双下时，由头游的对家（即上一局的二游）先出牌
        Player* thirdPlayer = getPlayerById(m_lastRoundFinishOrder[2]);
        Player* fourthPlayer = getPlayerById(m_lastRoundFinishOrder[3]);
        if (thirdPlayer && fourthPlayer && thirdPlayer->getTeam() == fourthPlayer->getTeam()) {
            m_currentPlayerId = m_lastRoundFinishOrder[1];
            qDebug() << "检测到双下，首出玩家变更为头游的对家（二游）: " << m_currentPlayerId;
        }

        m_circleLeaderId = m_currentPlayerId;

        // 通过状态机进入出牌阶段
        enterState(GamePhase::Playing);
        return;
    }

    const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
    Player* fromPlayer = getPlayerById(currentTribute.fromPlayerId);
    Player* toPlayer = getPlayerById(currentTribute.toPlayerId);

    if (!fromPlayer || !toPlayer) {
        qWarning() << "错误：找不到进贡相关的玩家";
        m_currentTributeIndex++;
        processNextTributeAction();
        return;
    }

    // 进入对应的进贡状态
    if (currentTribute.isReturn) {
        // 还贡阶段
        enterState(GamePhase::TributeProcess);
    } else {
        // 进贡阶段
        enterState(GamePhase::TributeInput);
    }

    // AI自动选择
    if (fromPlayer->getType() == Player::AI) {
        QVector<Card> hand = fromPlayer->getHandCards();
        std::sort(hand.begin(), hand.end());
        if (!hand.isEmpty()) {
            Card cardToTribute = currentTribute.isReturn ? hand.first() : hand.last();
            int fid = currentTribute.fromPlayerId;
            QTimer::singleShot(1000, [this, fid, cardToTribute]() {
                this->onPlayerTributeCardSelected(fid, cardToTribute);
            });
        }
        return;
    }

    // 人类玩家：发出信号请求UI交互
    emit sigAskForTribute(currentTribute.fromPlayerId, fromPlayer->getName(),
                         currentTribute.toPlayerId, toPlayer->getName(),
                         currentTribute.isReturn);

    // 发送提示消息
    QString actionName = currentTribute.isReturn ? "还贡" : "进贡";
    QString message = QString("请%1选择一张牌%2给%3")
        .arg(fromPlayer->getName())
        .arg(actionName)
        .arg(toPlayer->getName());
    emit sigBroadcastMessage(message);
}

// ==================== 辅助方法 ====================

Player* GD_Controller::getPlayerById(int id) const
{
    // 通过QMap的映射关系查找
    auto it = m_players.find(id);
    if (it != m_players.end()) {
        return it.value();
    }
	// 如果没找到，则返回nullptr并输出
    else {
        qDebug() << "GD_Controller::getPlayerById: Player with ID " << id << " not found!";
        return nullptr;
    }
}
Team* GD_Controller::getTeamOfPlayer(int playerId) const
{
    Player* player = getPlayerById(playerId);
    if (!player) {
        return nullptr;
    }

    int teamId = player->getTeam()->getId();
    auto it = m_teams.find(teamId);
    return (it != m_teams.end()) ? it.value() : nullptr;
}

bool GD_Controller::isPlayerInGame(int playerId) const
{
    return !m_roundFinishOrder.contains(playerId);
}

QVector<int> GD_Controller::getActivePlayerIdsSorted() const
{
    QVector<int> allIds;
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        allIds.append(it.key());
    }

    // 按座位顺序排序
    std::sort(allIds.begin(), allIds.end());

    // 只返回还在没出完牌的玩家
    QVector<int> activeIds;
    for (int id : allIds) {
        if (isPlayerInGame(id)) {
            activeIds.append(id);
        }
    }
    return activeIds;
}

bool GD_Controller::allOtherActivePlayersPassed(int currentPlayerId) const
{
    QVector<int> activeIds = getActivePlayerIdsSorted();

    for (int playerId : activeIds) {
        // 如果除了当前玩家之外的玩家还有未跳过的，返回false
        if (playerId != currentPlayerId && !m_passedPlayersInCircle.contains(playerId)) {
            return false;
        }
    }
    return true;
}

bool GD_Controller::canPerformAction(int playerId, QString& errorMsg)
{
    if (m_currentPhase != GamePhase::Playing) {
        errorMsg = "当前不是出牌阶段";
        qDebug() << "Action failed for player" << playerId
            << ". Reason: Invalid Phase. Current:" << static_cast<int>(m_currentPhase)
            << ", Expected: Playing (" << static_cast<int>(GamePhase::Playing) << ")";
        return false;
    }
    if (playerId != m_currentPlayerId) {
        errorMsg = "还没轮到您操作";
        qDebug() << "Action failed for player" << playerId
            << ". Reason: Not current player. Current turn:" << m_currentPlayerId;
        return false;
    }
    return true;
}

void GD_Controller::executePlay(int playerId, const CardCombo::ComboInfo& playedCombo)
{
    // 更新手牌并广播出牌信息
    processPlayerPlay(playerId, playedCombo); // 这个函数会设置 m_circleLeaderId
    QString message = QString("%1 出牌: %2").arg(getPlayerById(playerId)->getName()).arg(playedCombo.getDescription());
    emit sigBroadcastMessage(message);
    
    // 通知UI更新显示出牌
    emit sigUpdateTableCards(playerId, playedCombo, m_lastPlayedCards);
    
    qDebug() << "GD_Controller::executePlay： 出牌处理完成, playerId=" << playerId;

    // 推进游戏
    advanceToNextPlayer();
}

void GD_Controller::executePass(int playerId)
{
    // 处理过牌并广播信息
    processPlayerPass(playerId);
    QString message = QString("%1 选择过牌").arg(getPlayerById(playerId)->getName());
    emit sigBroadcastMessage(message);
    
    // 发出玩家过牌信号
    emit sigPlayerPassed(playerId);
    
    qDebug() << "GD_Controller::executePass： 过牌处理完成, playerId=" << playerId;

    // 推进游戏
    advanceToNextPlayer();
}

void GD_Controller::nextPlayer()
{
    // 固定顺序：0,1,2,3,0,1,2,3...
    const int numPlayers = m_players.size();
    if (numPlayers == 0) return;

    int nextId = (m_currentPlayerId + 1) % numPlayers;

    // 跳过已经出完牌的玩家
    while (m_roundFinishOrder.contains(nextId) && m_activePlayersInRound > 1) {
        nextId = (nextId + 1) % numPlayers;
    }

    m_currentPlayerId = nextId;

    qDebug() << "GD_Controller::nextPlayer： 下一个玩家ID计算为 " << m_currentPlayerId;
}

// 新增：扫描并更新已完成出牌的玩家状态，并发送广播
void GD_Controller::updateFinishedPlayers()
{
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        Player* player = it.value();
        if (player && player->getHandCards().isEmpty() && !m_roundFinishOrder.contains(it.key())) {
            m_roundFinishOrder.append(it.key());
            m_activePlayersInRound--;
            QString message = QString("%1 出完了所有牌，获得第%2名！")
                .arg(player->getName())
                .arg(5 - m_activePlayersInRound);
            emit sigBroadcastMessage(message);
            qDebug() << "GD_Controller::updateFinishedPlayers： 玩家" << it.key() << "出完牌";
        }
    }
}

bool GD_Controller::isLastPlayerStanding() const
{
    return m_activePlayersInRound <= 1;
}

void GD_Controller::appendLastPlayer()
{
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        if (!m_roundFinishOrder.contains(it.key())) {
            m_roundFinishOrder.append(it.key());
            qDebug() << "GD_Controller::appendLastPlayer： 最后一名玩家ID=" << it.key();
            break;
        }
    }
}

void GD_Controller::enterState(GamePhase newPhase)
{
    if (m_currentPhase == newPhase) return;

    qDebug() << "状态转换：从" << static_cast<int>(m_currentPhase) << "到" << static_cast<int>(newPhase);
    m_currentPhase = newPhase;

    switch (newPhase) {
        case GamePhase::NotStarted:
            // 重置所有游戏状态
            m_currentPlayerId = -1;
            m_circleLeaderId = -1;
            m_activePlayersInRound = 0;
            m_currentRoundNumber = 0;
            m_currentTableCombo.type = CardComboType::Invalid;
            m_currentTableCombo.cards_in_combo.clear();
            m_passedPlayersInCircle.clear();
            break;

        case GamePhase::Dealing:
            qDebug() << "进入状态：Dealing - 开始发牌阶段";
            // 1. 重置本轮状态
            m_passedPlayersInCircle.clear();
            m_activePlayersInRound = 4;
            m_currentTableCombo.type = CardComboType::Invalid;
            m_currentTableCombo.cards_in_combo.clear();
            m_circleLeaderId = -1;
            
            // 2. 发送新一轮开始的信号和消息
            emit sigNewRoundStarted(m_currentRoundNumber);
            {
                Card currentLevel_card;
                currentLevel_card.setPoint(m_levelStatus.getTeamPlayingLevel(0));
                QString levelStr = currentLevel_card.PointToString();
                emit sigBroadcastMessage(QString("第%1局开始！当前级牌：%2").arg(m_currentRoundNumber).arg(levelStr));
            }
            
            // 3. 调用发牌函数
            dealCardsToPlayers();

            // 4. 发牌后，决定下一步流程
            if (m_currentRoundNumber > 1 && m_lastRoundFinishOrder.size() == 4) {
                qDebug() << "发牌完毕，进入进贡流程";
                startTributePhaseLogic();
            } else {
                qDebug() << "发牌完毕，直接进入出牌流程";
                determineFirstPlayerForRound();
                enterState(GamePhase::Playing);
            }
            break;

        case GamePhase::Playing:
            {
                qDebug() << "进入状态：Playing - 开始出牌阶段";
                // 确保当前玩家已设置
                if (m_currentPlayerId == -1) {
                    qWarning() << "错误：进入Playing状态时currentPlayerId未设置";
                    return;
                }

                Player* currentPlayer = getPlayerById(m_currentPlayerId);
                if (!currentPlayer) {
                    qWarning() << "错误：找不到当前玩家，ID:" << m_currentPlayerId;
                    return;
                }

                // 设置当前玩家并启用其控制
                emit sigSetCurrentTurnPlayer(m_currentPlayerId, currentPlayer->getName());
                
                // 如果是新的一圈开始，不能过牌
                bool canPass = (m_currentTableCombo.type != CardComboType::Invalid);
                emit sigEnablePlayerControls(m_currentPlayerId, true, canPass);

                // 广播轮到谁出牌
                emit sigBroadcastMessage(QString("轮到 %1 出牌！").arg(currentPlayer->getName()));

        		// 如果是AI玩家，触发自动出牌
                QTimer::singleShot(0, [this]() {
                    if (Player* p = getPlayerById(m_currentPlayerId)) {
                        p->autoPlay(this, m_currentTableCombo);
                    }
                });
            }
            break;

        case GamePhase::TributeInput:
            {
                qDebug() << "进入状态：TributeInput - 等待玩家选择进贡牌";
                const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
                Player* fromPlayer = getPlayerById(currentTribute.fromPlayerId);
                
                if (fromPlayer) {
                    emit sigEnablePlayerControls(currentTribute.fromPlayerId, true, false);
                    emit sigBroadcastMessage(QString("请%1选择最大的牌进贡").arg(fromPlayer->getName()));
                }
            }
            break;

        case GamePhase::TributeProcess:
            {
                qDebug() << "进入状态：TributeProcess - 等待玩家选择还贡牌";
                const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
                Player* fromPlayer = getPlayerById(currentTribute.fromPlayerId);
                Player* toPlayer = getPlayerById(currentTribute.toPlayerId);
                
                if (fromPlayer && toPlayer) {
                    Team* fromTeam = getTeamOfPlayer(currentTribute.fromPlayerId);
                    Team* toTeam = getTeamOfPlayer(currentTribute.toPlayerId);
                    bool isTeammate = (fromTeam == toTeam);
                    
                    emit sigEnablePlayerControls(currentTribute.fromPlayerId, true, false);
                    if (isTeammate) {
                        emit sigBroadcastMessage(QString("请%1选择一张10或以下的牌还贡给队友%2")
                            .arg(fromPlayer->getName())
                            .arg(toPlayer->getName()));
                    } else {
                        emit sigBroadcastMessage(QString("请%1选择一张牌还贡给%2")
                            .arg(fromPlayer->getName())
                            .arg(toPlayer->getName()));
                    }
                }
            }
            break;

        case GamePhase::RoundOver:
            qDebug() << "进入状态：RoundOver - 本局结束";
            // 回合结束，调度结果处理
            QTimer::singleShot(0, this, &GD_Controller::processRoundResults);
            break;

        case GamePhase::GameOver:
            {
                qDebug() << "进入状态：GameOver - 游戏结束";
                // 游戏结束，处理最终逻辑
                int winnerTeamId = m_levelStatus.getGameWinnerTeamId();
                Team* finalWinner = m_teams[winnerTeamId];
                if (finalWinner) {
                    QString finalMessage = QString("恭喜%1队获得最终胜利！").arg(winnerTeamId + 1);
                    try {
                        emit sigGameOver(winnerTeamId, QString("队伍%1").arg(winnerTeamId + 1), finalMessage);
                    }
                    catch (...) {
                        qDebug() << "警告：发送游戏结束信号时发生异常";
                    }
                }
            }
            break;
    }
}

void GD_Controller::advanceToNextPlayer()
{
    // 记录当前行动的玩家ID，以便后续判断该玩家是否刚刚出完牌
    int lastPlayerId = m_currentPlayerId;
    
    // 1. 每次推进前，都先检查是否有人出完牌，以及是否因此导致回合结束
    if (handleRoundEnd()) {
        // 如果回合结束，processRoundResults会被调用并启动新一轮或结束游戏，
        // 此处无需再做任何事。
        return;
    }
    
    // 检查刚才行动的玩家是否已经出完牌（即不再游戏中）
    bool lastPlayerFinished = !isPlayerInGame(lastPlayerId);
    
    // 2. 检查当前"圈"是否结束
    // 条件：(A)当前有领出者，并且所有其他活跃玩家都已经Pass了
    //      或者(B)刚才行动的玩家出完了所有牌
    bool allOthersPassed = (m_circleLeaderId != -1 && allOtherActivePlayersPassed(m_circleLeaderId));
    bool circleEnded = allOthersPassed || lastPlayerFinished;
    
    if (circleEnded)
    {
        qDebug() << "advanceToNextPlayer: 圈结束! " 
                 << (lastPlayerFinished ? "玩家出完牌导致圈结束。" : "所有其他玩家已Pass。")
                 << "领出者: " << m_circleLeaderId;
        
        // 确定下一圈的领出者
        int nextLeaderId = -1;
        
        if (allOthersPassed && isPlayerInGame(m_circleLeaderId)) {
            // 情况A：所有人都Pass且圈主还在游戏中，由圈主开始新一圈
            nextLeaderId = m_circleLeaderId;
            qDebug() << "圈主 " << m_circleLeaderId << " 仍在游戏中，由他开始新一圈。";
        }
        else {
            // 情况B：出牌者打光牌或圈主已出完牌
            // 由当前行动者的下一位未出完牌的玩家开始新一圈
            m_currentPlayerId = lastPlayerId;
            nextPlayer(); // 找到下一个合法玩家
            nextLeaderId = m_currentPlayerId;
            qDebug() << "需要寻找新的圈主，下一圈由 " << nextLeaderId << " 开始。";
        }

        // 延迟1.5秒后再开始新一圈，让玩家有时间看清上一手牌
        QTimer::singleShot(1500, this, [this, nextLeaderId]() {
            // 设置新一圈的领出者和当前玩家
            m_currentPlayerId = nextLeaderId;
            m_circleLeaderId = nextLeaderId;
            
            Player* leader = getPlayerById(m_currentPlayerId);
            if (!leader) return; // 安全检查
            
            // 重置桌面，开始新的一圈
            resetTableCombo();
            m_passedPlayersInCircle.clear();
            
            // 通知UI和所有玩家
            emit sigClearTableCards();
            emit sigBroadcastMessage(QString("新的一圈开始，由 %1 出牌。").arg(leader->getName()));
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, leader->getName());
            
            // 新一圈的领出者不能Pass
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);

            // 如果是AI玩家，触发其行动
            if (leader->getType() == Player::AI) {
                QTimer::singleShot(500, this, [this]() {
                    if (m_currentPhase == GamePhase::Playing) {
                        Player* p = getPlayerById(m_currentPlayerId);
                        if (p && p->getType() == Player::AI) {
                            p->autoPlay(this, m_currentTableCombo);
                        }
                    }
                });
            }
        });
    }
    else
    {
        // 3. 圈未结束，正常轮到下一位玩家
        nextPlayer(); // nextPlayer() 只负责按顺序计算出下一个玩家ID
        
        Player* next_p = getPlayerById(m_currentPlayerId);
        if (!next_p) return; // 安全检查
        
        qDebug() << "advanceToNextPlayer: 圈未结束，轮到下一位玩家 " << m_currentPlayerId;
        
        // 通知UI和所有玩家
        emit sigBroadcastMessage(QString("轮到 %1 出牌！").arg(next_p->getName()));
        emit sigSetCurrentTurnPlayer(m_currentPlayerId, next_p->getName());
        
        // 圈未结束时，玩家可以选择出牌或过牌
        emit sigEnablePlayerControls(m_currentPlayerId, true, true);
    }
    
    // 4. 如果圈未结束，触发AI行动
    // 注意：圈结束的情况下，AI行动会在延迟后的新一圈开始时触发
    if (!circleEnded) {
        QTimer::singleShot(500, [this]() {
            if (m_currentPhase == GamePhase::Playing) {
                Player* p = getPlayerById(m_currentPlayerId);
                if (p && p->getType() == Player::AI) {
                    p->autoPlay(this, m_currentTableCombo);
                }
            }
        });
    }
}

// 添加记牌器相关的新方法实现
void GD_Controller::initializeCardCounts()
{
    qDebug() << "GD_Controller::initializeCardCounts - 初始化记牌器数据";
    
    // 清空现有数据
    m_remainingCardCounts.clear();
    
    // 初始化所有牌的数量
    // 游戏使用两副牌，所以大/小王各2张，其他点数各8张(每种花色2张)
    m_remainingCardCounts[Card::CardPoint::Card_2] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_3] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_4] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_5] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_6] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_7] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_8] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_9] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_10] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_J] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_Q] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_K] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_A] = 8;
    m_remainingCardCounts[Card::CardPoint::Card_LJ] = 2; // 小王
    m_remainingCardCounts[Card::CardPoint::Card_BJ] = 2; // 大王
    
    // 发送信号通知UI更新记牌器
    emit sigCardCountsUpdated(m_remainingCardCounts);
}

void GD_Controller::updateCardCounts(const QVector<Card>& playedCards)
{
    qDebug() << "GD_Controller::updateCardCounts - 更新记牌器数据，牌数:" << playedCards.size();
    
    // 遍历打出的牌，更新记牌器数据
    for (const Card& card : playedCards) {
        Card::CardPoint point = card.point();
        
        // 确保点数在映射中存在
        if (m_remainingCardCounts.contains(point)) {
            // 减少相应点数的牌的数量
            int currentCount = m_remainingCardCounts[point];
            if (currentCount > 0) {
                m_remainingCardCounts[point] = currentCount - 1;
            }
        }
    }
    
    // 发送信号通知UI更新记牌器
    emit sigCardCountsUpdated(m_remainingCardCounts);
}