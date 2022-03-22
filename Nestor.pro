include(qtsingleapplication/qtsingleapplication.pri)

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

TARGET = Nestor

RC_ICONS = nestor.ico

INCLUDEPATH += \
    $$PWD/3rd

LIBS += \
    -L$$PWD/3rd/WinDivert -lWinDivert

SOURCES += \
    main.cpp \
    main_window.cpp \
    divert_worker.cpp \
    widget/check_box_plus.cpp \
    widget/line_edit_plus.cpp

HEADERS += \
    main_window.h \
    config.h \
    divert_worker.h \
    variable.h \
    widget/check_box_plus.h \
    widget/line_edit_plus.h

RESOURCES += \
    resource.qrc
