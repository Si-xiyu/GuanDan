#include "SettingsManager.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

QString SettingsManager::getConfigFilePath()
{
    return QDir::toNativeSeparators(QApplication::applicationDirPath() + "/GuanDan.ini");
}

QSettings* SettingsManager::createSettings()
{
    return new QSettings(getConfigFilePath(), QSettings::IniFormat);
}

void SettingsManager::saveVolume(int volume)
{
    QSettings* settings = createSettings();
    settings->setValue("Audio/Volume", volume);
    delete settings;
}

int SettingsManager::loadVolume()
{
    QSettings* settings = createSettings();
    int volume = settings->value("Audio/Volume", 50).toInt(); // 默认音量50%
    delete settings;
    return volume;
}
