VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 5

DEFINES     += "VERSION_MAJOR=$$VERSION_MAJOR" \
               "VERSION_MINOR=$$VERSION_MINOR" \
               "VERSION_PATCH=$$VERSION_PATCH" \

VERSION      = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}

QT          += core gui network sql webenginewidgets widgets
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
PROG_NAME    = ipo-stalker

INCLUDEPATH += $${INC_DIR}

HEADERS     += inc/common.hpp \
               inc/mainwindow.hpp \
               inc/traymenu.hpp \
               inc/runguard.hpp \
               inc/ipo.hpp \
               inc/data-source.hpp \
               inc/data-sources/edelweiss.hpp \
               inc/data-sources/euronext.hpp \
               inc/data-sources/finnhub.hpp \
               inc/data-sources/ipo-cal-appspot.hpp \
               inc/data-sources/nasdaq.hpp \
               inc/data-sources/otcbb-swingtradebot.hpp \
               inc/db.hpp \
               inc/scraper.hpp \
               inc/settings.hpp \

SOURCES     += src/main.cpp \
               src/mainwindow.cpp \
               src/traymenu.cpp \
               src/runguard.cpp \
               src/data-source.cpp \
               src/data-sources/edelweiss.cpp \
               src/data-sources/euronext.cpp \
               src/data-sources/finnhub.cpp \
               src/data-sources/ipo-cal-appspot.cpp \
               src/data-sources/nasdaq.cpp \
               src/data-sources/otcbb-swingtradebot.cpp \
               src/db.cpp \
               src/scraper.cpp \
               src/settings.cpp \

FORMS       += ui/mainwindow.ui \

RESOURCES    = res/resources.qrc \

OTHER_FILES += res/images/$${PROG_NAME}.svg \
               res/stylesheets/$${PROG_NAME}.qss \

QMAKE_CLEAN += -r $${DESTDIR}/$${PROG_NAME}

DEFINES     += PROG_NAME=\\\"$${PROG_NAME}\\\"

# CONFIG      += debug

CONFIG(debug, release|debug):DEFINES += DEBUG

# GNU/Linux, FreeBSD, etc
unix:!mac {
    isEmpty(PREFIX) {
        PREFIX   = /usr
    }
    BINDIR       = $${PREFIX}/bin
    DATADIR      = $${PREFIX}/share

    OTHER_FILES += dist/$${PROG_NAME}.desktop

    target.path  = $${BINDIR}

    INSTALLS    += target

    desktop.path = $${DATADIR}/applications
    eval(desktop.files += dist/$${PROG_NAME}.desktop)

    INSTALLS    += desktop

    icon.path    = $${DATADIR}/icons/hicolor/scalable/apps
    eval(icon.files += res/images/$${PROG_NAME}.svg)

    INSTALLS    += icon
}
