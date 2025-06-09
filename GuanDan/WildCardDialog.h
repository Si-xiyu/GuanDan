#ifndef WILDCARDDIALOG_H
#define WILDCARDDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVector>
#include <QPropertyAnimation>

#include "Card.h"
#include "Cardcombo.h"
#include "CardWidget.h"

class WildCardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WildCardDialog(const QVector<CardCombo::ComboInfo>& validCombos,
        QWidget* parent = nullptr);
    ~WildCardDialog();

    // 获取用户选择的牌型组合
    CardCombo::ComboInfo getSelectedCombo() const;

    // 检查是否有有效选择
    bool hasValidSelection() const;

private slots:
    void onComboSelectionChanged();
    void onConfirmClicked();
    void onCancelClicked();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void createComboOption(const CardCombo::ComboInfo& combo, int index);
    void updateConfirmButtonState();
    void highlightSelection(int index);
    void playSelectionAnimation(QWidget* widget);

    // UI组件
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollWidget;
    QVBoxLayout* m_scrollLayout;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_confirmButton;
    QPushButton* m_cancelButton;

    // 选择相关
    QButtonGroup* m_radioGroup;
    QVector<CardCombo::ComboInfo> m_validCombos;
    QVector<QWidget*> m_comboWidgets;
    int m_selectedIndex;
    QVector<QString> m_selectedStyles;

    // 动画相关
    QPropertyAnimation* m_selectionAnimation;

    // 常量
    static const int DIALOG_MIN_WIDTH = 400;
    static const int DIALOG_MIN_HEIGHT = 300;
    static const int COMBO_WIDGET_HEIGHT = 100;
    static const int CARD_SPACING = 5;
    static const int MARGIN = 10;
    static const int ANIMATION_DURATION = 200;
    static const QString SELECTED_STYLE;
    static const QString NORMAL_STYLE;
    static const QString HOVER_STYLE;
    static const QString SELECTED_HOVER_STYLE;
};

#endif // WILDCARDDIALOG_H