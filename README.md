# IPO Stalker

Reminder system for upcoming IPOs.  Features automatic built-in scraping mechanism that aggregates data from multiple data sources.

[![screenshot](assets/screenshot.png)](assets/screenshot.png)

## Build

    qmake
    make -j


## Install

    sudo make install


## Uninstall

    sudo make uninstall


## Customize

Placing a file named `ipo-stalker.qss` into `~/.config/ipo-stalker/` will serve as custom stylesheet for the program.
You can use [ipo-stalker.qss](res/stylesheets/ipo-stalker.qss) for reference.
