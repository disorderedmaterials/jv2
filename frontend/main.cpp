// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "args.h"
#include "backend.h"
#include "mainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QThread>

namespace
{

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineHelpRequested
};
} // namespace

// Forward declaration of parsing
CommandLineParseResult parseCommandLine(const QApplication &, QCommandLineParser *, QString *);

int main(int argc, char *argv[])
{
    // Configure the main application early to create an event loop
    QApplication application(argc, argv);
    application.setWindowIcon(QIcon(":/icon"));

    // Command-line arguments
    QCommandLineParser parser;
    QString errorMessage;
    switch (parseCommandLine(application, &parser, &errorMessage))
    {
        case CommandLineParseResult::CommandLineHelpRequested:
            return 0;
        case CommandLineParseResult::CommandLineError:
        {
            qInfo() << errorMessage;
            return 1;
        }
        case CommandLineParseResult::CommandLineOk:
            break;
    }

    MainWindow window(parser);
    window.show();
    return application.exec();
}

CommandLineParseResult parseCommandLine(const QApplication &app, QCommandLineParser *parser, QString *errorMessage)
{
    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->setApplicationDescription("Journal Viewer 2");
    auto helpOption = parser->addHelpOption();
    parser->addOptions({{Args::LogLevel, "Log level for the backend. Matches gunicorn log levels: info, debug", "loglevel"}});

    if (!parser->parse(app.arguments()))
    {
        *errorMessage = parser->errorText();
        return CommandLineError;
    }
    if (parser->isSet(helpOption))
        return CommandLineHelpRequested;

    return CommandLineOk;
}
