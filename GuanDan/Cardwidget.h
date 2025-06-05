#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QWidget>

#include "card.h"
#include "player.h"

// 定义图片尺寸
const int CARD_WIDGET_WIDTH = 56;
const int CARD_WIDGET_HEIGHT = 78;
const int SELECTION_BORDER_SIZE_CW = 2;
const QColor HOVER_TINT_COLOR_CW = QColor(0, 0, 0, 50); // 悬停蒙版颜色
const QColor SELECTED_BORDER_COLOR_CW = Qt::blue; // 选中边框颜色
const QColor SELECTED_TINT_COLOR_CW = QColor(0, 0, 0, 70); // 选中蒙版颜色

class CardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CardWidget(QWidget* parent = nullptr);
    explicit CardWidget(const Card& cardData, Player* owner = nullptr, QWidget* parent = nullptr);

    QPixmap getImage();
    //正反面的设置
    void setFrontSide(bool flag);
    bool isFrontSide();
    //是否被选中的设置
    void setSelected(bool flag);
    bool isSelected();
    //卡片内容的设置
    void setCard(const Card& card);
    Card getCard();
    //所属于玩家的设置
    void setOwner(Player* player);
    Player* getOwner();

protected:
    //重写事件处理函数
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void enterEvent(QEvent* event);   // 添加悬停效果
    void leaveEvent(QEvent* event);

private:
    void loadCardImages(); // 私有辅助函数加载图片

    //QPixmap储存卡片图像
    QPixmap m_front;
    QPixmap m_back;
    //记录卡片状态
    bool m_isfront;
    bool m_isSelect;
    bool m_isHovered;

    Card m_card;
    Player* m_owner = nullptr;

signals:
    void clicked(CardWidget* widget);   //发送卡牌被选中信号
};

#endif // CARDWIDGET_H
