VERSION_MAJOR = 0
VERSION_MINOR = 0
VERSION_PATCH = 1

DEFINES     += "VERSION_MAJOR=$$VERSION_MAJOR"\
               "VERSION_MINOR=$$VERSION_MINOR"\
               "VERSION_PATCH=$$VERSION_PATCH" \

VERSION      = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}

QT          += core gui widgets network
CONFIG      += c++11
TEMPLATE     = app

SRC_DIR      = src
INC_DIR      = inc
FORMS_DIR    = ui

OBJECTS_DIR  = .objs
MOC_DIR      = .mocs
UI_DIR       = .uis
RCC_DIR      = .qrcs

DESTDIR      = bin
TARGET       = ipo-calendar

INCLUDEPATH += $${INC_DIR}

SOURCES     += src/main.cpp \
               src/mainwindow.cpp \
               src/traymenu.cpp \
               src/runguard.cpp \
               src/data-sources/ipo-cal-appspot.cpp \

HEADERS     += inc/mainwindow.hpp \
               inc/traymenu.hpp \
               inc/runguard.hpp \
               inc/data-sources/ipo-cal-appspot.hpp \

FORMS       += ui/mainwindow.ui \

RESOURCES    = res/resources.qrc \

OTHER_FILES += dist/$${TARGET}.desktop \
               res/images/$${TARGET}.svg \
               res/images/tray.png \
               res/styles/$${TARGET}.qss \

QMAKE_CLEAN += -r $${DESTDIR}/$${TARGET}

DEFINES     += TARGET=\\\"$${TARGET}\\\"

unix:!mac {
    isEmpty(PREFIX) {
        PREFIX   = /usr
    }
    BINDIR       = $${PREFIX}/bin
    DATADIR      = $${PREFIX}/share

    target.path  = $${BINDIR}

    INSTALLS    += target

    desktop.path = $${DATADIR}/applications
    eval(desktop.files += dist/$${TARGET}.desktop)

    INSTALLS    += desktop

    icon.path    = $${DATADIR}/icons/hicolor/scalable/apps
    eval(icon.files += res/images/$${TARGET}.svg)

    INSTALLS    += icon
}
