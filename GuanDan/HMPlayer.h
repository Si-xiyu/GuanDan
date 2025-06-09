//#pragma once
//
//#include <QObject>
//#include <QEventLoop>
//#include "Player.h"
//#include "Cardcombo.h"
//
//// HMPlayer 继承自 Player，用于处理人类玩家的UI交互和出牌逻辑
//class HMPlayer : public Player
//{
//    Q_OBJECT
//
//public:
//    // 构造函数: name 玩家名字, id 玩家ID
//    explicit HMPlayer(const QString& name, int id);
//
//    // 重写 AI/人类统一出牌接口
//    QVector<Card> choosePlay(const CardCombo::ComboInfo& currentTableCombo) override;
//
//signals:
//    // 当玩家通过 UI 提交出牌或过牌时发出
//    void playCardsSubmitted(const QVector<Card>& cards);
//    void passSubmitted();
//    // 请求提示信号
//    void requestHint();
//
//public slots:
//    // 响应 UI 操作
//    void onPlayButtonClicked();
//    void onPassButtonClicked();
//    void onCardsSelected(const QVector<Card>& cards);
//
//private:
//    // 存储玩家在 UI 中选中的卡牌
//    QVector<Card> m_selectedCards;
//    // 阻塞等待玩家提交的事件循环
//    QEventLoop m_eventLoop;
//};
//
