#include "SettingsDialog.h"
#include "SoundManager.h"
#include "SettingsManager.h"
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

    // 创建确认按钮
    m_confirmButton = new QPushButton(tr("确认"), this);
    m_confirmButton->setFixedSize(80, 30);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    QHBoxLayout* durationLayout = new QHBoxLayout();
    
    volumeLayout->addWidget(new QLabel(tr("音量："), this));
    volumeLayout->addWidget(m_volumeSlider);
    volumeLayout->addWidget(m_volumeLabel);

    durationLayout->addWidget(new QLabel(tr("出牌时间："), this));
    durationLayout->addWidget(m_durationSpinBox);

    mainLayout->addLayout(volumeLayout);
    mainLayout->addLayout(durationLayout);
    mainLayout->addWidget(m_confirmButton, 0, Qt::AlignCenter);

    // 连接信号
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsDialog::onVolumeChanged);
    connect(m_confirmButton, &QPushButton::clicked, this, &SettingsDialog::onConfirmClicked);
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
