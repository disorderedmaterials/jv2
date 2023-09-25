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

    // Start backend as soon as we can
    Backend backend(parser);
    backend.start();
    // A sleep is not ideal but the mainwindow needs the backend fully started
    // before it can start
    QThread::sleep(1);

    // Start frontend and hook up kill backend on quit
    QObject::connect(&application, &QApplication::aboutToQuit, &backend, &Backend::stop);
    MainWindow window;
    window.show();
    return application.exec();
}

CommandLineParseResult parseCommandLine(const QApplication &app, QCommandLineParser *parser, QString *errorMessage)
{
    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->setApplicationDescription("Journal Viewer 2");
    auto helpOption = parser->addHelpOption();
    // clang-format off
    parser->addOptions({
        {Args::LogLevel, "Log level for the backend. Matches gunicorn log levels: info, debug", "loglevel"},
        {Args::RunLocatorClass, "Name of class used to located run data. Fully-qualified Python module.class name is required.", "class"},
        {Args::RunLocatorPrefix, "A prefix given to the run locator, setting the base path for all run files.", "prefix"}
    });
    // clang-format on

    if (!parser->parse(app.arguments()))
    {
        *errorMessage = parser->errorText();
        return CommandLineError;
    }
    if (parser->isSet(helpOption))
        return CommandLineHelpRequested;

    return CommandLineOk;
}
