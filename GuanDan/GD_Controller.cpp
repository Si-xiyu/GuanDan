#include "GD_Controller.h"
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>
#include <QTextStream>

#include "carddeck.h"

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

// 处理玩家出牌操作
void GD_Controller::onPlayerAttemptPlay(int playerId, const QVector<Card>& cardsToPlay)
{
    if (m_currentPhase != GamePhase::Playing) {
        emit sigShowPlayerMessage(playerId, "当前不是出牌阶段", true);
        return;
    }

    if (playerId != m_currentPlayerId) {
        emit sigShowPlayerMessage(playerId, "还没轮到您出牌", true);
        return;
    }

    CardCombo::ComboInfo playedCombo;
    if (!canPlayerPlay(playerId, cardsToPlay, playedCombo)) {
        emit sigShowPlayerMessage(playerId, "出牌不符合规则", true);
        return;
    }

    if (processPlayerPlay(playerId, playedCombo)) {
        checkCircleEndAndNextAction();
        checkRoundEndAndNextAction();
    }
}

// 处理玩家过牌操作
void GD_Controller::onPlayerPass(int playerId)
{
    if (m_currentPhase != GamePhase::Playing) {
        emit sigShowPlayerMessage(playerId, "当前不是出牌阶段", true);
        return;
    }

    if (playerId != m_currentPlayerId) {
        emit sigShowPlayerMessage(playerId, "还没轮到您操作", true);
        return;
    }

    // 检查是否可以过牌（第一个出牌的人不能过牌）
    if (m_currentTableCombo.type == CardComboType::Invalid) {
        emit sigShowPlayerMessage(playerId, "您是第一个出牌，不能过牌", true);
        return;
    }

    processPlayerPass(playerId);
    checkCircleEndAndNextAction();
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
    if (m_currentPhase != GamePhase::TributeInput) {
        emit sigShowPlayerMessage(tributingPlayerId, "当前不是进贡阶段", true);
        return;
    }

    if (m_currentTributeIndex >= m_pendingTributes.size()) {
        return;
    }

    TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];
    if (currentTribute.fromPlayerId != tributingPlayerId) {
        emit sigShowPlayerMessage(tributingPlayerId, "现在不是您进贡的时候", true);
        return;
    }

    Player* player = getPlayerById(tributingPlayerId);
    if (!player || !player->getHandCards().contains(tributeCard)) {
        emit sigShowPlayerMessage(tributingPlayerId, "您没有这张牌", true);
        return;
    }

    // 验证进贡的牌
    bool isValidTribute = true;
    Card::CardPoint currentLevel = m_levelStatus.getTeamPlayingLevel(player->getTeam()->getId());

    // 获取玩家手牌中的最大牌
    Card maxCard = tributeCard;
    for (const Card& card : player->getHandCards()) {
        // 跳过红桃级牌
        if (card.suit() == Card::Heart && card.point() == currentLevel) {
            continue;
        }
        // 比较大小，使用Card的比较方法
        if (card > maxCard) {
            maxCard = card;
        }
    }

    // 验证是否是最大的牌
    if (tributeCard != maxCard) {
        emit sigShowPlayerMessage(tributingPlayerId, "必须进贡您手中最大的牌（红桃级牌除外）", true);
        return;
    }

    // 执行进贡
    currentTribute.card = tributeCard;
    completeTribute(currentTribute);

    m_currentTributeIndex++;
    processNextTributeAction();
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
	// 第一局，随机一名玩家开始
    if (m_currentRoundNumber == 1) {
		QVector<int> playerIds = getActivePlayerIdsSorted(); // 获取所有玩家ID
        // 随机选择一个玩家作为首个出牌玩家
        int randomIndex = QRandomGenerator::global()->bounded(playerIds.size());
        m_currentPlayerId = playerIds[randomIndex];

        m_circleLeaderId = m_currentPlayerId; // 首个出牌玩家也是圈主
        Player* firstPlayer = getPlayerById(m_currentPlayerId);

		// 如果找到首个出牌玩家，则发送信号
        if (firstPlayer) {
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, firstPlayer->getName());
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
            emit sigBroadcastMessage(QString("第%1局开始，随机选择%2先出牌！")
                .arg(m_currentRoundNumber)
                .arg(firstPlayer ? firstPlayer->getName() : "未知玩家"));
		}
        else
        {
	        qWarning() << "错误：首个出牌玩家ID" << m_currentPlayerId << "不存在，无法开始游戏";
        }
       
    }
    // 其他局，由上一局最后一名玩家开始
    else {
		// 如果上一局完成了，则最后一名玩家开始新一局
        if (!m_roundFinishOrder.isEmpty()) {
            m_currentPlayerId = m_roundFinishOrder.last();
            m_circleLeaderId = m_currentPlayerId;
        }
    }
	// 如果当前玩家ID有效，则设置当前玩家为出牌玩家
    if (m_currentPlayerId != -1) {
        Player* firstPlayer = getPlayerById(m_currentPlayerId);
        if (firstPlayer) {
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, firstPlayer->getName());
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        }
    }
}

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
    m_currentPlayerId = activePlayers[nextIndex];

    Player* nextPlayer = getPlayerById(m_currentPlayerId);
    if (!nextPlayer) {
        return;
		qDebug()<< "GD_Controller::nextPlayerTurn():下一个玩家ID" << m_currentPlayerId << "不存在";
    }
	emit sigSetCurrentTurnPlayer(m_currentPlayerId, nextPlayer->getName());
    emit sigEnablePlayerControls(m_currentPlayerId, true, false);
}

bool GD_Controller::canPlayerPlay(int playerId, const QVector<Card>& cardsToPlay, CardCombo::ComboInfo& outPlayedCombo)
{
    Player* player = getPlayerById(playerId);
    if (!player) return false;

    // 验证玩家是否拥有这些牌
    for (const Card& card : cardsToPlay) {
        if (!player->getHandCards().contains(card)) {
            return false;
        }
    }

    // 获取当前级牌
    Card::CardPoint currentLevel = m_levelStatus.getTeamPlayingLevel(getTeamOfPlayer(playerId)->getId());

    // 使用Player的canPlayCards方法验证牌型
	if (!player->canPlayCards(cardsToPlay, m_currentTableCombo))
        return false;
    else
        return true;
}

bool GD_Controller::processPlayerPlay(int playerId, const CardCombo::ComboInfo& playedCombo)
{
    // 从玩家手牌中移除牌
    Player* player = getPlayerById(playerId);
	player->removeCards(playedCombo.cards_in_combo);

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

    // 检查玩家是否出完牌
    if (player->getHandCards().isEmpty()) {
        m_roundFinishOrder.append(playerId);
        m_activePlayersInRound--;

        emit sigBroadcastMessage(QString("%1 出完了所有牌！").arg(player->getName()));

        // 如果这是第一个出完牌的玩家，清空桌面开始新一圈
        if (m_roundFinishOrder.size() == 1) {
            m_currentTableCombo.type = CardComboType::Invalid;
            m_currentTableCombo.cards_in_combo.clear();
            emit sigClearTableCards();
        }
    }

    return true;
}

void GD_Controller::processPlayerPass(int playerId)
{
    if (m_currentTableCombo.type == CardComboType::Invalid) {
        emit sigShowPlayerMessage(playerId, "首轮出牌不能过", true);
        return;
    }

    Player* player = getPlayerById(playerId);
    if (!player) return;

    m_passedPlayersInCircle.insert(playerId);

    QString message = QString("%1 选择不出").arg(player->getName());
    emit sigBroadcastMessage(message);

    // 检查是否所有其他玩家都过牌了
    if (allOtherActivePlayersPassed(m_circleLeaderId)) {
        // 清空桌面，由最后出牌的玩家继续出牌
        m_currentTableCombo.type = CardComboType::Invalid;
        m_currentTableCombo.cards_in_combo.clear();
        m_passedPlayersInCircle.clear();
        emit sigClearTableCards();

        QString newMessage = QString("所有玩家都过牌，轮到 %1 出牌").arg(player->getName());
        emit sigBroadcastMessage(newMessage);
    }
    else {
        // 轮到下一个玩家
        nextPlayerTurn();
    }
}

// 当玩家跳过时进行判定
void GD_Controller::checkCircleEndAndNextAction()
{
    // 检查是否所有其他活跃玩家都已过牌
    if (allOtherActivePlayersPassed(m_circleLeaderId)) {
        // 一圈结束，赢家开始新一圈，重置桌面牌型
        m_currentTableCombo.type = CardComboType::Invalid;
        m_currentTableCombo.cards_in_combo.clear();
        m_passedPlayersInCircle.clear();
		// 重置当前玩家ID为这小轮的赢家（没人大过他）
        m_currentPlayerId = m_circleLeaderId;

        emit sigClearTableCards();

        Player* winner = getPlayerById(m_circleLeaderId);
        if (winner) {
            emit sigSetCurrentTurnPlayer(m_currentPlayerId, winner->getName());
            emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        }
    }
    else {
        // 继续下一个玩家
        nextPlayerTurn();
    }
}

// 当玩家出完牌时判定
void GD_Controller::checkRoundEndAndNextAction()
{
    // 检查是否有玩家出完牌
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        Player* player = it.value();
        if (player && player->getHandCards().isEmpty() && !m_roundFinishOrder.contains(it.key())) {
            m_roundFinishOrder.append(it.key());
            m_activePlayersInRound--;

            QString message = QString("%1 出完了所有牌，获得第%2名！")
                .arg(player->getName())
                .arg(5 - m_activePlayersInRound);
            emit sigBroadcastMessage(message);
        }
    }

    // 如果只剩一个玩家，本局结束
    if (m_activePlayersInRound <= 1) {
        // 将最后一名玩家加入排名
        for (auto it = m_players.begin(); it != m_players.end(); ++it) {
            if (!m_roundFinishOrder.contains(it.key())) {
                m_roundFinishOrder.append(it.key());
                break;
            }
        }

        processRoundResults();
    }
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
        int redJokers = 0;
        for (const Card& card : fourthPlayer->getHandCards()) {
            if (card.point() == Card::Card_BJ) redJokers++;
        }
        canResistTribute = (redJokers >= 2);
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
        if (m_pendingTributes.size() >= 2) {
            // 双下情况：进贡大者先出牌
            if (m_pendingTributes[0].card > m_pendingTributes[1].card) {
                m_currentPlayerId = m_pendingTributes[0].fromPlayerId;
            }
            else {
                m_currentPlayerId = m_pendingTributes[1].fromPlayerId;
            }
        }
        else {
            // 单下情况：进贡者先出牌
            m_currentPlayerId = m_pendingTributes[0].fromPlayerId;
        }

        m_currentPhase = GamePhase::Playing;
        emit sigSetCurrentTurnPlayer(m_currentPlayerId, getPlayerById(m_currentPlayerId)->getName());
        emit sigEnablePlayerControls(m_currentPlayerId, true, false);
        return;
    }

    const TributeInfo& currentTribute = m_pendingTributes[m_currentTributeIndex];

    if (currentTribute.isReturn) {
        // 还贡阶段：自动选择合适的牌
        Player* returnPlayer = getPlayerById(currentTribute.fromPlayerId);
        Player* receivePlayer = getPlayerById(currentTribute.toPlayerId);

        if (returnPlayer && receivePlayer) {
            // 选择还贡的牌（给队友还小牌，给对手可以还任意牌）
            Team* returnTeam = getTeamOfPlayer(currentTribute.fromPlayerId);
            Team* receiveTeam = getTeamOfPlayer(currentTribute.toPlayerId);

            QVector<Card> hand = returnPlayer->getHandCards();
            Card returnCard;

            if (returnTeam == receiveTeam) {
                // 给队友：还10以下的牌
                for (const Card& card : hand) {
                    if (card.point() <= Card::Card_10) {
                        returnCard = card;
                        break;
                    }
                }
            }
            else {
                // 给对手：可以还任意牌，这里选择最小的
                if (!hand.isEmpty()) {
                    returnCard = hand[0];
                    for (const Card& card : hand) {
                        if (card < returnCard) {
                            returnCard = card;
                        }
                    }
                }
            }

            // 执行还贡
            TributeInfo tribute = currentTribute;
            tribute.card = returnCard;
            this->completeTribute(tribute);

            m_currentTributeIndex++;
            processNextTributeAction();
        }
    }
    else {
        // 进贡阶段：等待玩家选择
        m_currentPhase = GamePhase::TributeInput;

        Player* tributingPlayer = getPlayerById(currentTribute.fromPlayerId);
        Player* receivingPlayer = getPlayerById(currentTribute.toPlayerId);

        if (tributingPlayer && receivingPlayer) {
            emit sigAskForTribute(currentTribute.fromPlayerId, tributingPlayer->getName(),
                currentTribute.toPlayerId, receivingPlayer->getName(), false);
        }
    }
}

void GD_Controller::completeTribute(const TributeInfo& tribute)
{
    Player* fromPlayer = getPlayerById(tribute.fromPlayerId);
    Player* toPlayer = getPlayerById(tribute.toPlayerId);

    if (!fromPlayer || !toPlayer) {
        return;
    }

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
}

// ==================== 辅助方法 ====================

Player* GD_Controller::getPlayerById(int id) const
{
    // 通过QMap的映射关系查找
    auto it = m_players.find(id);
    return (it != m_players.end()) ? it.value() : nullptr;
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
        if (playerId != currentPlayerId && !m_passedPlayersInCircle.contains(playerId)) {
            return false;
        }
    }
    return true;
}