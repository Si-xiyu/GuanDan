#include <QPainter>
#include <QWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QDebug>
#include <QTransform>
#include <QResizeEvent>

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
    , m_zValue(0)
{
    setAttribute(Qt::WA_Hover);    // 启用悬停检测
    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);   // 设置尺寸
    loadCardImages(); // 加载图片
    setMouseTracking(true);
}

CardWidget::CardWidget(const Card& cardData, Player* owner, QWidget* parent)
    : QWidget(parent)
    , m_isfront(true)
    , m_isSelect(false)
    , m_isHovered(false)
    , m_rotation(0.0)
    , m_card(cardData)
    , m_owner(owner)
    , m_zValue(0)
{
    setAttribute(Qt::WA_Hover); // 启用悬停事件
    setFixedSize(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
    loadCardImages(); // 加载图片
    setMouseTracking(true);
}

CardWidget::~CardWidget()
{
    // 析构函数，清理资源
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
        if (m_isSelect) {
            // 如果被选中，保持当前的悬停效果
            m_isHovered = true;
        } else {
            // 如果取消选中，也取消悬停效果
            m_isHovered = false;
        }
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

    // 绘制卡片图像
    painter.drawPixmap(targetRect, currentImage);

    // 如果卡片被选中或悬停，绘制效果
    if (m_isSelect || m_isHovered) {
        // 绘制边框
        painter.setPen(QPen(SELECTED_BORDER_COLOR_CW, SELECTION_BORDER_SIZE_CW));
        painter.drawRect(targetRect.adjusted(SELECTION_BORDER_SIZE_CW/2, 
                                          SELECTION_BORDER_SIZE_CW/2,
                                          -SELECTION_BORDER_SIZE_CW/2, 
                                          -SELECTION_BORDER_SIZE_CW/2));
        
        // 绘制蒙版，选中和悬停使用相同的效果
        painter.fillRect(targetRect, HOVER_TINT_COLOR_CW);
    }

    // 恢复变换矩阵
    painter.restore();

    // 5. 如果是级牌，添加特殊标识
    if (m_isfront && m_card.isWildCard()) {
        // 级牌特殊：绘制金色边框
        QPen pen(QColor(255, 215, 0)); // 金色
        pen.setWidth(5);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        // 在卡片边缘绘制边框
        QRect borderRect = targetRect.adjusted(pen.width()/2,
                                               pen.width()/2,
                                               -pen.width()/2,
                                               -pen.width()/2);
        painter.drawRect(borderRect);
    }
}

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    //qDebug() << "CardWidget::mousePressEvent - 点击卡牌:" 
    //         << m_card.PointToString() << m_card.SuitToString()
    //         << "启用状态:" << isEnabled()
    //         << "Z值:" << zValue()
    //         << "位置:" << pos()
    //         << "父控件:" << (parentWidget() ? parentWidget()->metaObject()->className() : "无")
    //         << "可见性:" << isVisible()
    //         << "大小:" << size();
             
    if (!isEnabled()) {
        qDebug() << "卡牌未启用，忽略点击事件";
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        // 不在这里切换选择状态，而是由外部控制
        // setSelected(!m_isSelect);  // 移除这行
        emit clicked(this);
    }
}

void CardWidget::enterEvent(QEvent* event)
{
    if (!m_isSelect && !m_isHovered) {  // 只有未选中的卡片才响应悬停
        m_isHovered = true;
        update(); // 请求重绘以显示悬停效果
    }
    QWidget::enterEvent(event);
}

void CardWidget::leaveEvent(QEvent* event)
{
    if (!m_isSelect && m_isHovered) {  // 只有未选中的卡片才响应离开
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

    //qDebug() << "Trying to load card image:" << frontPath;
    //qDebug() << "Card info - Suit:" << m_card.SuitToString() << "Point:" << m_card.PointToString();

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
        //qDebug() << "Trying to load back image:" << backPath;
        if (!m_back.load(backPath)) {
            qWarning() << "CardWidget: Failed to load back image";
            // 创建一个默认的背面图片
            m_back = QPixmap(CARD_WIDGET_WIDTH, CARD_WIDGET_HEIGHT);
            m_back.fill(Qt::blue);
        }
    }
    update(); // 确保加载图片后刷新显示
}

void CardWidget::setZValue(int z)
{
    if (m_zValue != z) {
        m_zValue = z;
        // 使用raise或lower来模拟Z值变化
        if (parentWidget()) {
            // 先将控件移到最底层
            lower();
            // 然后根据需要提升
            for (int i = 0; i < z; ++i) {
                raise();
            }
        }
    }
}

int CardWidget::zValue() const
{
    return m_zValue;
}

void CardWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // 在大小变化时重新加载和缩放图片
    if (m_isfront) {
        // 缩放正面图片以适应新大小
        if (!m_front.isNull()) {
            QPixmap scaledFront = m_front.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            // 如果需要，可以在这里更新内部存储的图片
        }
    } else {
        // 缩放背面图片以适应新大小
        if (!m_back.isNull()) {
            QPixmap scaledBack = m_back.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            // 如果需要，可以在这里更新内部存储的图片
        }
    }
    
    // 记录调试信息
    //qDebug() << "CardWidget::resizeEvent - 卡牌:" 
    //         << m_card.PointToString() << m_card.SuitToString()
    //         << "新大小:" << size();
}

