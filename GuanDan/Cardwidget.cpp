#include <QPainter>
#include <QWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QDebug>

#include "cardwidget.h"
#include "card.h"
#include "player.h"

CardWidget::CardWidget(QWidget* parent)
    : QWidget{ parent }
{
    m_isfront = true;
    m_isSelect = false;
    m_isHovered = false;

    setAttribute(Qt::WA_Hover);    // 启用悬停检测
    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);   // 设置尺寸
    loadCardImages(); // 加载图片
}

CardWidget::CardWidget(const Card& cardData, Player* owner, QWidget* parent)
    : QWidget(parent),
    m_card(cardData),
    m_owner(owner)
{
    m_isfront = true;
    m_isSelect = false;
    m_isHovered = false;

    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
    setAttribute(Qt::WA_Hover); // 启用悬停事件
    loadCardImages(); // 加载图片
}

QPixmap CardWidget::getImage()
{
    return m_front;
}

void CardWidget::setFrontSide(bool flag)
{
    m_isfront = flag;
}

bool CardWidget::isFrontSide()
{
    return m_isfront;
}

void CardWidget::setSelected(bool flag)
{
    if (m_isSelect != flag) {
        m_isSelect = flag;
        update(); // 选中状态改变，请求重绘
    }
}

bool CardWidget::isSelected()
{
    return m_isSelect;
}

void CardWidget::setCard(const Card& card)
{
    m_card = card;
}

Card CardWidget::getCard()
{
    return m_card;
}

void CardWidget::setOwner(Player* player)
{
    m_owner = player;
}

Player* CardWidget::getOwner()
{
    return m_owner;
}

//图片绘制事件重写
void CardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    QRect widgetRect = this->rect();
    QPixmap imageToDraw;

    // 1. 确定基础图像
    if (m_isfront) {
        imageToDraw = m_front;
    }
    else {
        imageToDraw = m_back;
    }

    // 2. 绘制基础图像
    if (!imageToDraw.isNull()) {
        // 保持宽高比缩放并居中绘制
        QPixmap scaledPixmap = imageToDraw.scaled(widgetRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (widgetRect.width() - scaledPixmap.width()) / 2;
        int y = (widgetRect.height() - scaledPixmap.height()) / 2;
        painter.drawPixmap(x, y, scaledPixmap);
    }
    else {
        // 图片加载失败或未设置时的占位符，显示卡牌的信息
        painter.fillRect(widgetRect, Qt::lightGray);
        painter.setPen(Qt::black);
        QString cardStr = m_isfront ? (m_card.SuitToString() + "_" + m_card.PointToString()) : "Back";
        painter.drawText(widgetRect, Qt::AlignCenter, cardStr + "\n(No Image)");
    }

    // 3. 绘制悬停效果 (深色蒙版)
    if (m_isHovered || m_isSelect) {
        painter.fillRect(widgetRect, HOVER_TINT_COLOR_CW); // 使用头文件中定义的颜色
    }

    // 4. 绘制选中边框
    if (m_isSelect) {
        painter.setPen(QPen(SELECTED_BORDER_COLOR_CW, SELECTION_BORDER_SIZE_CW));
        // 调整边框矩形，使边框线在widget边缘居中
        QRectF borderRect = QRectF(widgetRect).adjusted(
            (float)SELECTION_BORDER_SIZE_CW / 2.0f,
            (float)SELECTION_BORDER_SIZE_CW / 2.0f,
            -(float)SELECTION_BORDER_SIZE_CW / 2.0f,
            -(float)SELECTION_BORDER_SIZE_CW / 2.0f
        );
        painter.drawRoundedRect(borderRect, 5.0, 5.0); // 圆角效果
    }
}

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 发射clicked 信号，由父控件HandCards处理选中逻辑
        // 父控件接收到信号后，会调用这个 CardWidget 的 setSelected() 方法
        emit clicked(this);
    }
    QWidget::mousePressEvent(event); // 调用基类的实现，确保事件可以继续传播
}

void CardWidget::enterEvent(QEvent* event)
{
    if (!m_isHovered) {
        m_isHovered = true;
        update(); // 请求重绘以显示悬停效果
    }
    QWidget::enterEvent(event);
}

void CardWidget::leaveEvent(QEvent* event)
{
    if (m_isHovered) {
        m_isHovered = false;
        update(); // 请求重绘以移除悬停效果
    }
    QWidget::leaveEvent(event);
}

// 加载图片
void CardWidget::loadCardImages()
{
    QString imageName = m_card.getImageFilename();
    QString frontPath = QString(":/Card_image/res/%1.png").arg(imageName);

    //加载正面图片
    if (!m_front.load(frontPath)) {
        qWarning() << "CardWidget: Failed to load front image:" << frontPath
            << "for card:" << imageName;
    }

    // 加载背面图片
    if (m_back.isNull()) {
        if (!m_back.load(":/Card_image/res/Back.png")) {
            qWarning() << "CardWidget: Failed to load back image";
        }
    }
    update(); // 确保加载图片后刷新显示
}

