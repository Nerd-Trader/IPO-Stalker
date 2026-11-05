#include <QtGui>

#include "traymenu.hpp"

TrayMenu::TrayMenu(QWidget *parent) :
    QMenu(parent)
{
    toggleAction = new QAction(tr("Toggle"), this);
    aboutAction = new QAction(tr("About IPO Stalker %1").arg(PROG_VERSION), this);
    quitAction = new QAction(tr("Quit"), this);

    connect(toggleAction, SIGNAL(triggered()), parent, SLOT(toggleHidden()));
    connect(aboutAction, SIGNAL(triggered()), parent, SLOT(showAbout()));
    connect(quitAction, SIGNAL(triggered()), parent, SLOT(quitApplication()));

    addAction(toggleAction);
    addSeparator();
    addAction(aboutAction);
    addAction(quitAction);
}

TrayMenu::~TrayMenu()
{
    delete toggleAction;
    delete aboutAction;
    delete quitAction;
}
