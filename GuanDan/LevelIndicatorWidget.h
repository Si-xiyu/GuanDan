#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "Card.h"
#include "Cardwidget.h"

class LevelIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LevelIndicatorWidget(QWidget* parent = nullptr);
    ~LevelIndicatorWidget();

public slots:
    void updateLevels(Card::CardPoint team1Level, Card::CardPoint team2Level);

private:
    void setupUI();

    CardWidget* m_team1CardWidget;
    CardWidget* m_team2CardWidget;
    QLabel* m_team1Label;
    QLabel* m_team2Label;
};

