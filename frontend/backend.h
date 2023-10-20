// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
#include <QNetworkAccessManager>
#include <QProcess>
#include <QString>

// Forward-declarations
class JournalSource;
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
    void listJournals(const JournalSource &source, const QString &journalDirectory,
                      HttpRequestWorker::HttpRequestHandler handler = {});
    // Get journal file at the specified location
    void getJournal(const Locator &location, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get any updates to the specified journal
    void getJournalUpdates(const Locator &location, HttpRequestWorker::HttpRequestHandler handler = {});
    // Search all journals for matching runs
    void findRuns(const QString &journalDirectory, const QString &value, const QString &textInput, const QString options,
                  HttpRequestWorker::HttpRequestHandler handler = {});
    // Go to cycle containing specified run number
    void goToCycle(const QString &journalDirectory, const QString &runNo, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * NeXuS Endpoints
     */
    public:
    // Get NeXuS log values present in specified run files
    void getNexusFields(const Locator &location, const std::vector<int> &runNos,
                        HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS log value data for specified run files
    void getNexusLogValueData(const Locator &location, const std::vector<int> &runNos, const QString &logValue,
                              HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor range for specified run number
    void getNexusMonitorRange(const Locator &location, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor spectrum for specified run numbers
    void getNexusMonitor(const Locator &location, const std::vector<int> &runNos, int monitorId,
                         HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS spectrum range for specified run numbers
    void getNexusSpectrumRange(const Locator &location, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra for specified run numbers
    void getNexusDetector(const Locator &location, const std::vector<int> &runNos, int monitorId,
                          HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra analysis for specified run number
    void getNexusDetectorAnalysis(const Locator &location, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * Generation Endpoints
     */
    public:
    // List data directory for the specified source
    void listDataDirectory(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
    // Generate journals for the specified source
    void generateJournals(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
};
