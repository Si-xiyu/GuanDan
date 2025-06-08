#include "HMPlayer.h"
#include <QDebug>

// 构造函数，设置玩家类型为人类
HMPlayer::HMPlayer(const QString& name, int id)
    : Player(name, id)
{
    setType(Type::Human);
}

// 根据当前桌面牌型阻塞等待玩家操作，返回玩家选择的牌
QVector<Card> HMPlayer::choosePlay(const CardCombo::ComboInfo& currentTableCombo)
{
    Q_UNUSED(currentTableCombo);
    // 清空之前的选择
    m_selectedCards.clear();
    // 进入事件循环，等待玩家提交出牌或过牌
    m_eventLoop.exec();
    return m_selectedCards;
}

// 当UI点击出牌按钮时调用，发出playCardsSubmitted信号并退出事件循环
void HMPlayer::onPlayButtonClicked()
{
    qDebug() << "HMPlayer::onPlayButtonClicked - 提交出牌:" << m_selectedCards.size() << "张";
    emit playCardsSubmitted(m_selectedCards);
    m_eventLoop.quit();
}

// 当UI点击过牌按钮时调用，发出passSubmitted信号并退出事件循环
void HMPlayer::onPassButtonClicked()
{
    qDebug() << "HMPlayer::onPassButtonClicked - 提交过牌";
    emit passSubmitted();
    m_eventLoop.quit();
}

// 当UI选牌时调用，更新内部选择列表
void HMPlayer::onCardsSelected(const QVector<Card>& cards)
{
    qDebug() << "HMPlayer::onCardsSelected - 选中" << cards.size() << "张牌";
    m_selectedCards = cards;
}
