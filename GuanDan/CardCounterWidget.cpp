#include "CardCounterWidget.h"
#include <QDebug>

CardCounterWidget::CardCounterWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
{
    initializeUI();
    setupCardLabels();
}

CardCounterWidget::~CardCounterWidget()
{
}

void CardCounterWidget::initializeUI()
{
    // 设置半透明的深色背景和圆角边框
    setStyleSheet(
        "QWidget {"
        "   background-color: rgba(0, 0, 0, 0.5);"
        "   color: white;"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "}"
        "QLabel {"
        "   color: white;"
        "   font-size: 16px;"
        "   padding: 2px;"
        "   min-height: 15px;"
        "}"
    );

    // 创建网格布局
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(6);
    
    // 添加标题
    QLabel* titleLabel = new QLabel(tr("记牌器"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 18px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(titleLabel, 0, 0, 1, 2);
}

void CardCounterWidget::setupCardLabels()
{
    // 创建所有牌点数的标签
    int row = 1; // 从第1行开始，第0行是标题
    
    // 依次添加大王、小王和A-2的标签
    QVector<Card::CardPoint> allPoints = {
        Card::CardPoint::Card_BJ,  // 大王
        Card::CardPoint::Card_LJ,  // 小王
        Card::CardPoint::Card_A,
        Card::CardPoint::Card_K,
        Card::CardPoint::Card_Q,
        Card::CardPoint::Card_J,
        Card::CardPoint::Card_10,
        Card::CardPoint::Card_9,
        Card::CardPoint::Card_8,
        Card::CardPoint::Card_7,
        Card::CardPoint::Card_6,
        Card::CardPoint::Card_5,
        Card::CardPoint::Card_4,
        Card::CardPoint::Card_3,
        Card::CardPoint::Card_2
    };
    
    for (Card::CardPoint point : allPoints) {
        // 创建牌名称标签
        QLabel* nameLabel = new QLabel(getCardPointName(point), this);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        
        // 创建牌数量标签
        QLabel* countLabel = new QLabel("0", this);
        countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // 添加到布局
        m_layout->addWidget(nameLabel, row, 0);
        m_layout->addWidget(countLabel, row, 1);
        
        // 存储数量标签的映射关系
        m_countLabels[point] = countLabel;
        
        row++;
    }
}

QString CardCounterWidget::getCardPointName(Card::CardPoint point) const
{
    switch (point) {
    case Card::CardPoint::Card_2:  return tr("2");
    case Card::CardPoint::Card_3:  return tr("3");
    case Card::CardPoint::Card_4:  return tr("4");
    case Card::CardPoint::Card_5:  return tr("5");
    case Card::CardPoint::Card_6:  return tr("6");
    case Card::CardPoint::Card_7:  return tr("7");
    case Card::CardPoint::Card_8:  return tr("8");
    case Card::CardPoint::Card_9:  return tr("9");
    case Card::CardPoint::Card_10: return tr("10");
    case Card::CardPoint::Card_J:  return tr("J");
    case Card::CardPoint::Card_Q:  return tr("Q");
    case Card::CardPoint::Card_K:  return tr("K");
    case Card::CardPoint::Card_A:  return tr("A");
    case Card::CardPoint::Card_LJ: return tr("小 王");
    case Card::CardPoint::Card_BJ: return tr("大 王");
    default: return tr("未知");
    }
}

void CardCounterWidget::updateCounts(const QMap<Card::CardPoint, int>& counts)
{
    // 遍历传入的牌点数和数量映射
    QMapIterator<Card::CardPoint, int> it(counts);
    while (it.hasNext()) {
        it.next();
        Card::CardPoint point = it.key();
        int count = it.value();
        
        // 如果有对应的标签，则更新其文本
        if (m_countLabels.contains(point)) {
            QLabel* label = m_countLabels[point];
            label->setText(QString::number(count));
            
            // 如果数量为0，则设置文本颜色为红色，否则为白色
            if (count == 0) {
                label->setStyleSheet("color: red;");
            } else {
                label->setStyleSheet("color: white;");
            }
        }
    }
} 