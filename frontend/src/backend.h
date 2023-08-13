// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QProcess>
#include <QString>

// Forward-declarations
class QCommandLineParser;

/**
 * Control the backend process that communicates with the Journal server
 */
class Backend : public QObject
{
    Q_OBJECT

    public:
    static QString bind_address() { return "127.0.0.1:5000"; };

    Backend(const QCommandLineParser &args);

    public slots:

    void start();
    void stop();

    private:
    void configureProcessArgs(const QCommandLineParser &args);
    void configureEnvironment(const QCommandLineParser &args);

    QProcess process_;
};
