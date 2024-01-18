// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "args.h"
#include "mainWindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    // Configure the main application early to create an event loop
    QApplication application(argc, argv);
    QApplication::setWindowIcon(QIcon(":/icon"));

    // Set up and parse command-line arguments
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription("Journal Viewer 2");
    auto helpOption = parser.addHelpOption();
    parser.addOptions(
        {{CLIArgs::ISISArchiveDirectory, "Path to directory / mountpoint containing main ISIS Archive run data", "directory"},
         {CLIArgs::LogLevel, "Log level for the backend. Matches gunicorn log levels: info, debug", "log level"},
         {CLIArgs::NoIDAaaS, "Don't automatically define the IDAaaS source"},
         {CLIArgs::NoISISArchive, "Don't automatically define the ISIS Archive source"},
         {CLIArgs::UseWaitress, "Use waitress instead of gunicorn (Windows only)"},
         {CLIArgs::DebugBackend, "Enable debug logging in backend"}});

    if (!parser.parse(QApplication::arguments()))
    {
        qInfo() << parser.errorText();
        return 1;
    }
    if (parser.isSet(helpOption))
        parser.showHelp();

    MainWindow window(parser);
    window.show();
    return QApplication::exec();
}
