#pragma once
#include <QDialog>
#include <QVector>
#include "Card.h"

class CardWidget;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;

class TributeDialog : public QDialog
{
    Q_OBJECT

public:
    // handCards: 可选择的手牌列表; isReturn: 是否还贡（true）或进贡（false）
    explicit TributeDialog(const QVector<Card>& handCards, bool isReturn, QWidget* parent = nullptr);
    ~TributeDialog();

    // 获取用户选择的牌
    Card getSelectedCard() const;
    bool hasValidSelection() const;

private slots:
    void onCardClicked(CardWidget* widget);
    void onConfirmClicked();
    void onCancelClicked();

private:
    void setupUI();
    QVector<Card> m_handCards;
    QVector<CardWidget*> m_cardWidgets;
    Card m_selectedCard;
    QPushButton* m_confirmButton;
    bool m_hasSelection = false;
    bool m_isReturn;
};

