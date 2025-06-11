#include "TributeDialog.h"

#include <QWidget>
#include "Cardwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <algorithm>

TributeDialog::TributeDialog(const QVector<Card>& handCards, bool isReturn, bool isToTeammate, QWidget* parent)
    : QDialog(parent)
    , m_handCards(handCards)
    , m_isReturn(isReturn)
    , m_isToTeammate(isToTeammate)
    , m_confirmButton(nullptr)
    , m_infoLabel(nullptr)
{
    setupUI();
    setWindowTitle(m_isReturn ? tr("选择还贡的牌") : tr("选择进贡的牌"));
    setModal(true);
    setMinimumSize(400, 300);
    
    // 禁用关闭按钮
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    
    // 禁止通过ESC键关闭
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowModality(Qt::ApplicationModal);
}

TributeDialog::~TributeDialog() {}

void TributeDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 标题标签
    QString titleText;
    if (m_isReturn) {
        titleText = m_isToTeammate ? 
            tr("请选择一张10或以下的牌还贡给队友：") : 
            tr("请选择一张牌还贡：");
    } else {
        titleText = tr("请选择您手牌中最大的牌进贡：");
    }
    QLabel* label = new QLabel(titleText, this);
    mainLayout->addWidget(label);

    // 卡片滚动区域
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

    // 信息标签
    m_infoLabel = new QLabel(tr("请选择一张牌"), this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet("QLabel { color: blue; }");
    mainLayout->addWidget(m_infoLabel);

    // 确认按钮
    m_confirmButton = new QPushButton(tr("确认选择"), this);
    m_confirmButton->setEnabled(false);
    connect(m_confirmButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(m_confirmButton);
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
    
    // 验证选择
    validateSelection();
}

void TributeDialog::validateSelection()
{
    bool isValid = false;
    QString message;

    if (m_isReturn) {
        // 还贡规则：如果是还贡给队友，牌必须是10或以下
        if (m_isToTeammate && m_selectedCard.point() > Card::Card_10) {
            message = tr("错误：还贡给队友的牌必须是10或以下的牌！");
        } else {
            isValid = true;
            message = tr("选择有效，请点击确认按钮");
        }
    } else {
        // 进贡规则：必须是手牌中最大的牌
        Card largestCard = findLargestCard();
        if (m_selectedCard < largestCard) {
            message = tr("错误：进贡必须选择手牌中最大的牌！");
        } else {
            isValid = true;
            message = tr("选择有效，请点击确认按钮");
        }
    }

    // 更新UI状态
    m_confirmButton->setEnabled(isValid);
    m_infoLabel->setText(message);
    m_infoLabel->setStyleSheet(isValid ? 
        "QLabel { color: green; }" : 
        "QLabel { color: red; }");
}

Card TributeDialog::findLargestCard() const
{
    if (m_handCards.isEmpty()) {
        return Card(); // 返回一个无效的牌
    }
    
    Card largest = m_handCards.first();
    for (const Card& card : m_handCards) {
        if (card > largest) {
            largest = card;
        }
    }
    return largest;
}

Card TributeDialog::getSelectedCard() const
{
    return m_selectedCard;
}


