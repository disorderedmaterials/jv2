// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QProcess>
#include <QString>

// Forward-declarations
class QCommandLineParser;

// Backend Process
class Backend : public QObject
{
    Q_OBJECT

    public:
    Backend(const QCommandLineParser &args);

    private:
    // Main backend process
    QProcess process_;

    private:
    // Return the backend bind address
    QString bindAddress() const;
    // Configure backend process arguments
    void configureProcessArgs(const QCommandLineParser &args);
    // Configure backend process environment
    void configureEnvironment(const QCommandLineParser &args);

    public slots:
    // Start the backend process
    void start();
    // Stop the backend processs
    void stop();

    signals:
    void started(const QString &);
};
