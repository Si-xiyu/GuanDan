#pragma once

#include <QDialog>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onVolumeChanged(int value);
    void onConfirmClicked();

private:
    QSlider* m_volumeSlider;
    QLabel* m_volumeLabel;
    QPushButton* m_confirmButton;
    int m_currentVolume;
};

