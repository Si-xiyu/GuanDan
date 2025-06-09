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

    // 确认/取消按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_confirmButton = new QPushButton(m_isReturn ? tr("还贡") : tr("进贡"), this);
    m_confirmButton->setEnabled(false);
    connect(m_confirmButton, &QPushButton::clicked, this, &TributeDialog::onConfirmClicked);
    btnLayout->addWidget(m_confirmButton);
    QPushButton* cancel = new QPushButton(tr("取消"), this);
    connect(cancel, &QPushButton::clicked, this, &TributeDialog::onCancelClicked);
    btnLayout->addWidget(cancel);
    mainLayout->addLayout(btnLayout);
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
    m_confirmButton->setEnabled(true);
}

void TributeDialog::onConfirmClicked()
{
    if (m_hasSelection) accept();
}

void TributeDialog::onCancelClicked()
{
    reject();
}

Card TributeDialog::getSelectedCard() const
{
    return m_selectedCard;
}

bool TributeDialog::hasValidSelection() const
{
    return m_hasSelection;
}
