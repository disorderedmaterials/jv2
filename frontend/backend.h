// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

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
    HttpRequestWorker *postRequest(const QString &url, const QJsonObject &data,
                                   const HttpRequestWorker::HttpRequestHandler &handler);
    // Create a request
    HttpRequestWorker *createRequest(const QString &url, const HttpRequestWorker::HttpRequestHandler &handler = {});

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
    void ping(const HttpRequestWorker::HttpRequestHandler &handler = {});

    /*
     * Journal Endpoints
     */
    public:
    // Get journal index for the specified source
    void getJournalIndex(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get journal file at the specified location
    void getJournal(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get any updates to the specified current journal in the specified source
    void getJournalUpdates(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get number of uncached journals for specified source
    void getUncachedJournalCount(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Search across all journals for matching runs
    void search(const JournalSource *source, const std::map<QString, QString> &searchTerms,
                const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Find journal containing specified run number
    void findJournal(const JournalSource *source, int runNo, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get all journals for source in background
    void acquireAllJournals(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Request update on background scan
    void acquireAllJournalsUpdate(const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Stop background scan
    void acquireAllJournalsStop(const HttpRequestWorker::HttpRequestHandler &handler = {});

    /*
     * NeXuS Endpoints
     */
    public:
    // Get NeXuS log values present in specified run files
    void getNexusFields(const JournalSource *source, const std::vector<int> &runNos,
                        const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get NeXuS log value data for specified run files
    void getNexusLogValueData(const JournalSource *source, const std::vector<int> &runNos, const QString &logValue,
                              const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get NeXuS spectrum count for specified run number
    void getNexusSpectrumCount(const JournalSource *source, const QString &spectrumType, int runNo,
                               const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get NeXuS spectrum for specified run numbers
    void getNexusSpectrum(const JournalSource *source, const QString &spectrumType, int monitorId,
                          const std::vector<int> &runNos, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Get NeXuS detector spectra analysis for specified run number
    void getNexusDetectorAnalysis(const JournalSource *source, int runNo,
                                  const HttpRequestWorker::HttpRequestHandler &handler = {});

    /*
     * Generation Endpoints
     */
    public:
    // Journal Generation Styles
    enum class JournalGenerationStyle
    {
        Full,
        UpdateAll
    };
    // Return text for journal generation style
    static QString journalGenerationStyle(JournalGenerationStyle style);
    // Generate data file list for the specified source
    void generateList(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Perform scan of data files discovered in the specified source
    void generateScan(const JournalSource *source, JournalGenerationStyle generationStyle,
                      const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Request update on background scan
    void generateScanUpdate(const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Stop background scan
    void generateScanStop(const HttpRequestWorker::HttpRequestHandler &handler = {});
    // Finalise journals from scanned data
    void generateFinalise(const JournalSource *source, JournalGenerationStyle generationStyle,
                          const HttpRequestWorker::HttpRequestHandler &handler = {});
};
