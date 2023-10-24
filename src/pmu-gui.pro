
QT += \
    core \
    gui \
    widgets \
    charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += #QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QT_DISABLE_DEPRECATED_BEFORE=0x051500

SOURCES += \
    abstractphasorview.cpp \
    appcentralwidget.cpp \
    appdockwidget.cpp \
    appmainmenu.cpp \
    appstatusbar.cpp \
    apptoolbar.cpp \
    main.cpp \
    appmainwindow.cpp \
    phasorpolarview.cpp \
    phasorwaveview.cpp \
    pmu.cpp \
    rawdatareader.cpp

HEADERS += \
    abstractphasorview.h \
    appcentralwidget.h \
    appdockwidget.h \
    appmainmenu.h \
    appmainwindow.h \
    appstatusbar.h \
    apptoolbar.h \
    phasorpolarview.h \
    phasorwaveview.h \
    pmu.h \
    rawdatareader.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
