QT += core gui widgets

CONFIG += c++20
CONFIG -= app_bundle
win32 {
    QMAKE_CXXFLAGS += -Wall -Wextra
    LIBS += -luser32 -lshell32
}
TARGET = QRandomPicker

SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/tabpage.cpp \
           src/session.cpp \
           src/sessionmanager.cpp \
           src/newcopydialog.cpp \
           src/nameeditor.cpp \
           src/historydialog.cpp \
           src/sessionpickerdialog.cpp \
           src/miniwindow.cpp \
           src/pickpopup.cpp \
           src/floatingball.cpp
HEADERS += src/mainwindow.h \
           src/tabpage.h \
           src/session.h \
           src/sessionmanager.h \
           src/newcopydialog.h \
           src/nameeditor.h \
           src/historydialog.h \
           src/sessionpickerdialog.h \
           src/miniwindow.h \
           src/pickpopup.h \
           src/floatingball.h
FORMS += src/mainwindow.ui \
         src/newcopydialog.ui

RESOURCES += \
    resource/resource.qrc

RC_ICONS = "resource/QRandomPicker.ico"