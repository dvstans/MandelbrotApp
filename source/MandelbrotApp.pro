QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 0.2.2
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QMAKE_CXXFLAGS += -O2

SOURCES += \
    calcstatusdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    mandelbrotcalc.cpp \
    mandelbrotviewer.cpp \
    paletteeditdialog.cpp \
    palettegenerator.cpp

HEADERS += \
    calcstatusdialog.h \
    mainwindow.h \
    mandelbrotcalc.h \
    mandelbrotviewer.h \
    paletteeditdialog.h \
    palettegenerator.h \
    paletteinfo.h

FORMS += \
    calcstatusdialog.ui \
    mainwindow.ui \
    paletteeditdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    resources.qrc
