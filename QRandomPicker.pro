QT += core gui widgets

CONFIG += c++20
CONFIG -= app_bundle
win32 {
    QMAKE_CXXFLAGS += -Wall -Wextra
    LIBS += -luser32 -lshell32
}
TARGET = QRandomPicker

SOURCES += main.cpp \
           mainwindow.cpp \
           tabpage.cpp \
           namepicker.cpp \
           newcopydialog.cpp
HEADERS += mainwindow.h \
           tabpage.h \
           namepicker.h \
           newcopydialog.h

