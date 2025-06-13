#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea> // 如果牌太多，用于手牌区域滚动
#include <QVector>
#include <QMap>
#include <QPixmap>

#include "Player.h"     // 玩家数据类
#include "Card.h"       // 卡牌数据类
#include "CardWidget.h" // 用于显示单张卡牌的控件

// 前向声明
class GD_Controller; // PlayerWidget可以直接向控制器发送信号

// 定义玩家视图的常量
const int PLAYER_WIDGET_MIN_WIDTH = 400;
const int PLAYER_WIDGET_MIN_HEIGHT = 150;
const int CARD_OVERLAP_HORIZONTAL = 30;
const int CARD_OVERLAP_VERTICAL = 20;
const int SIDE_CARD_OVERLAP = 20;
const int CARDS_MARGIN = 10;
const int NAME_LABEL_HEIGHT = 20;

// 玩家位置枚举
enum class PlayerPosition {
    Bottom,     // 下方玩家（当前玩家）
    Left,       // 左侧玩家
    Top,        // 上方玩家
    Right       // 右侧玩家
};

class PlayerWidget : public QWidget
{
    Q_OBJECT

public:
    PlayerWidget(Player* player, PlayerPosition position, bool isCurrentPlayer, QWidget* parent = nullptr);
    ~PlayerWidget();

    // 玩家信息相关
    void setPlayer(Player* player);
    Player* getPlayer() const;
    void setPlayerName(const QString& name);
    void updatePlayerInfo();

    // 卡片操作相关
    void updateCards(const QVector<Card>& cards);
    void removeCards(const QVector<Card>& cards);
    void addCards(const QVector<Card>& cards);
    void clearSelection();
    QVector<Card> getSelectedCards() const;
    void selectCards(const QVector<Card>& cardsToSelect);

    // 状态设置
    void setCardsVisible(bool visible);
    void setHighlighted(bool highlighted);
    void setEnabled(bool enabled);
    void setPlayerStatus(const QString& status);
    void setPosition(PlayerPosition position);
    PlayerPosition getPosition() const { return m_position; }

    // 显示更新
    void updateHandDisplay(const QVector<Card>& cards, bool showFront);
    void updateHandDisplayNoAnimation(const QVector<Card>& cards, bool showFront);
    void displayPlayedCombo(const QVector<Card>& cards);
    void clearPlayedCardsArea();

    // 设置玩家背景
    void setPlayerBackground(const QString& backgroundPath);
    void setDefaultBackground();

signals:
    // 当玩家选择了牌时发出信号
    void cardsSelected(const QVector<Card>& cards);
    void playCardsRequested();    // 出牌信号
    void skipTurnRequested();     // 跳过信号

private slots:
    // 处理卡牌点击事件
    void cardClicked(CardWidget* cardWidget);

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void contextMenuEvent(QContextMenuEvent* event) override;  // 添加右键菜单事件处理

private:
    // 重新布局所有卡片
    void relayoutCards();
    void relayoutCardsStatic();
    // 根据牌的大小顺序对卡片进行排序
    void sortCards();
    // 创建新的卡片视图
    CardWidget* createCardWidget(const Card& card);
    // 计算卡片的理想位置
    QPoint calculateCardPosition(int index, int stackIndex) const;
    // 获取相同点数的牌的数量
    int getStackSize(const Card& card) const;

    // 根据玩家位置计算合适的尺寸
    QSize calculatePreferredSize() const;
    // 根据玩家位置布局卡片
    void layoutCardsForBottom();
    void layoutCardsForTop();
    void layoutCardsForSide();
    
    // 更新卡片的Z顺序
    void updateCardZOrder();
    
    // 停止所有动画
    void stopAllAnimations();
    // 动画效果
    void animateCardsRemoval(const QVector<CardWidget*>& widgets);
    void animateCardsAddition(const QVector<CardWidget*>& widgets);

    Player* m_player;                     // 关联的玩家对象
    bool m_isCurrentPlayer;               // 是否是当前玩家
    bool m_isEnabled;                     // 是否可以操作
    bool m_isHighlighted;                 // 是否高亮显示
    QLabel* m_nameLabel;                  // 玩家名称标签
    QLabel* m_statusLabel;                // 玩家状态标签
    QVector<CardWidget*> m_cardWidgets;   // 所有卡片视图
    QMap<Card::CardPoint, QVector<CardWidget*>> m_cardStacks; // 按点数分组的卡片，用于在视图中堆叠显示

    // UI 元素
    QVBoxLayout* m_mainLayout;      // 主垂直布局
    QFrame* m_handFrame;            // 包裹手牌区域的框架，用于样式
    QHBoxLayout* m_handLayout;      // 用于排列 CardWidget 的水平布局
    QScrollArea* m_handScrollArea;  // 使手牌区域可滚动

    QVector<CardWidget*> m_playedCardWidgets; // 用于显示在"牌桌"上组合的 CardWidget

    PlayerPosition m_position;            // 玩家位置

    // 新增：背景相关
    QPixmap m_backgroundPixmap;
    QPixmap m_defaultBackgroundPixmap;
    bool m_useCustomBackground;

    // 新增：加载默认资源
    void loadDefaultResources();
};