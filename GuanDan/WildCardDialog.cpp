#include "WildCardDialog.h"
#include <QApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

// 定义静态常量样式
const QString WildCardDialog::SELECTED_STYLE = 
    "QWidget { "
    "   border: 2px solid #2E7D32; "  // 更深的绿色边框
    "   border-radius: 5px; "
    "   margin: 2px; "
    "   background-color: #E8F5E9; "  // 更深的背景色
    "   transition: all 0.2s ease-in-out; "
    "}";

const QString WildCardDialog::NORMAL_STYLE = 
    "QWidget { "
    "   border: 1px solid #ddd; "
    "   border-radius: 5px; "
    "   margin: 2px; "
    "   background-color: #ffffff; "
    "   transition: all 0.2s ease-in-out; "
    "}";

const QString WildCardDialog::HOVER_STYLE = 
    "QWidget { "
    "   border: 1px solid #4CAF50; "
    "   border-radius: 5px; "
    "   margin: 2px; "
    "   background-color: #f5f5f5; "
    "   transition: all 0.2s ease-in-out; "
    "}";

// 添加新的悬停样式常量
const QString WildCardDialog::SELECTED_HOVER_STYLE = 
    "QWidget { "
    "   border: 2px solid #1B5E20; "  // 更深的绿色边框
    "   border-radius: 5px; "
    "   margin: 2px; "
    "   background-color: #C8E6C9; "  // 更深的背景色
    "   transition: all 0.2s ease-in-out; "
    "}";

WildCardDialog::WildCardDialog(const QVector<CardCombo::ComboInfo>& validCombos,
    QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollWidget(nullptr)
    , m_scrollLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_confirmButton(nullptr)
    , m_cancelButton(nullptr)
    , m_radioGroup(nullptr)
    , m_validCombos(validCombos)
    , m_selectedIndex(-1)
    , m_selectedStyles(validCombos.size(), QString())  // 初始化选中样式数组
    , m_selectionAnimation(nullptr)
{
    setupUI();

    // 设置对话框属性
    setWindowTitle(tr("选择出牌组合"));
    setModal(true);
    setMinimumSize(DIALOG_MIN_WIDTH, DIALOG_MIN_HEIGHT);

    // 设置窗口样式
    setStyleSheet(
        "QDialog {"
        "   background-color: #ffffff;"
        "   border: 1px solid #cccccc;"
        "   border-radius: 8px;"
        "}"
        "QPushButton {"
        "   padding: 5px 15px;"
        "   border-radius: 4px;"
        "   font-size: 12px;"
        "}"
        "QPushButton:enabled {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: #45a049;"
        "}"
        "QScrollArea {"
        "   border: none;"
        "}"
    );

    // 居中显示
    if (parent) {
        move(parent->geometry().center() - rect().center());
    }
    else {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->geometry();
            move(screenGeometry.center() - rect().center());
        }
    }

    // 初始化动画
    m_selectionAnimation = new QPropertyAnimation(this);
    m_selectionAnimation->setDuration(ANIMATION_DURATION);
}

WildCardDialog::~WildCardDialog()
{
    // Qt会自动清理子组件，但我们需要清理手动创建的组件
    for (QWidget* widget : m_comboWidgets) {
        if (widget) {
            widget->deleteLater();
        }
    }
}

void WildCardDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setMargin(MARGIN);
    m_mainLayout->setSpacing(MARGIN);

    // 标题标签
    m_titleLabel = new QLabel(tr("检测到多种可能的牌型组合，请选择要出的牌型："));
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    m_mainLayout->addWidget(m_titleLabel);

    // 创建滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameStyle(QFrame::StyledPanel);

    m_scrollWidget = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollWidget);
    m_scrollLayout->setMargin(MARGIN);
    m_scrollLayout->setSpacing(MARGIN);

    m_scrollArea->setWidget(m_scrollWidget);
    m_mainLayout->addWidget(m_scrollArea);

    // 创建单选按钮组
    m_radioGroup = new QButtonGroup(this);
    connect(m_radioGroup, &QButtonGroup::idClicked,
        this, &WildCardDialog::onComboSelectionChanged);

    // 为每个有效组合创建选项
    for (int i = 0; i < m_validCombos.size(); ++i) {
        createComboOption(m_validCombos[i], i);
    }

    // 添加弹性空间
    m_scrollLayout->addStretch();

    // 创建按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();

    // 创建跳过按钮
    QPushButton* skipButton = new QPushButton(tr("跳过"));
    skipButton->setMinimumSize(80, 30);
    skipButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FFA500;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #FF8C00;"
        "}"
    );
    connect(skipButton, &QPushButton::clicked, this, &QDialog::reject);
    m_buttonLayout->addWidget(skipButton);

    // 创建出牌按钮
    m_confirmButton = new QPushButton(tr("出牌"));
    m_confirmButton->setMinimumSize(80, 30);
    m_confirmButton->setEnabled(false);
    m_confirmButton->setStyleSheet(
        "QPushButton:enabled {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: #45a049;"
        "}"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &WildCardDialog::onConfirmClicked);
    m_buttonLayout->addWidget(m_confirmButton);

    m_mainLayout->addLayout(m_buttonLayout);
}

void WildCardDialog::createComboOption(const CardCombo::ComboInfo& combo, int index)
{
    // 创建包含单选按钮和牌型显示的容器
    QWidget* comboWidget = new QWidget();
    comboWidget->setMinimumHeight(COMBO_WIDGET_HEIGHT);
    comboWidget->setStyleSheet(NORMAL_STYLE);

    QHBoxLayout* comboLayout = new QHBoxLayout(comboWidget);
    comboLayout->setMargin(MARGIN);
    comboLayout->setSpacing(MARGIN);

    // 创建单选按钮
    QRadioButton* radioButton = new QRadioButton();
    radioButton->setStyleSheet("QRadioButton { font-size: 12px; }");
    m_radioGroup->addButton(radioButton, index);
    comboLayout->addWidget(radioButton);

    // 创建牌型描述标签
    QLabel* descLabel = new QLabel(combo.getDescription());
    descLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #555;");
    descLabel->setMinimumWidth(120);
    comboLayout->addWidget(descLabel);

    // 创建牌的显示区域
    QHBoxLayout* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(CARD_SPACING);

    // 显示牌型中的每张牌
    for (const Card& card : combo.cards_in_combo) {
        CardWidget* cardWidget = new CardWidget(card);
        cardWidget->setFrontSide(true);
        cardWidget->setFixedSize(CARD_WIDGET_WIDTH * 0.8, CARD_WIDGET_HEIGHT * 0.8); // 稍微缩小显示
        cardWidget->setEnabled(false); // 禁用交互
        cardsLayout->addWidget(cardWidget);
    }

    cardsLayout->addStretch();
    comboLayout->addLayout(cardsLayout);

    // 如果有癞子，显示癞子信息
    if (combo.wild_cards_used > 0) {
        QLabel* wildLabel = new QLabel(tr("(使用%1张癞子)").arg(combo.wild_cards_used));
        wildLabel->setStyleSheet("font-size: 10px; color: #888; font-style: italic;");
        comboLayout->addWidget(wildLabel);
    }

    // 特殊牌型标记
    if (combo.is_flush_straight_bomb) {
        QLabel* specialLabel = new QLabel(tr("同花顺"));
        specialLabel->setStyleSheet("font-size: 10px; color: #d32f2f; font-weight: bold;");
        comboLayout->addWidget(specialLabel);
    }

    comboLayout->addStretch();

    m_scrollLayout->addWidget(comboWidget);
    m_comboWidgets.append(comboWidget);

    // 添加事件过滤器
    comboWidget->installEventFilter(this);
}

bool WildCardDialog::eventFilter(QObject* obj, QEvent* event)
{
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (widget && m_comboWidgets.contains(widget)) {
        int index = m_comboWidgets.indexOf(widget);
        if (event->type() == QEvent::MouseButtonPress) {
            // 当点击组件时，切换选中状态
            if (index == m_selectedIndex) {
                // 如果已经选中，则取消选中
                m_radioGroup->button(index)->setChecked(false);
                m_selectedIndex = -1;
                // 恢复普通样式，允许悬停效果
                widget->setStyleSheet(NORMAL_STYLE);
                m_selectedStyles[index].clear();
            } else {
                // 如果未选中，则选中
                if (QRadioButton* radio = qobject_cast<QRadioButton*>(m_radioGroup->button(index))) {
                    radio->setChecked(true);
                    // 保存当前样式（可能是悬停样式）
                    m_selectedStyles[index] = widget->styleSheet();
                    m_selectedIndex = index;
                    updateConfirmButtonState();
                }
            }
            return true;
        }
        else if (event->type() == QEvent::Enter) {
            if (index != m_selectedIndex) {
                // 只有未选中的组件才响应悬停效果
                widget->setStyleSheet(HOVER_STYLE);
            }
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            if (index != m_selectedIndex) {
                // 只有未选中的组件才恢复普通样式
                widget->setStyleSheet(NORMAL_STYLE);
            }
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void WildCardDialog::highlightSelection(int index)
{
    // 更新所有组件的样式
    for (int i = 0; i < m_comboWidgets.size(); ++i) {
        QWidget* widget = m_comboWidgets[i];
        if (i == index) {
            // 如果当前组件处于悬停状态，使用悬停样式
            widget->setStyleSheet(m_selectedStyles[i]);
        } else {
            widget->setStyleSheet(NORMAL_STYLE);
        }
    }
    m_selectedIndex = index;
    updateConfirmButtonState();
}

void WildCardDialog::playSelectionAnimation(QWidget* widget)
{
    if (!widget || !m_selectionAnimation) return;
    
    // 停止当前动画
    m_selectionAnimation->stop();
    
    // 创建不透明度效果
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    
    // 设置动画属性
    m_selectionAnimation->setTargetObject(effect);
    m_selectionAnimation->setPropertyName("opacity");
    m_selectionAnimation->setStartValue(0.5);
    m_selectionAnimation->setEndValue(1.0);
    
    // 启动动画
    m_selectionAnimation->start();
}

void WildCardDialog::onComboSelectionChanged()
{
    // 获取当前选中的按钮索引
    int selectedId = m_radioGroup->checkedId();
    if (selectedId >= 0) {
        // 更新所有组件的样式
        for (int i = 0; i < m_comboWidgets.size(); ++i) {
            QWidget* widget = m_comboWidgets[i];
            if (i == selectedId) {
                // 如果已经有保存的样式，使用保存的样式
                if (!m_selectedStyles[i].isEmpty()) {
                    widget->setStyleSheet(m_selectedStyles[i]);
                } else {
                    // 否则使用当前样式（可能是悬停样式）
                    m_selectedStyles[i] = widget->styleSheet();
                }
            } else {
                widget->setStyleSheet(NORMAL_STYLE);
                m_selectedStyles[i].clear();
            }
        }
        m_selectedIndex = selectedId;
        updateConfirmButtonState();
    }
}

void WildCardDialog::updateConfirmButtonState()
{
    m_confirmButton->setEnabled(m_selectedIndex >= 0 && m_selectedIndex < m_validCombos.size());
}

void WildCardDialog::onConfirmClicked()
{
    if (hasValidSelection()) {
        accept(); // 关闭对话框并返回QDialog::Accepted
    }
}

void WildCardDialog::onCancelClicked()
{
    reject(); // 关闭对话框并返回QDialog::Rejected
}

CardCombo::ComboInfo WildCardDialog::getSelectedCombo() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_validCombos.size()) {
        return m_validCombos[m_selectedIndex];
    }

    // 返回无效组合
    return CardCombo::ComboInfo();
}

bool WildCardDialog::hasValidSelection() const
{
    return m_selectedIndex >= 0 && m_selectedIndex < m_validCombos.size();
}