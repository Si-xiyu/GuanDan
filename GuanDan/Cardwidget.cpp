#include <QPainter>
#include <QWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QDebug>
#include <QTransform>

#include "Cardwidget.h"
#include "Card.h"
#include "Player.h"

CardWidget::CardWidget(QWidget* parent)
    : QWidget(parent)
    , m_isfront(true)
    , m_isSelect(false)
    , m_isHovered(false)
    , m_rotation(0.0)
    , m_card()
    , m_owner(nullptr)
{
    setAttribute(Qt::WA_Hover);    // 启用悬停检测
    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);   // 设置尺寸
    loadCardImages(); // 加载图片
}

CardWidget::CardWidget(const Card& cardData, Player* owner, QWidget* parent)
    : QWidget(parent)
    , m_isfront(true)
    , m_isSelect(false)
    , m_isHovered(false)
    , m_rotation(0.0)
    , m_card(cardData)
    , m_owner(owner)
{
    setAttribute(Qt::WA_Hover); // 启用悬停事件
    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
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

// 卡片旋转功能
void CardWidget::setRotation(qreal angle)
{
    if (m_rotation != angle) {
        m_rotation = angle;
        update(); // 触发重绘
    }
}

qreal CardWidget::rotation() const
{
    return m_rotation;
}

// 图片绘制事件重写
void CardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 保存当前的变换矩阵
    painter.save();

    // 设置旋转中心点为卡片中心
    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_rotation);
    painter.translate(-width() / 2, -height() / 2);

    // 绘制卡片
    QPixmap currentImage = m_isfront ? m_front : m_back;
    QRect targetRect = rect();

    // 如果卡片被选中，绘制选中边框
    if (m_isSelect) {
        painter.setPen(QPen(SELECTED_BORDER_COLOR_CW, SELECTION_BORDER_SIZE_CW));
        painter.drawRect(targetRect.adjusted(SELECTION_BORDER_SIZE_CW/2, 
                                          SELECTION_BORDER_SIZE_CW/2,
                                          -SELECTION_BORDER_SIZE_CW/2, 
                                          -SELECTION_BORDER_SIZE_CW/2));
        
        // 绘制选中时的蒙版
        painter.fillRect(targetRect, SELECTED_TINT_COLOR_CW);
    }

    // 绘制卡片图像
    painter.drawPixmap(targetRect, currentImage);

    // 如果鼠标悬停，绘制悬停效果
    if (m_isHovered && !m_isSelect) {
        painter.fillRect(targetRect, HOVER_TINT_COLOR_CW);
    }

    // 恢复变换矩阵
    painter.restore();

    // 5. 如果是级牌，添加特殊标识
    if (m_isfront && m_card.isWildCard()) {
        // 在右上角绘制一个金色星星标记
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 215, 0)); // 金色

        int starSize = targetRect.width() / 4; // 星星大小为卡片宽度的1/4
        QRect starRect(targetRect.right() - starSize - 2, 2, starSize, starSize);

        // 绘制一个简单的五角星
        QPolygonF star;
        const double PI = 3.14159265358979323846;
        for (int i = 0; i < 5; ++i) {
            double angle = -PI / 2 + i * 4 * PI / 5;
            star << QPointF(
                starRect.center().x() + starRect.width() / 2 * cos(angle),
                starRect.center().y() + starRect.height() / 2 * sin(angle)
            );
            angle += 2 * PI / 5;
            star << QPointF(
                starRect.center().x() + starRect.width() / 4 * cos(angle),
                starRect.center().y() + starRect.height() / 4 * sin(angle)
            );
        }
        painter.drawPolygon(star);
    }
}

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 发射clicked 信号，由父控件HandCards处理选中逻辑
        // 父控件接收到信号后，会调用这个 CardWidget 的 setSelected() 方法
        emit clicked(this);
    }
    event->accept(); // 处理完事件后标记为已接受
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
    QString frontPath = QString(":/pic/res/%1.png").arg(imageName);

    qDebug() << "Trying to load card image:" << frontPath;
    qDebug() << "Card info - Suit:" << m_card.SuitToString() << "Point:" << m_card.PointToString();

    //加载正面图片
    if (!m_front.load(frontPath)) {
        qWarning() << "CardWidget: Failed to load front image:" << frontPath
            << "for card:" << imageName;
        
        // 尝试加载一个默认图片
        m_front = QPixmap(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
        m_front.fill(Qt::white);
        QPainter p(&m_front);
        p.drawRect(0, 0, CARD_WIDGET_WIDTH-1, CARD_WIDGET_HEIGHT-1);
        p.drawText(rect(), Qt::AlignCenter, imageName);
    }

    // 加载背面图片
    if (m_back.isNull()) {
        QString backPath = ":/pic/res/Back.png";
        qDebug() << "Trying to load back image:" << backPath;
        if (!m_back.load(backPath)) {
            qWarning() << "CardWidget: Failed to load back image";
            // 创建一个默认的背面图片
            m_back = QPixmap(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            m_back.fill(Qt::blue);
        }
    }
    update(); // 确保加载图片后刷新显示
}

