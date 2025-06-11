#pragma once

#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include <QFrame>
#include "Card.h"
#include "CardWidget.h"
#include "CardCombo.h"

// 常量定义
const int SHOW_CARD_OVERLAP_HORIZONTAL = 40;    // 卡片水平重叠的像素数
const int SHOW_CARD_OVERLAP_VERTICAL = 25;      // 相同牌垂直重叠的像素数（用于癞子堆叠）
const int SHOW_CARD_MIN_WIDTH = 150;
const int SHOW_CARD_MIN_HEIGHT = 150;

class ShowCardWidget : public QWidget
{
    Q_OBJECT

public:
    static const int DefaultCardWidth = 71;  // 默认卡片宽度
    static const int DefaultCardHeight = 96; // 默认卡片高度
    explicit ShowCardWidget(QWidget* parent = nullptr);
    ~ShowCardWidget();

public slots:
    // 更新显示的牌型
    void updateDisplay(const CardCombo::ComboInfo& combo, const QVector<Card>& originalCards);
    // 清空显示
    void clearDisplay();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

private:
    // 创建新的卡片视图
    CardWidget* createCardWidget(const Card& card);
    // 重新布局所有卡片
    void relayoutCards();
    // 计算卡片位置
    QPoint calculateCardPosition(int index, int stackIndex) const;
    // 更新卡片的Z顺序（堆叠效果）
    void updateCardZOrder();

    QVBoxLayout* m_mainLayout;          // 主布局
    QLabel* m_comboTypeLabel;           // 显示牌型名称的标签
    QFrame* m_cardFrame;                // 卡牌容器框架
    QVector<CardWidget*> m_cardWidgets; // 所有卡片视图
    
    // 映射普通牌和癞子牌的关系（癞子牌->变成的牌）
    QMap<CardWidget*, CardWidget*> m_wildToRepresentMap;
    
    // 当前显示的牌型信息
    CardCombo::ComboInfo m_currentCombo;
    QVector<Card> m_originalCards;
};

