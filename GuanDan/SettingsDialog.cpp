#include "SettingsDialog.h"
#include "SoundManager.h"
#include "SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentVolume(SoundManager::instance().getVolume())
{
    setWindowTitle(tr("设置"));
    setFixedSize(300, 150);

    // 创建音量滑块
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(m_currentVolume);

    // 创建音量标签
    m_volumeLabel = new QLabel(QString::number(m_currentVolume) + "%", this);
    m_volumeLabel->setAlignment(Qt::AlignCenter);

    // 创建确认按钮
    m_confirmButton = new QPushButton(tr("确认"), this);
    m_confirmButton->setFixedSize(80, 30);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    
    volumeLayout->addWidget(new QLabel(tr("音量："), this));
    volumeLayout->addWidget(m_volumeSlider);
    volumeLayout->addWidget(m_volumeLabel);

    mainLayout->addLayout(volumeLayout);
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
    accept();
}
