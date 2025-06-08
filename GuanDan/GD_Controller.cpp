#include "GD_Controller.h"
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>
#include <QTextStream>

#include "carddeck.h"
#include "WildCardDialog.h"

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

    m_currentPhase = GamePhase::Dealing;
    m_currentRoundNumber = 1;

    emit sigGameStarted();
    emit sigBroadcastMessage("掼蛋游戏开始！");

    startNewRound();
}

// ==================== 玩家操作槽函数 ====================

// 处理玩家出牌操作(总方法)
// 只进行了判断合法性和处理玩家手牌，信号部分没有处理
void GD_Controller::onPlayerPlay(int playerId, const QVector<Card>& cardsToPlay)
{
    QString errorMsg;
    if (!canPerformAction(playerId, errorMsg)) {
        emit sigShowPlayerMessage(playerId, errorMsg, true);
        return;
    }

    // 记录玩家选中的原始卡牌，用于正确移除手牌
    m_lastPlayedCards = cardsToPlay;

    CardCombo::ComboInfo playedCombo;
    if (!PlayerPlay(playerId, cardsToPlay, playedCombo)) {
        emit sigShowPlayerMessage(playerId, "出牌不符合规则", true);
        return;
    }
    executePlay(playerId, playedCombo);

    if (handleRoundEnd()) {
        return;
    }
    handleCircleEnd();
}

// 处理玩家过牌操作(总方法)
void GD_Controller::onPlayerPass(int playerId)
{
    QString errorMsg;
    if (!canPerformAction(playerId, errorMsg)) {
        emit sigShowPlayerMessage(playerId, errorMsg, true);
        return;
    }
    // 第一个出牌的人不能过牌的检查留在canPerformAction或executePass前
    if (m_currentTableCombo.type == CardComboType::Invalid) {
        emit sigShowPlayerMessage(playerId, "您是第一个出牌，不能过牌", true);
        return;
    }
    executePass(playerId);

    if (handleRoundEnd()) {
        return;
    }
    handleCircleEnd();
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
    m_currentPhase = GamePhase::Dealing;
    m_roundFinishOrder.clear();
    m_passedPlayersInCircle.clear();
    m_activePlayersInRound = 4;
    m_currentTableCombo.type = CardComboType::Invalid;
    m_currentTableCombo.cards_in_combo.clear();
    m_circleLeaderId = -1;

    // 发送新一轮开始的信号
    emit sigNewRoundStarted(m_currentRoundNumber);

    // 获取当前级牌信息
    Card currentLevel_card;
    currentLevel_card.setPoint(m_levelStatus.getTeamPlayingLevel(0)); // 两队级牌相同
    QString levelStr = currentLevel_card.PointToString();
    emit sigBroadcastMessage(QString("第%1局开始！当前级牌：%2").arg(m_currentRoundNumber).arg(levelStr));

    // 处理进贡还贡阶段
    if (m_currentRoundNumber > 1) {
        dealCardsToPlayers();
        startTributePhaseLogic();
    }
    else {
        dealCardsToPlayers();
    }
}

void GD_Controller::dealCardsToPlayers()
{
    // 创建牌组并自动初始化（deck构造函数中会创建两副牌并洗牌）
    CardDeck deck;
	QVector<Card> allCards = deck.getDeckCards(); // 得到乱序的牌组（注意，是无所有者的牌组！）

    // 发牌
    const int cardsPerPlayer = 27; // 每个玩家27张
	QVector<int> playerIds = getActivePlayerIdsSorted(); // 因为没人出完牌，所以直接获取所有玩家ID

	// 给四个玩家发牌
    for (int i = 0; i < 4; ++i) {
		QVector<Card> playerCards; // 添加到每个玩家的手牌数组
        for (int j = 0; j < cardsPerPlayer; ++j) {
            playerCards.append(allCards[i * cardsPerPlayer + j]);
        }
        // 获取玩家对象的映射
        Player* player = getPlayerById(playerIds[i]);

        // 给playerCards找主人
        for (Card& card : playerCards) {
            card.setOwner(player); // 设置牌的所有者ID
		}
		// 如果玩家存在，则清空原有手牌并添加新牌
        if (player) {
            // 清空原有手牌并添加新牌
            player->getHandCards().clear();
            player->addCards(playerCards);
            emit sigCardsDealt(playerIds[i], playerCards); // 发送发牌完成信号
        }
        else
        {
			qDebug() << "错误：玩家ID" << playerIds[i] << "不存在，无法发牌";
        }
    }

    // 设置游戏阶段为出牌阶段
    m_currentPhase = GamePhase::Playing;

    // 确定首个出牌玩家
    determineFirstPlayerForRound();
}

void GD_Controller::determineFirstPlayerForRound()
{
    qDebug() << "GD_Controller::determineFirstPlayerForRound() - 开始确定首个出牌玩家";
    
    // 第一局，强制设置ID为0的玩家开始
    if (m_currentRoundNumber == 1) {
        // 强制设置ID为0的玩家为首个出牌玩家
        m_currentPlayerId = 0;
        m_circleLeaderId = m_currentPlayerId;
        Player* firstPlayer = getPlayerById(m_currentPlayerId);

        qDebug() << "第一局强制设置玩家ID:" << m_currentPlayerId << "作为首个出牌玩家";
        
        if (firstPlayer) {
            qDebug() << "发送设置当前玩家信号:" << m_currentPlayerId << firstPlayer->getName();
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, firstPlayer->getName());
            
            qDebug() << "发送启用玩家控制信号:" << m_currentPlayerId << "可出牌:true 可跳过:false";
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
            
            emit sigBroadcastMessage(QString("第%1局开始，玩家%2先出牌！")
                .arg(m_currentRoundNumber)
                .arg(firstPlayer->getName()));
        }
        else {
            qWarning() << "错误：首个出牌玩家ID" << m_currentPlayerId << "不存在，无法开始游戏";
        }
        return;  // 第一局处理完毕后直接返回，避免重复发送信号
    }
    // 其他局，由上一局最后一名玩家开始
    else {
		// 如果上一局完成了，则最后一名玩家开始新一局
        if (!m_roundFinishOrder.isEmpty()) {
            m_currentPlayerId = m_roundFinishOrder.last();
            m_circleLeaderId = m_currentPlayerId;
            qDebug() << "非第一局，选择上一局最后一名玩家ID:" << m_currentPlayerId << "作为首个出牌玩家";
        }
    }
    
	// 如果当前玩家ID有效，则设置当前玩家为出牌玩家
    if (m_currentPlayerId != -1) {
        Player* firstPlayer = getPlayerById(m_currentPlayerId);
        if (firstPlayer) {
            qDebug() << "最终设置当前玩家:" << m_currentPlayerId << firstPlayer->getName();
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, firstPlayer->getName());
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        } else {
            qWarning() << "错误：无法找到ID为" << m_currentPlayerId << "的玩家";
        }
    } else {
        qWarning() << "错误：当前玩家ID无效";
    }
}

// 找到下一个未出完牌玩家的轮次逻辑 (仅控制m_currentPlayerId）
void GD_Controller::nextPlayerTurn()
{
    QVector<int> activePlayers = getActivePlayerIdsSorted();
    if (activePlayers.isEmpty()) {
		qDebug() << "GD_Controller::nextPlayerTurn():玩家都出完牌了，没有下一个玩家";
        return;
    }
    // 找到当前玩家的索引
    int currentIndex = activePlayers.indexOf(m_currentPlayerId);
    if (currentIndex == -1) return;

    // 找到下一个未出完牌的玩家
    int nextIndex = (currentIndex + 1) % activePlayers.size();
	// 设置当前玩家ID为下一个玩家
    m_currentPlayerId = activePlayers[nextIndex];

    Player* nextPlayer = getPlayerById(m_currentPlayerId);
    if (!nextPlayer) {
        return;
		qDebug()<< "GD_Controller::nextPlayerTurn():下一个玩家ID" << m_currentPlayerId << "不存在";
    }
	emit sigSetCurrentTurnPlayer(m_currentPlayerId, nextPlayer->getName());
    emit sigEnablePlayerControls(m_currentPlayerId, true, false);
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
            qDebug() << "GD_Controller::PlayerPlay： 玩家" << player->getName() << "选择的牌有多种可出牌型，共 " << possibleCombos.size() << " 种。正在弹出选择对话框...";

            // 创建并显示WildCardDialog
            WildCardDialog dialog(possibleCombos, nullptr);
            if (dialog.exec() == QDialog::Accepted && dialog.hasValidSelection()) {
                // 获取用户选择的组合
                outPlayedCombo = dialog.getSelectedCombo();
                qDebug() << "测试：用户选择了组合";
                qDebug() << "  类型:" << outPlayedCombo.type;
                qDebug() << "  描述:" << outPlayedCombo.getDescription();
                qDebug() << "  使用的牌:";
                for (const Card& card : outPlayedCombo.cards_in_combo) {
                    qDebug() << "    " << card.PointToString() << card.SuitToString();
                }
                return true;
            }
            qDebug() << "测试：用户取消了选择";
            return false;
		}
    }
    // 不能出牌，返回false
    else
    {
        qDebug() << "GD_Controller::PlayerPlay： 玩家" << player->getName() << "选择的牌不符合出牌规则";
        return false;
	}
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
    m_lastPlayedCards.clear();

    // 更新桌面牌型
    m_currentTableCombo = playedCombo;
    m_circleLeaderId = playerId;
    m_passedPlayersInCircle.clear(); // 清空本圈已过牌的玩家

    // 通知UI更新
    emit sigUpdatePlayerHand(playerId, player->getHandCards());
    emit sigUpdateTableCards(playedCombo, player->getName());

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

// 当玩家跳过时进行判定
bool GD_Controller::checkCircleEnd()
{
    qDebug() << "GD_Controller::checkCircleEnd： 调用";
    // 1. 赢家开始新的一圈：检查是否所有其他活跃玩家都已过牌
    if (allOtherActivePlayersPassed(m_circleLeaderId)) {
        qDebug() << "GD_Controller::checkCircleEnd： 所有其他玩家已过, leaderId=" << m_circleLeaderId;
        // 一圈结束，赢家开始新一圈，重置桌面牌型
        resetCircleState();
        // 发送重置信号
        emitCircleResetSignals();
        return true;
    } else if (m_roundFinishOrder.size() <= 2) {
        qDebug() << "GD_Controller::checkCircleEnd： 第一或第二玩家出完牌, 清空桌面";
        resetTableCombo();
        emit sigClearTableCards();
    }
    qDebug() << "GD_Controller::checkCircleEnd： 未触发圈结束";
    return false;
}

// 当玩家出完牌时判定
bool GD_Controller::checkRoundEnd()
{
    qDebug() << "GD_Controller::checkRoundEnd： 调用";
    updateFinishedPlayers();
    if (isLastPlayerStanding()) {
        qDebug() << "GD_Controller::checkRoundEnd： 仅剩一名玩家";
        appendLastPlayer();
        processRoundResults();
        return true;
    }
    qDebug() << "GD_Controller::checkRoundEnd： 回合继续";
    return false;
}

void GD_Controller::processRoundResults()
{
    if (m_roundFinishOrder.size() != 4) return;

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

    if (!winningTeam || winnerRank == -1) return;

    // 更新级牌状态
    m_levelStatus.updateLevelsAfterRound(winningTeam->getId(), winnerRank,
        *m_teams[0], *m_teams[1]);

    // 生成本局总结
    QString summary = generateRoundSummary();
    emit sigRoundOver(summary, m_roundFinishOrder);

    // 检查游戏是否结束
    if (m_levelStatus.isGameOver()) {
        int winnerTeamId = m_levelStatus.getGameWinnerTeamId();
        Team* finalWinner = m_teams[winnerTeamId];
        if (finalWinner) {
            QString finalMessage = QString("恭喜%1队获得最终胜利！").arg(winnerTeamId + 1);
            emit sigGameOver(winnerTeamId, QString("队伍%1").arg(winnerTeamId + 1), finalMessage);
        }
    }
    else {
        // 准备开始新的一局
        m_currentRoundNumber++;
    	startNewRound();
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
    m_currentPhase = GamePhase::TributeProcess;
    m_pendingTributes.clear();
    m_currentTributeIndex = 0;

    // 根据上局排名确定进贡情况
    if (m_roundFinishOrder.size() < 4) {
        // 异常情况，直接开始新局
        determineFirstPlayerForRound();
        m_currentPhase = GamePhase::Playing;
        return;
    }

    int firstPlayerId = m_roundFinishOrder[0];   // 头游
    int secondPlayerId = m_roundFinishOrder[1];  // 二游  
    int thirdPlayerId = m_roundFinishOrder[2];   // 三游
    int fourthPlayerId = m_roundFinishOrder[3];  // 末游

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
        m_currentPhase = GamePhase::Playing;
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
        QVector<TributeInfo> tributes;
        // 只收集进贡（非还贡）操作
        for (const TributeInfo& tribute : m_pendingTributes) {
            if (!tribute.isReturn) {
                tributes.append(tribute);
            }
        }
        if (tributes.size() >= 2) {
            // 双下情况：进贡大者先出牌
            if (tributes[0].card > tributes[1].card) {
                m_currentPlayerId = tributes[0].fromPlayerId;
            }
            else {
                m_currentPlayerId = tributes[1].fromPlayerId;
            }
        }
        else if (tributes.size() == 1) {
            // 单下情况：进贡者先出牌
            m_currentPlayerId = tributes[0].fromPlayerId;
        }
        else {
            // 异常情况，使用头游先出牌
            if (!m_roundFinishOrder.isEmpty()) {
                m_currentPlayerId = m_roundFinishOrder.first();
            }
        }
        m_currentPhase = GamePhase::Playing;
        Player* firstPlayer = getPlayerById(m_currentPlayerId);
        if (firstPlayer) {
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, firstPlayer->getName());
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        }
        return;
    }

    const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
    Player* fromPlayer = getPlayerById(currentTribute.fromPlayerId);
    Player* toPlayer = getPlayerById(currentTribute.toPlayerId);

    if (!fromPlayer || !toPlayer) {
        return;
    }

    if (currentTribute.isReturn) {
        // 还贡阶段：让玩家选择牌
        m_currentPhase = GamePhase::TributeProcess;
        Team* fromTeam = getTeamOfPlayer(currentTribute.fromPlayerId);
        Team* toTeam = getTeamOfPlayer(currentTribute.toPlayerId);
        bool isTeammate = (fromTeam == toTeam);

        // 发送信号，通知UI显示还贡选择界面
        emit sigAskForTribute(currentTribute.fromPlayerId, fromPlayer->getName(),
            currentTribute.toPlayerId, toPlayer->getName(), true);
            
        // 在消息中提示规则
         if (isTeammate) {
             qDebug("GD_Controller::processNextTributeAction(): 请选择一张10或以下的牌还贡给队友");
         } else {
			 qDebug("GD_Controller::processNextTributeAction(): 请选择一张牌还贡给对手");
         }
    }
    else {
        // 进贡阶段：让玩家选择牌
        m_currentPhase = GamePhase::TributeInput;
        emit sigAskForTribute(currentTribute.fromPlayerId, fromPlayer->getName(),
            currentTribute.toPlayerId, toPlayer->getName(), false);
            
         // 提示玩家选择最大的牌
		qDebug("GD_Controller::processNextTributeAction(): 请选择手牌中最大的牌进贡给对手");
    }
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
        return false;
    }
    if (playerId != m_currentPlayerId) {
        errorMsg = "还没轮到您操作";
        return false;
    }
    return true;
}

void GD_Controller::executePlay(int playerId, const CardCombo::ComboInfo& playedCombo)
{
    // 更新手牌并广播出牌信息
    processPlayerPlay(playerId, playedCombo);
    QString message = QString("%1 出牌: %2").arg(getPlayerById(playerId)->getName()).arg(playedCombo.getDescription());
    emit sigBroadcastMessage(message);
    qDebug() << "GD_Controller::executePlay： 出牌处理完成, playerId=" << playerId;
    // 切换到下一个玩家
    nextPlayer();
}

void GD_Controller::executePass(int playerId)
{
    // 处理过牌并广播信息
    processPlayerPass(playerId);
    QString message = QString("%1 选择过牌").arg(getPlayerById(playerId)->getName());
    emit sigBroadcastMessage(message);
    qDebug() << "GD_Controller::executePass： 过牌处理完成, playerId=" << playerId;
    // 切换到下一个玩家
    nextPlayer();
}

bool GD_Controller::handleRoundEnd()
{
    qDebug() << "GD_Controller::handleRoundEnd： 开始判断";
    if (checkRoundEnd()) {
        qDebug() << "GD_Controller::handleRoundEnd： 回合结束";
        // 构建回合结束摘要
        QStringList summary;
        for (int id : m_roundFinishOrder) {
            summary << getPlayerById(id)->getName();
        }
        emit sigRoundOver(summary.join(", "), m_roundFinishOrder);
        return true;
    }
    qDebug() << "GD_Controller::handleRoundEnd： 回合未结束";
    return false;
}

void GD_Controller::handleCircleEnd()
{
    qDebug() << "GD_Controller::handleCircleEnd： 开始判断圈结束";
    if (allOtherActivePlayersPassed(m_currentPlayerId)) {
        qDebug() << "GD_Controller::handleCircleEnd： 圈结束, leaderId=" << m_currentPlayerId;
        // 开始新一圈
        m_circleLeaderId = m_currentPlayerId;
        m_currentTableCombo.type = CardComboType::Invalid;
        m_currentTableCombo.cards_in_combo.clear();
        m_passedPlayersInCircle.clear();
        emit sigClearTableCards();
    }
    qDebug() << "GD_Controller::handleCircleEnd： 发送启用控制信号 for playerId=" << m_currentPlayerId;
    // 重新启用玩家控制
    emit sigEnablePlayerControls(m_currentPlayerId, true, true);
}

void GD_Controller::nextPlayer()
{
    QVector<int> activeIds = getActivePlayerIdsSorted();
    auto it = std::find(activeIds.begin(), activeIds.end(), m_currentPlayerId);
    if (it != activeIds.end() && ++it != activeIds.end()) {
        m_currentPlayerId = *it;
    } else {
        m_currentPlayerId = activeIds.isEmpty() ? -1 : activeIds.front();
    }
    if (m_currentPlayerId >= 0) {
        emit sigSetCurrentTurnPlayer(m_currentPlayerId, getPlayerById(m_currentPlayerId)->getName());
        qDebug() << "GD_Controller::nextPlayer： 当前玩家ID=" << m_currentPlayerId;
    }
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

// 新增：重置桌面状态，开始新一圈（仅更新状态，不发送信号）
void GD_Controller::resetCircleState()
{
    resetTableCombo();
    m_passedPlayersInCircle.clear();
    m_currentPlayerId = m_circleLeaderId;
    qDebug() << "GD_Controller::resetCircleState： 圈状态已重置, leaderId=" << m_circleLeaderId;
}

// 新增：发送新一圈开始的UI信号
void GD_Controller::emitCircleResetSignals()
{
    emit sigClearTableCards();
    emit sigSetCurrentTurnPlayer(m_currentPlayerId, getPlayerById(m_currentPlayerId)->getName());
    emit sigEnablePlayerControls(m_currentPlayerId, true, false);
    qDebug() << "GD_Controller::emitCircleResetSignals： 信号发送完成, currentPlayerId=" << m_currentPlayerId;
}