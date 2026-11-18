QT += core gui qml quick quickcontrols2 dbus multimedia

CONFIG += c++17 release
TARGET = sift
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/filepicker.h \
    src/portalfilepicker.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/portalfilepicker.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
