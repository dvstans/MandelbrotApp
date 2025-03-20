#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "starting app" << std::endl;

    QApplication a(argc, argv);

    // Set application style
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();

    return a.exec();
}
