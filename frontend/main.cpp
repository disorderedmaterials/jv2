// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "args.h"
#include "mainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Configure the main application early to create an event loop
    QApplication application(argc, argv);
    QApplication::setWindowIcon(QIcon(":/icon"));

    // Set up and parse command-line arguments
    CLIArgs parser;
    if (!parser.parseArguments(QApplication::arguments()))
        return 1;

    MainWindow window(parser);
    window.show();
    return QApplication::exec();
}
