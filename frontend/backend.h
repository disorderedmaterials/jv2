// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
#include <QNetworkAccessManager>
#include <QProcess>
#include <QString>

// Forward-declarations
class DataSource;
class Locator;
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
    // Network manager
    QNetworkAccessManager manager_;

    private:
    // Return the backend bind address
    QString bindAddress() const;
    // Return a complete route, combining '/'-separated arguments to form the URL
    template <typename... Args> QString createRoute(Args... routeParts)
    {
        QString result = "http://" + bindAddress();
        ([&] { result += "/" + QString("%1").arg(routeParts); }(), ...);
        return result;
    }
    // Create a POST request
    HttpRequestWorker *postRequest(const QString &url, const QJsonObject &data, HttpRequestWorker::HttpRequestHandler handler);
    // Create a request
    HttpRequestWorker *createRequest(const QString &url, HttpRequestWorker::HttpRequestHandler handler = {});
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

    /*
     * Server Endpoints
     */
    public:
    // Ping backend to see if it's alive
    void ping(HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * Journal Endpoints
     */
    public:
    // List available journals in the specified source and directory
    void listJournals(const DataSource &source, const QString &journalDirectory,
                      HttpRequestWorker::HttpRequestHandler handler = {});
    // Get journal file at the specified locations
    void getJournal(const Locator &location, HttpRequestWorker::HttpRequestHandler handler = {});
    // Search all journals for matching runs
    void findRuns(const QString &journalDirectory, const QString &value, const QString &textInput, const QString options,
                  HttpRequestWorker::HttpRequestHandler handler = {});
    // Get updated journal data
    void updateJournal(const QString &journalDirectory, const QString &cycleString, const QString &lastKnownRunNo,
                       HttpRequestWorker::HttpRequestHandler handler = {});
    // Ping for any updates in the specified journal directory
    void pingJournals(const QString &journalDirectory, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get total uAmps for run numbers in the given cycle
    void getRunTotalMuAmps(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                           HttpRequestWorker::HttpRequestHandler handler = {});
    // Go to cycle containing specified run number
    void goToCycle(const QString &journalDirectory, const QString &runNo, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * NeXuS Endpoints
     */
    public:
    // Set data mountpoint
    void setRunDataRoot(const QString &directory, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS log values present in specified run files
    void getNexusFields(const QString &dataDirectory, const QString &cycles, const QString &runNos,
                        HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS log value data for specified run files
    void getNexusLogValueData(const QString &dataDirectory, const QString &runNos, const QString &cycles,
                              const QString &logValue, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor range for specified run numbers in the given cycle
    void getNexusMonitorRange(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                              HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor spectrum for specified run numbers in the given cycle
    void getNexusMonitor(const QString &dataDirectory, const QString &runNos, const QString &cycle, const QString &spectrumID,
                         HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS spectrum range for specified run numbers in the given cycle
    void getNexusSpectrumRange(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                               HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra for specified run numbers in the given cycle
    void getNexusDetector(const QString &dataDirectory, const QString &runNos, const QString &cycle, const QString &spectrumID,
                          HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra analysis for specified run numbers in the given cycle [FIXME - bad name]
    // [FIXME - Different argument order (cycle/runs) to others]
    void getNexusDetectorAnalysis(const QString &dataDirectory, const QString &cycle, const QString &runNos,
                                  HttpRequestWorker::HttpRequestHandler handler = {});
};
