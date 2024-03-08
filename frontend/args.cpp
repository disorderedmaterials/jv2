// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "args.h"

CLIArgs::CLIArgs() : helpOption_(addHelpOption())
{
    setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    setApplicationDescription("Journal Viewer 2");

    addOptions(
        {{CLIArgs::ISISArchiveDirectory, "Path to directory / mountpoint containing main ISIS Archive run data", "directory"},
         {CLIArgs::LogLevel, "Log level for the backend. Matches WSGI server log levels: (e.g. for gunicorn 'info' or 'debug')",
          "log level"},
         {CLIArgs::NoIDAaaS, "Don't automatically define the IDAaaS source"},
         {CLIArgs::NoISISArchive, "Don't automatically define the ISIS Archive source"},
         {CLIArgs::UseWaitress, "Use waitress instead of gunicorn (Windows only)"},
         {CLIArgs::DebugBackend, "Enable debug logging in backend"}});
}

// Parse arguments, returning if all is OK
bool CLIArgs::parseArguments(const QList<QString> &arguments)
{
    if (!parse(arguments))
    {
        qInfo() << errorText();
        return false;
    }

    // Show help and exit if requested
    if (isSet(helpOption_))
        showHelp();

    return true;
}
