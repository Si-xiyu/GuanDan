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
        "   min-height: 14px;"
        "   background-color: transparent;"
        "}"
    );

    // 创建网格布局
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(6);
    
    // 添加标题
    QLabel* titleLabel = new QLabel(tr("记牌器"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 18px; background: transparent;");
    titleLabel->setAlignment(Qt::AlignCenter);

	// 设置标题标签的字体和对齐方式
    m_layout->addWidget(titleLabel, 0, 0, 1, 4);

    // 第0列 (左侧牌名) 和 第2列 (右侧牌名) 设置相同的拉伸因子
    m_layout->setColumnStretch(0, 1);
    m_layout->setColumnStretch(2, 1);

    // 第1列 (左侧数量) 和 第3列 (右侧数量) 不拉伸，保持内容宽度
    m_layout->setColumnStretch(1, 0);
    m_layout->setColumnStretch(3, 0);
}

void CardCounterWidget::setupCardLabels()
{
    QVector<Card::CardPoint> allPoints = {
        Card::CardPoint::Card_BJ, Card::CardPoint::Card_LJ, Card::CardPoint::Card_A,
        Card::CardPoint::Card_K, Card::CardPoint::Card_Q, Card::CardPoint::Card_J,
        Card::CardPoint::Card_10, Card::CardPoint::Card_9, Card::CardPoint::Card_8,
        Card::CardPoint::Card_7, Card::CardPoint::Card_6, Card::CardPoint::Card_5,
        Card::CardPoint::Card_4, Card::CardPoint::Card_3, Card::CardPoint::Card_2
    };

    const int numPoints = allPoints.size();
    // 计算中点，用于分列
    const int firstColCount = (numPoints + 1) / 2; // 8项放左边, 7项放右边

    for (int i = 0; i < numPoints; ++i) {
        Card::CardPoint point = allPoints[i];

        int row, col;
        // 根据索引i判断牌点应该放在左侧还是右侧
        if (i < firstColCount) {
            // 左侧两列 (列0: 牌名, 列1: 数量)
            row = 1 + i;
            col = 0;
        }
        else {
            // 右侧两列 (列2: 牌名, 列3: 数量)
            row = 1 + (i - firstColCount);
            col = 2;
        }

        QLabel* nameLabel = new QLabel(getCardPointName(point), this);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QLabel* countLabel = new QLabel("0", this);
        countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        countLabel->setStyleSheet("background-color: white; color: black; border-radius: 4px; padding: 1px 4px;");
        countLabel->setFixedWidth(24);

        // 添加到计算出的新行列位置
        m_layout->addWidget(nameLabel, row, col);
        m_layout->addWidget(countLabel, row, col + 1);

        m_countLabels[point] = countLabel;
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
            
            // 如果数量为0，则设置文本颜色为红色，否则为黑色，但保持白色背景
            if (count == 0) {
                label->setStyleSheet("background-color: white; color: red; border-radius: 4px; padding: 1px 4px;");
            } else {
                label->setStyleSheet("background-color: white; color: black; border-radius: 4px; padding: 1px 4px;");
            }
        }
    }
} 