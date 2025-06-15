#include "GuanDan.h"
#include "SettingsManager.h"
#include "SoundManager.h"
#include <QApplication>
#include <QtCore>
#include <QIcon>

int main(int argc, char *argv[])
{
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

    QApplication a(argc, argv);

    // 设置图标
    a.setWindowIcon(QIcon(":/icon/res/App_icon.png"));
    
    // 加载音量设置
    int volume = SettingsManager::loadVolume();
    SoundManager::instance().setVolume(volume);
    
    GuanDan w;
    w.show();
    return a.exec();
}
