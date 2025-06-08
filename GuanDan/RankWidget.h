#ifndef RANKWIDGET_H
#define RANKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMap>
#include <QVector>
#include <QPropertyAnimation>
#include <QGraphicsEffect>

class Player;

class RankWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RankWidget(QWidget* parent = nullptr);
    ~RankWidget();

    // 设置玩家信息
    void setPlayers(const QMap<int, Player*>& players);

    // 更新排名显示
    void updateRanking(const QVector<int>& finishedOrder);

    // 重置排名（新局开始时调用）
    void resetRanking();

    // 设置当前出牌玩家高亮
    void setCurrentPlayer(int playerId);

private slots:
    void onAnimationFinished();

private:
    struct PlayerRankInfo {
        QFrame* frame;           // 玩家信息框架
        QLabel* nameLabel;       // 玩家姓名
        QLabel* rankLabel;       // 排名标签
        QLabel* statusLabel;     // 状态标签
        int playerId;            // 玩家ID
        int currentRank;         // 当前排名（0表示未完成）
        bool isFinished;         // 是否已出完牌
    };

    void setupUI();
    void createPlayerRankInfo(int playerId, const QString& playerName);
    void updatePlayerDisplay(int playerId, int rank, bool isFinished);
    void highlightCurrentPlayer(int playerId);
    void clearHighlights();
    void animateRankChange(int playerId);

    // 获取排名显示文本
    QString getRankText(int rank) const;
    QString getStatusText(bool isFinished, int rank) const;

    // 样式相关
    void applyPlayerFrameStyle(QFrame* frame, bool isFinished, bool isCurrent);
    void applyRankLabelStyle(QLabel* label, int rank);

private:
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;

    QMap<int, PlayerRankInfo> m_playerInfos;  // 玩家ID -> 排名信息
    QMap<int, Player*> m_players;             // 玩家ID -> 玩家对象

    int m_currentPlayerId;                    // 当前出牌玩家ID
    QVector<int> m_finishedOrder;            // 已完成玩家的顺序

    // 动画相关
    QPropertyAnimation* m_currentAnimation;
    QGraphicsEffect* m_glowEffect;

    // 样式常量
    static const QString STYLE_FRAME_NORMAL;
    static const QString STYLE_FRAME_FINISHED;
    static const QString STYLE_FRAME_CURRENT;
    static const QString STYLE_RANK_FIRST;
    static const QString STYLE_RANK_SECOND;
    static const QString STYLE_RANK_THIRD;
    static const QString STYLE_RANK_FOURTH;
    static const QString STYLE_STATUS_PLAYING;
    static const QString STYLE_STATUS_FINISHED;
};

#endif // RANKWIDGET_H