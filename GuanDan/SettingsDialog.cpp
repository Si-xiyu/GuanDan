#include "SettingsDialog.h"
#include "SoundManager.h"
#include "SettingsManager.h"
#include "RulesDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentVolume(SoundManager::instance().getVolume())
{
    setWindowTitle(tr("游戏设置"));
    setFixedSize(300, 250);  // 增加高度以容纳新设置

    // 移除窗口标题栏的问号（帮助）按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 创建音量滑块
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(m_currentVolume);

    // 创建音量标签
    m_volumeLabel = new QLabel(QString::number(m_currentVolume) + "%", this);
    m_volumeLabel->setAlignment(Qt::AlignCenter);

    // 创建出牌时间设置
    m_durationSpinBox = new QSpinBox(this);
    m_durationSpinBox->setRange(0, 60);
    m_durationSpinBox->setSuffix(" 秒");
    m_durationSpinBox->setSpecialValueText(tr("不限时")); // 0值显示为"不限时"
    m_durationSpinBox->setSingleStep(5);
    m_durationSpinBox->setValue(SettingsManager::loadTurnDuration());

    // 创建按钮
    m_confirmButton = new QPushButton(tr("确认"), this);
    m_confirmButton->setFixedSize(80, 30);
    
    // 创建规则按钮
    m_rulesButton = new QPushButton(tr("规则介绍"), this);
    m_rulesButton->setFixedSize(80, 30);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    QHBoxLayout* durationLayout = new QHBoxLayout();
    QHBoxLayout* buttonLayout = new QHBoxLayout(); // 新增按钮布局
    
    volumeLayout->addWidget(new QLabel(tr("音量："), this));
    volumeLayout->addWidget(m_volumeSlider);
    volumeLayout->addWidget(m_volumeLabel);

    durationLayout->addWidget(new QLabel(tr("出牌时间："), this));
    durationLayout->addWidget(m_durationSpinBox);
    
    // 将按钮添加到按钮布局
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_rulesButton);
    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(volumeLayout);
    mainLayout->addLayout(durationLayout);
    mainLayout->addStretch(); // 添加弹性空间
    mainLayout->addLayout(buttonLayout); // 添加按钮布局

    // 连接信号
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsDialog::onVolumeChanged);
    connect(m_confirmButton, &QPushButton::clicked, this, &SettingsDialog::onConfirmClicked);
    connect(m_rulesButton, &QPushButton::clicked, this, &SettingsDialog::onShowRulesClicked); // 连接规则按钮信号
}

void SettingsDialog::onVolumeChanged(int value)
{
    m_currentVolume = value;
    m_volumeLabel->setText(QString::number(value) + "%");
    SoundManager::instance().setVolume(value);
}

void SettingsDialog::onConfirmClicked()
{
    SettingsManager::saveVolume(m_currentVolume);
    SettingsManager::saveTurnDuration(m_durationSpinBox->value());
    accept();
}

// 实现显示规则对话框的槽函数
void SettingsDialog::onShowRulesClicked()
{
    RulesDialog* dialog = new RulesDialog(this);
    dialog->exec(); // 以模态方式显示对话框，阻塞与父窗口的交互，直到对话框关闭
    delete dialog; // 对话框关闭后清理实例
}
