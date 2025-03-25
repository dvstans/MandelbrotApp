#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "starting app" << std::endl;

    QCoreApplication::setOrganizationName("DVStansberry");
    QCoreApplication::setApplicationName("MandelbrotApp");

    QApplication a(argc, argv);

    // Set application style
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();

    return a.exec();
}
