#include "GuanDan.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GuanDan window;
    window.show();
    return app.exec();
}
