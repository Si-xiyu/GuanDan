#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QVector>
#include "Card.h"

class Team;

class Player : public QObject
{
    Q_OBJECT
public:
    explicit Player(QString name, int id, Team* team = nullptr, QObject* parent = nullptr)
        : QObject(parent), m_name(name), m_id(id), m_team(team)
    {
    }

    // 玩家类型：人类或AI
    enum Type { Human, AI, Unknown };

    // 基础信息
    void setName(QString name);
    QString getName() const;
    void setType(Type type);
    Type getType() const;
    void setId(int Id);

    // 手牌管理
    void addCards(const QVector<Card>& cards);
    void removeCards(const QVector<Card>& cards);
    QVector<Card> getHandCards() const;

    // 所属队伍
    void setTeam(Team* team);
    Team* getTeam() const;

    // 游戏状态
    void setReady(bool ready);
    bool isReady() const;

    // 出牌验证（调用CardCombo类）
    bool canPlayCards(const QVector<Card>& cards,
        int current_table_combo_type,
        int current_table_combo_level) const;

signals:
    void cardsUpdated(); // 手牌变化信号
    void playerReady(bool isReady); // 准备状态变化信号

private:
    QString m_name; // 玩家名称
    Type m_type = Unknown; // 玩家类型，初始为Unknown
    int m_id; // 玩家id

    QVector<Card> m_handCards;    // 手牌
    Team* m_team = nullptr;       // 所属队伍
    bool m_isReady = false;       // 是否准备就绪
};

#endif // PLAYER_H
