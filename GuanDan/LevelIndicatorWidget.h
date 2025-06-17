#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "Card.h"
#include "Cardwidget.h"
#include "ui_LevelIndicatorWidget.h"

class LevelIndicatorWidget : public QWidget
{
	Q_OBJECT

public:
	LevelIndicatorWidget(QWidget *parent = nullptr);
	~LevelIndicatorWidget();

public slots:
	// 更新队伍级牌显示
	void updateLevels(Card::CardPoint team1Level, Card::CardPoint team2Level);

private:
	Ui::LevelIndicatorWidgetClass ui;

	// 设置UI布局和元素
	void setupUI();

	// ---UI元素---
	CardWidget* m_team1CardWidget;
	CardWidget* m_team2CardWidget;
	QLabel* m_team1Label;
	QLabel* m_team2Label;
};

