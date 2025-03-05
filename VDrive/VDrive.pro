QT       += core gui widgets
CONFIG   += c++17


LIBS += -F/opt/homebrew/Cellar/qca/2.3.9_3/lib -framework qca-qt6


INCLUDEPATH += /opt/homebrew/Cellar/qca/2.3.9_3/lib/qca-qt6.framework/Headers

SOURCES += \
    drive.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    VirtualDrive.h \
    mainwindow.h

FORMS += \
    mainwindow.ui
