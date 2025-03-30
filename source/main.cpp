#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>

/**
 * @brief Program entry point
 * @param argc - Argument count
 * @param argv - Arguments
 * @return Exit code from QApplication::exec
 */
int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("DVStansberry");
    QCoreApplication::setApplicationName("MandelbrotApp");

    QApplication a(argc, argv);

    // Set application style
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();

    return a.exec();
}
