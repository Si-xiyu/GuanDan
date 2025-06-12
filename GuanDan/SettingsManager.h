#pragma once

#include <QString>
#include <QSettings>

class SettingsManager
{
public:
    static void saveVolume(int volume);
    static int loadVolume();
    static QString getConfigFilePath();

private:
    SettingsManager() = delete; // 禁止实例化
    static QSettings* createSettings();
};

