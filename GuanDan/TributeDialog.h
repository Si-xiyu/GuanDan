#pragma once
#include <QDialog>
#include <QVector>
#include "Card.h"

class CardWidget;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QScrollArea;
class QPushButton;

class TributeDialog : public QDialog
{
    Q_OBJECT

public:
    // handCards: 可选择的手牌列表; isReturn: 是否还贡（true）或进贡（false）; isToTeammate: 是否还贡给队友
    explicit TributeDialog(const QVector<Card>& handCards, bool isReturn, bool isToTeammate = false, QWidget* parent = nullptr);
    ~TributeDialog();

    // 获取用户选择的牌
    Card getSelectedCard() const;

private slots:
    void onCardClicked(CardWidget* widget);
    void validateSelection();

private:
    void setupUI();
    Card findLargestCard() const;
    
    QVector<Card> m_handCards;
    QVector<CardWidget*> m_cardWidgets;
    Card m_selectedCard;
    bool m_isReturn;
    bool m_isToTeammate;
    
    QPushButton* m_confirmButton;
    QLabel* m_infoLabel;
};

