#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "starting app" << std::endl;

    QApplication a(argc, argv);

    // Load an application style
    QFile styleFile( ":/stylesheet.qss");
    styleFile.open( QFile::ReadOnly );
    QString style( styleFile.readAll() );
    a.setStyleSheet( style );

    MainWindow w;
    w.show();

    auto ret = a.exec();

    std::cout << "exec ret: " << ret << std::endl;

    return ret;
    //return a.exec();
}
