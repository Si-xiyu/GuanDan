#include "TributeDialog.h"

#include <QWidget>
#include "Cardwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include "Cardwidget.h"

TributeDialog::TributeDialog(const QVector<Card>& handCards, bool isReturn, QWidget* parent)
    : QDialog(parent)
    , m_handCards(handCards)
    , m_isReturn(isReturn)
{
    setupUI();
    setWindowTitle(m_isReturn ? tr("选择还贡的牌") : tr("选择进贡的牌"));
    setModal(true);
    setMinimumSize(400, 200);
}

TributeDialog::~TributeDialog() {}

void TributeDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* label = new QLabel(m_isReturn ? tr("请选择一张牌进行还贡：") : tr("请选择一张牌进行进贡："), this);
    mainLayout->addWidget(label);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    QWidget* scrollWidget = new QWidget(scroll);
    QHBoxLayout* cardsLayout = new QHBoxLayout(scrollWidget);
    cardsLayout->setSpacing(10);

    // 创建 CardWidget 列表
    for (const Card& card : m_handCards) {
        CardWidget* cw = new CardWidget(card, nullptr, scrollWidget);
        cw->setFrontSide(true);
        cw->setSelected(false);
        connect(cw, &CardWidget::clicked, this, &TributeDialog::onCardClicked);
        m_cardWidgets.append(cw);
        cardsLayout->addWidget(cw);
    }
    cardsLayout->addStretch();
    scrollWidget->setLayout(cardsLayout);
    scroll->setWidget(scrollWidget);
    mainLayout->addWidget(scroll);

    // 说明文字
    QLabel* infoLabel = new QLabel(tr("点击卡片直接选择"), this);
    infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel);
}

void TributeDialog::onCardClicked(CardWidget* widget)
{
    // 清除之前的选择
    for (CardWidget* cw : m_cardWidgets) {
        if (cw != widget) {
            cw->setSelected(false);
            cw->update(); // 强制更新视图
        }
    }
    
    // 设置当前选中的卡片
    widget->setSelected(true);
    widget->update(); // 强制更新视图
    
    m_selectedCard = widget->getCard();
    m_hasSelection = true;
    
    // 直接接受选择并关闭对话框
    accept();
}

Card TributeDialog::getSelectedCard() const
{
    return m_selectedCard;
}

bool TributeDialog::hasValidSelection() const
{
    return m_hasSelection;
}
