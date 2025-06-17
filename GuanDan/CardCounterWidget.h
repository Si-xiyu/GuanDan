#ifndef CARDCOUNTERWIDGET_H
#define CARDCOUNTERWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include "Card.h"

class CardCounterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CardCounterWidget(QWidget* parent = nullptr);
    ~CardCounterWidget();

public slots:
    void updateCounts(const QMap<Card::CardPoint, int>& counts);

private:
	void initializeUI(); // 初始化UI组件
	void setupCardLabels(); // 设置卡牌标签
    QString getCardPointName(Card::CardPoint point) const;

    QGridLayout* m_layout;
    QMap<Card::CardPoint, QLabel*> m_countLabels; // 存储点数与数量标签的映射
};

#endif // CARDCOUNTERWIDGET_H 