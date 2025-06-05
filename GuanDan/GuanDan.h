#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_GuanDan.h"

class GuanDan : public QMainWindow
{
    Q_OBJECT

public:
    GuanDan(QWidget *parent = nullptr);
    ~GuanDan();

private:
    Ui::GuanDanClass ui;
};

