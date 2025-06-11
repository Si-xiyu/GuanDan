#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "PlayerWidget.h"
#include "ShowCardWidget.h"
#include "CardCombo.h"

class PlayerAreaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerAreaWidget(Player* player, PlayerPosition position, bool isCurrentPlayer, QWidget* parent = nullptr);
    ~PlayerAreaWidget();

    // 代理PlayerWidget的主要方法，使其对外接口保持一致
    void setPlayer(Player* player);
    Player* getPlayer() const;
    void setPlayerName(const QString& name);
    void updatePlayerInfo();
    void updateCards(const QVector<Card>& cards);
    void removeCards(const QVector<Card>& cards);
    void addCards(const QVector<Card>& cards);
    void clearSelection();
    QVector<Card> getSelectedCards() const;
    void setCardsVisible(bool visible);
    void setHighlighted(bool highlighted);
    void setEnabled(bool enabled);
    void setPlayerStatus(const QString& status);
    void setPosition(PlayerPosition position);
    PlayerPosition getPosition() const;
    void setPlayerBackground(const QString& backgroundPath);
    void setDefaultBackground();
    
    // 添加手牌显示更新方法
    void updateHandDisplay(const QVector<Card>& cards, bool showFront);
    void updateHandDisplayNoAnimation(const QVector<Card>& cards, bool showFront);

    // 获取内部控件
    PlayerWidget* getPlayerWidget() const { return m_playerWidget; }
    ShowCardWidget* getShowCardWidget() const { return m_showCardWidget; }

public slots:
    // 更新显示的出牌
    void updatePlayedCards(const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards);
    // 清空显示的出牌
    void clearPlayedCards();

signals:
    // 转发PlayerWidget的信号
    void cardsSelected(const QVector<Card>& cards);
    void playCardsRequested();
    void skipTurnRequested();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;

private:
    // 根据位置布局组件
    void arrangeComponents();

    PlayerWidget* m_playerWidget;       // 玩家控件
    ShowCardWidget* m_showCardWidget;   // 显示出牌控件
    PlayerPosition m_position;          // 玩家位置
    QBoxLayout* m_mainLayout;           // 主布局（根据位置可能是水平或垂直）
}; 