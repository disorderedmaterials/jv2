// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
#include <QNetworkAccessManager>
#include <QProcess>
#include <QString>

// Forward-declarations
class JournalSource;
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
    // Ping backend with a message
    void ping(const QString &message, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * Journal Endpoints
     */
    public:
    // Get journal index for the specified source
    void getJournalIndex(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get journal file at the specified location
    void getJournal(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get any updates to the specified current journal in the specified source
    void getJournalUpdates(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
    // Search across all journals for matching runs
    void search(const JournalSource &source, const std::map<QString, QString> &searchTerms,
                HttpRequestWorker::HttpRequestHandler handler = {});
    // Go to cycle containing specified run number
    void goToCycle(const QString &journalDirectory, const QString &runNo, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * NeXuS Endpoints
     */
    public:
    // Get NeXuS log values present in specified run files
    void getNexusFields(const JournalSource &source, const std::vector<int> &runNos,
                        HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS log value data for specified run files
    void getNexusLogValueData(const JournalSource &source, const std::vector<int> &runNos, const QString &logValue,
                              HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor range for specified run number
    void getNexusMonitorRange(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS monitor spectrum for specified run numbers
    void getNexusMonitor(const JournalSource &source, const std::vector<int> &runNos, int monitorId,
                         HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS spectrum range for specified run numbers
    void getNexusSpectrumRange(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra for specified run numbers
    void getNexusDetector(const JournalSource &source, const std::vector<int> &runNos, int monitorId,
                          HttpRequestWorker::HttpRequestHandler handler = {});
    // Get NeXuS detector spectra analysis for specified run number
    void getNexusDetectorAnalysis(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler = {});

    /*
     * Generation Endpoints
     */
    public:
    // List data directory for the specified source
    void listDataDirectory(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
    // Generate journals for the specified source
    void generateJournals(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler = {});
};
