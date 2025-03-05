QT       += core gui widgets
CONFIG   += c++17

# 1) Explicitly add the framework search path with -F
LIBS += -F/opt/homebrew/Cellar/qca/2.3.9_3/lib -framework qca-qt6

# 2) (Optional) If you still get "QtCrypto file not found," add the Headers path:
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
