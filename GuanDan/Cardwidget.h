#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QWidget>

#include "Card.h"
#include "Player.h"

// 定义图片尺寸
const int CARD_WIDGET_WIDTH = 90;
const int CARD_WIDGET_HEIGHT = 125;
const int SELECTION_BORDER_SIZE_CW = 2;
const QColor HOVER_TINT_COLOR_CW = QColor(0, 0, 0, 50); // 悬停蒙版颜色
const QColor SELECTED_BORDER_COLOR_CW = Qt::blue; // 选中边框颜色
const QColor SELECTED_TINT_COLOR_CW = QColor(0, 0, 0, 70); // 选中蒙版颜色

class CardWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

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

    // 旋转相关方法
    void setRotation(qreal angle);
    qreal rotation() const;

protected:
    //重写事件处理函数
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;   // 添加悬停效果
    void leaveEvent(QEvent* event) override;

private:
    void loadCardImages(); // 私有辅助函数加载图片

    //QPixmap储存卡片图像
    QPixmap m_front;
    QPixmap m_back;
    //记录卡片状态
    bool m_isfront;
    bool m_isSelect;
    bool m_isHovered;
    qreal m_rotation;  // 存储旋转角度

    Card m_card;
    Player* m_owner = nullptr;

signals:
    void clicked(CardWidget* widget);   //发送卡牌被选中信号
};

#endif // CARDWIDGET_H
