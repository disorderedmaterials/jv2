// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include <QCommandLineParser>
#include <QList>
#include <QString>

class CLIArgs : public QCommandLineParser
{
    public:
    CLIArgs();

    private:
    // Standard help option
    QCommandLineOption helpOption_;

    public:
    // Parse arguments, returning if all is OK
    bool parseArguments(const QList<QString> &arguments);

    /*
     * Argument Name Strings
     */
    public:
    const inline static QString LogLevel = QStringLiteral("log-level");
    const inline static QString HideIDAaaS = QStringLiteral("hide-idaaas");
    const inline static QString HideISISArchive = QStringLiteral("hide-isis-archive");
    const inline static QString ISISArchiveDirectory = QStringLiteral("isis-archive-dir");
    const inline static QString UseWaitress = QStringLiteral("use-waitress");
    const inline static QString DebugBackend = QStringLiteral("debug-backend");
};
