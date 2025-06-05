#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea> // 如果牌太多，用于手牌区域滚动
#include <QVector>
#include <QMap>

#include "Player.h"     // 玩家数据类
#include "Card.h"       // 卡牌数据类
#include "CardWidget.h" // 用于显示单张卡牌的控件

// 前向声明
class GD_Controller; // PlayerWidget可以直接向控制器发送信号

// 定义玩家视图的常量
const int PLAYER_WIDGET_MIN_WIDTH = 600;
const int PLAYER_WIDGET_MIN_HEIGHT = 150;
const int CARD_OVERLAP_HORIZONTAL = 30;  // 卡片水平重叠的像素数
const int CARD_OVERLAP_VERTICAL = 20;    // 相同牌垂直重叠的像素数
const int SIDE_CARD_OVERLAP = 15;        // 侧面玩家牌的重叠像素数
const int CARDS_MARGIN = 10;             // 卡片区域的边距
const int NAME_LABEL_HEIGHT = 25;        // 玩家名称标签的高度

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
    explicit PlayerWidget(Player* player, PlayerPosition position = PlayerPosition::Bottom, 
                        bool isCurrentPlayer = false, QWidget* parent = nullptr);
    PlayerWidget(Player* player, bool isCurrentPlayer, QWidget* parent);
    ~PlayerWidget();

    // 更新玩家手牌
    void updateCards(const QVector<Card>& cards);
    // 移除指定的牌
    void removeCards(const QVector<Card>& cards);
    // 添加新的牌
    void addCards(const QVector<Card>& cards);
    // 获取当前选中的牌
    QVector<Card> getSelectedCards() const;
    // 清除所有选中状态
    void clearSelection();
    // 设置是否可以操作
    void setEnabled(bool enabled);
    // 设置是否显示牌面
    void setCardsVisible(bool visible);
    // 设置玩家状态（准备、思考中等）
    void setPlayerStatus(const QString& status);
    // 高亮显示玩家（轮到该玩家出牌时）
    void setHighlighted(bool highlighted);

    void setPlayer(Player* player); // 将此控件与一个玩家数据对象关联
    Player* getPlayer() const;

    // 设置玩家位置
    void setPosition(PlayerPosition position);
    PlayerPosition getPosition() const { return m_position; }

    // UI 更新槽函数 (由 GameWindow 或 Controller 连接)
public slots:
    void updateHandDisplay(const QVector<Card>& handCards, bool showCardFronts); // 更新手牌显示
    void setPlayerName(const QString& name);
    void highlightTurn(bool isCurrentTurn);      // 如果轮到此玩家，高亮显示
    void clearPlayedCardsArea();                // 清空此玩家打出的牌的区域
    void displayPlayedCombo(const QVector<Card>& cards); // 显示此玩家刚刚打出的牌组合

signals:
    // 当玩家选择了牌时发出信号
    void cardsSelected(const QVector<Card>& cards);
    // 当玩家点击了某张牌时发出信号
    void cardClicked(CardWidget* cardWidget);

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

private:
    // 重新布局所有卡片
    void relayoutCards();
    // 根据牌的大小顺序对卡片进行排序
    void sortCards();
    // 创建新的卡片视图
    CardWidget* createCardWidget(const Card& card);
    // 更新玩家信息显示
    void updatePlayerInfo();
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

    Player* m_player;                     // 关联的玩家对象
    bool m_isCurrentPlayer;               // 是否是当前玩家
    bool m_isEnabled;                     // 是否可以操作
    bool m_isHighlighted;                 // 是否高亮显示
    QLabel* m_nameLabel;                  // 玩家名称标签
    QLabel* m_statusLabel;                // 玩家状态标签
    QVector<CardWidget*> m_cardWidgets;   // 所有卡片视图
    QMap<Card::CardPoint, QVector<CardWidget*>> m_cardStacks; // 按点数分组的卡片

    // UI 元素
    QVBoxLayout* m_mainLayout;      // 主垂直布局
    QFrame* m_handFrame;            // 包裹手牌区域的框架，用于样式
    QHBoxLayout* m_handLayout;      // 用于排列 CardWidget 的水平布局
    QScrollArea* m_handScrollArea;  // 使手牌区域可滚动

    QList<CardWidget*> m_playedCardWidgets; // 用于显示在"牌桌"上组合的 CardWidget

    bool m_isCurrentTurn; // 用于样式控制，标记是否轮到此玩家
    PlayerPosition m_position;            // 玩家位置
};

#endif // PLAYERWIDGET_H