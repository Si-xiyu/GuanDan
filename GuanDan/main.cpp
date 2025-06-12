#include "GuanDan.h"
#include "SettingsManager.h"
#include "SoundManager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 加载音量设置
    int volume = SettingsManager::loadVolume();
    SoundManager::instance().setVolume(volume);
    
    GuanDan w;
    w.show();
    return a.exec();
}
