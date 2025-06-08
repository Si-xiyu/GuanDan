#include "RankWidget.h"
#include "Player.h"
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QDebug>

// 样式常量定义
const QString RankWidget::STYLE_FRAME_NORMAL =
"QFrame { "
"border: 2px solid #CCCCCC; "
"border-radius: 8px; "
"background-color: #F8F8F8; "
"margin: 2px; "
"padding: 8px; "
"}";

const QString RankWidget::STYLE_FRAME_FINISHED =
"QFrame { "
"border: 2px solid #4CAF50; "
"border-radius: 8px; "
"background-color: #E8F5E8; "
"margin: 2px; "
"padding: 8px; "
"}";

const QString RankWidget::STYLE_FRAME_CURRENT =
"QFrame { "
"border: 3px solid #2196F3; "
"border-radius: 8px; "
"background-color: #E3F2FD; "
"margin: 2px; "
"padding: 8px; "
"}";

const QString RankWidget::STYLE_RANK_FIRST =
"QLabel { "
"color: #FFD700; "
"font-weight: bold; "
"font-size: 16px; "
"}";

const QString RankWidget::STYLE_RANK_SECOND =
"QLabel { "
"color: #C0C0C0; "
"font-weight: bold; "
"font-size: 14px; "
"}";

const QString RankWidget::STYLE_RANK_THIRD =
"QLabel { "
"color: #CD7F32; "
"font-weight: bold; "
"font-size: 14px; "
"}";

const QString RankWidget::STYLE_RANK_FOURTH =
"QLabel { "
"color: #808080; "
"font-weight: bold; "
"font-size: 12px; "
"}";

const QString RankWidget::STYLE_STATUS_PLAYING =
"QLabel { "
"color: #666666; "
"font-style: italic; "
"}";

const QString RankWidget::STYLE_STATUS_FINISHED =
"QLabel { "
"color: #4CAF50; "
"font-weight: bold; "
"}";

RankWidget::RankWidget(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_currentPlayerId(-1)
    , m_currentAnimation(nullptr)
    , m_glowEffect(nullptr)
{
    setupUI();
}

RankWidget::~RankWidget()
{
    if (m_currentAnimation) {
        m_currentAnimation->stop();
        delete m_currentAnimation;
    }
    if (m_glowEffect) {
        delete m_glowEffect;
    }
}

void RankWidget::setupUI()
{
    setFixedSize(280, 400);
    setWindowTitle("游戏排行榜");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // 标题
    m_titleLabel = new QLabel("排行榜", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "QLabel { "
        "font-size: 18px; "
        "font-weight: bold; "
        "color: #333333; "
        "margin-bottom: 10px; "
        "padding: 8px; "
        "background-color: #E0E0E0; "
        "border-radius: 5px; "
        "}"
    );
    m_mainLayout->addWidget(m_titleLabel);

    // 添加弹性空间，确保玩家信息居中显示
    m_mainLayout->addStretch();

    setLayout(m_mainLayout);

    // 设置整体样式
    setStyleSheet(
        "RankWidget { "
        "background-color: #FAFAFA; "
        "border: 1px solid #DDDDDD; "
        "border-radius: 10px; "
        "}"
    );
}

void RankWidget::setPlayers(const QMap<int, Player*>& players)
{
    m_players = players;
    m_playerInfos.clear();

    // 清除现有的玩家显示
    for (int i = m_mainLayout->count() - 1; i >= 0; --i) {
        QLayoutItem* item = m_mainLayout->itemAt(i);
        if (item && item->widget() && item->widget() != m_titleLabel) {
            delete item->widget();
        }
    }

    // 为每个玩家创建显示信息
    QVector<int> sortedPlayerIds;
    for (auto it = players.begin(); it != players.end(); ++it) {
        sortedPlayerIds.append(it.key());
    }
    std::sort(sortedPlayerIds.begin(), sortedPlayerIds.end());

    for (int playerId : sortedPlayerIds) {
        Player* player = players[playerId];
        if (player) {
            createPlayerRankInfo(playerId, player->getName());
        }
    }

    m_mainLayout->addStretch();
}

void RankWidget::createPlayerRankInfo(int playerId, const QString& playerName)
{
    PlayerRankInfo info;
    info.playerId = playerId;
    info.currentRank = 0;
    info.isFinished = false;

    // 创建主框架
    info.frame = new QFrame(this);
    info.frame->setFixedHeight(80);
    applyPlayerFrameStyle(info.frame, false, false);

    // 创建水平布局
    QHBoxLayout* hLayout = new QHBoxLayout(info.frame);
    hLayout->setContentsMargins(10, 5, 10, 5);
    hLayout->setSpacing(10);

    // 玩家姓名
    info.nameLabel = new QLabel(playerName, info.frame);
    info.nameLabel->setStyleSheet(
        "QLabel { "
        "font-size: 14px; "
        "font-weight: bold; "
        "color: #333333; "
        "}"
    );
    info.nameLabel->setFixedWidth(80);

    // 排名标签
    info.rankLabel = new QLabel("--", info.frame);
    info.rankLabel->setAlignment(Qt::AlignCenter);
    info.rankLabel->setFixedWidth(60);
    applyRankLabelStyle(info.rankLabel, 0);

    // 状态标签
    info.statusLabel = new QLabel("未出完牌", info.frame);
    info.statusLabel->setStyleSheet(STYLE_STATUS_PLAYING);
    info.statusLabel->setAlignment(Qt::AlignCenter);

    // 添加到布局
    hLayout->addWidget(info.nameLabel);
    hLayout->addWidget(info.rankLabel);
    hLayout->addWidget(info.statusLabel);
    hLayout->addStretch();

    // 保存信息并添加到主布局
    m_playerInfos[playerId] = info;
    m_mainLayout->insertWidget(m_mainLayout->count() - 1, info.frame);
}

void RankWidget::updateRanking(const QVector<int>& finishedOrder)
{
    m_finishedOrder = finishedOrder;

    qDebug() << "RankWidget::updateRanking - 更新排名，已完成玩家：" << finishedOrder;

    // 重新排列玩家框架顺序：先完成的，再未完成的
    // 移除所有玩家框架
    for (auto it = m_playerInfos.begin(); it != m_playerInfos.end(); ++it) {
        m_mainLayout->removeWidget(it.value().frame);
    }
    // 构建新的顺序：已完成->未完成
    QVector<int> orderedIds = m_finishedOrder;
    for (auto it = m_playerInfos.begin(); it != m_playerInfos.end(); ++it) {
        if (!m_finishedOrder.contains(it.key())) {
            orderedIds.append(it.key());
        }
    }
    // 依次插入框架
    for (int playerId : orderedIds) {
        m_mainLayout->insertWidget(m_mainLayout->count() - 1, m_playerInfos[playerId].frame);
    }

    // 首先重置所有玩家状态
    for (auto& info : m_playerInfos) {
        info.isFinished = false;
        info.currentRank = 0;
    }

    // 更新已完成的玩家
    for (int i = 0; i < finishedOrder.size(); ++i) {
        int playerId = finishedOrder[i];
        if (m_playerInfos.contains(playerId)) {
            int rank = i + 1;
            m_playerInfos[playerId].isFinished = true;
            m_playerInfos[playerId].currentRank = rank;
            updatePlayerDisplay(playerId, rank, true);
            animateRankChange(playerId);
        }
    }

    // 更新未完成的玩家
    for (auto it = m_playerInfos.begin(); it != m_playerInfos.end(); ++it) {
        if (!it.value().isFinished) {
            updatePlayerDisplay(it.key(), 0, false);
        }
    }
}

void RankWidget::updatePlayerDisplay(int playerId, int rank, bool isFinished)
{
    if (!m_playerInfos.contains(playerId)) {
        return;
    }

    PlayerRankInfo& info = m_playerInfos[playerId];

    // 更新排名显示
    info.rankLabel->setText(getRankText(rank));
    applyRankLabelStyle(info.rankLabel, rank);

    // 更新状态显示
    info.statusLabel->setText(getStatusText(isFinished, rank));
    info.statusLabel->setStyleSheet(isFinished ? STYLE_STATUS_FINISHED : STYLE_STATUS_PLAYING);

    // 更新框架样式
    bool isCurrent = (playerId == m_currentPlayerId);
    applyPlayerFrameStyle(info.frame, isFinished, isCurrent);

    qDebug() << "RankWidget::updatePlayerDisplay - 玩家" << playerId
        << "排名:" << rank << "完成:" << isFinished;
}

void RankWidget::resetRanking()
{
    m_finishedOrder.clear();
    m_currentPlayerId = -1;

    qDebug() << "RankWidget::resetRanking - 重置排名显示";

    for (auto& info : m_playerInfos) {
        info.isFinished = false;
        info.currentRank = 0;
        updatePlayerDisplay(info.playerId, 0, false);
    }

    clearHighlights();
}

void RankWidget::setCurrentPlayer(int playerId)
{
    if (m_currentPlayerId == playerId) {
        return;
    }

    // 清除之前的高亮
    clearHighlights();

    m_currentPlayerId = playerId;
    highlightCurrentPlayer(playerId);

    qDebug() << "RankWidget::setCurrentPlayer - 设置当前玩家:" << playerId;
}

void RankWidget::highlightCurrentPlayer(int playerId)
{
    if (!m_playerInfos.contains(playerId)) {
        return;
    }

    PlayerRankInfo& info = m_playerInfos[playerId];
    applyPlayerFrameStyle(info.frame, info.isFinished, true);

    // 添加发光效果
    if (!m_glowEffect) {
        m_glowEffect = new QGraphicsDropShadowEffect(this);
        m_glowEffect->setBlurRadius(15);
        m_glowEffect->setColor(QColor(33, 150, 243, 180));
        m_glowEffect->setOffset(0, 0);
    }

    info.frame->setGraphicsEffect(m_glowEffect);
}

void RankWidget::clearHighlights()
{
    for (auto& info : m_playerInfos) {
        applyPlayerFrameStyle(info.frame, info.isFinished, false);
        info.frame->setGraphicsEffect(nullptr);
    }

    if (m_glowEffect) {
        m_glowEffect->setParent(nullptr);
        m_glowEffect = nullptr;
    }
}

void RankWidget::animateRankChange(int playerId)
{
    if (!m_playerInfos.contains(playerId)) {
        return;
    }

    PlayerRankInfo& info = m_playerInfos[playerId];

    // 停止之前的动画
    if (m_currentAnimation) {
        m_currentAnimation->stop();
        delete m_currentAnimation;
    }

    // 创建缩放动画
    m_currentAnimation = new QPropertyAnimation(info.rankLabel, "geometry", this);
    m_currentAnimation->setDuration(500);
    m_currentAnimation->setEasingCurve(QEasingCurve::OutBounce);

    QRect originalGeometry = info.rankLabel->geometry();
    QRect scaledGeometry = originalGeometry;
    scaledGeometry.setWidth(scaledGeometry.width() * 1.2);
    scaledGeometry.setHeight(scaledGeometry.height() * 1.2);
    scaledGeometry.moveCenter(originalGeometry.center());

    m_currentAnimation->setStartValue(scaledGeometry);
    m_currentAnimation->setEndValue(originalGeometry);

    connect(m_currentAnimation, &QPropertyAnimation::finished,
        this, &RankWidget::onAnimationFinished);

    m_currentAnimation->start();
}

void RankWidget::onAnimationFinished()
{
    if (m_currentAnimation) {
        delete m_currentAnimation;
        m_currentAnimation = nullptr;
    }
}

QString RankWidget::getRankText(int rank) const
{
    switch (rank) {
    case 1: return "🥇 1st";
    case 2: return "🥈 2nd";
    case 3: return "🥉 3rd";
    case 4: return "4th";
    default: return "--";
    }
}

QString RankWidget::getStatusText(bool isFinished, int rank) const
{
    if (isFinished) {
        switch (rank) {
        case 1: return "头游";
        case 2: return "二游";
        case 3: return "三游";
        case 4: return "末游";
        default: return "已完成";
        }
    }
    else {
        return "未出完牌";
    }
}

void RankWidget::applyPlayerFrameStyle(QFrame* frame, bool isFinished, bool isCurrent)
{
    if (isCurrent) {
        frame->setStyleSheet(STYLE_FRAME_CURRENT);
    }
    else if (isFinished) {
        frame->setStyleSheet(STYLE_FRAME_FINISHED);
    }
    else {
        frame->setStyleSheet(STYLE_FRAME_NORMAL);
    }
}

void RankWidget::applyRankLabelStyle(QLabel* label, int rank)
{
    switch (rank) {
    case 1:
        label->setStyleSheet(STYLE_RANK_FIRST);
        break;
    case 2:
        label->setStyleSheet(STYLE_RANK_SECOND);
        break;
    case 3:
        label->setStyleSheet(STYLE_RANK_THIRD);
        break;
    case 4:
        label->setStyleSheet(STYLE_RANK_FOURTH);
        break;
    default:
        label->setStyleSheet(
            "QLabel { "
            "color: #999999; "
            "font-size: 12px; "
            "}"
        );
        break;
    }
}