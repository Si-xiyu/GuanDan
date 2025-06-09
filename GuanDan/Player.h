#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QVector>
#include "Card.h"
#include "Cardcombo.h"

class Team;
class GD_Controller;

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
    int getID();

    // 手牌管理
    void addCards(const QVector<Card>& cards);
    void removeCards(const QVector<Card>& cards);
    void clearHandCards(); // 清空所有手牌
    QVector<Card> getHandCards() const;

    // 所属队伍
    void setTeam(Team* team);
    Team* getTeam() const;

    // 游戏状态
    void setReady(bool ready);
    bool isReady() const;

    // 出牌验证（调用CardCombo类）
    bool canPlayCards(const QVector<Card>& cards, CardCombo::ComboInfo& current_table) const;

    // 玩家回合自动行为：默认无操作，人类等待UI，AI在子类中重写出牌逻辑
    virtual void autoPlay(GD_Controller* controller, const CardCombo::ComboInfo& currentTableCombo) { }

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
