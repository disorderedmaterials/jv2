// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "backend.h"
#include "args.h"
#include "httpRequestWorker.h"
#include "journalSource.h"
#include <QCommandLineParser>
#include <QProcessEnvironment>

Backend::Backend(const QCommandLineParser &args) : process_()
{
    QStringList backendArgs;

    process_.setProgram("jv2backend");
    backendArgs << "-b" << bindAddress();
    backendArgs << "-t"
                << "300";
    if (args.isSet(CLIArgs::DebugBackend))
        backendArgs << "-d";
    if (args.isSet(CLIArgs::UseWaitress))
    {
        backendArgs << "-w";
        waitressBackend_ = true;
    }

    process_.setArguments(backendArgs);
    process_.setProcessChannelMode(QProcess::ForwardedChannels);
}

/*
 * Private Functions
 */

// Return the backend bind address
QString Backend::bindAddress() const { return "127.0.0.1:5000"; };

// Create a POST request
HttpRequestWorker *Backend::postRequest(const QString &url, const QJsonObject &data,
                                        const HttpRequestWorker::HttpRequestHandler &handler)
{
    return new HttpRequestWorker(manager_, url, data, handler);
}

// Create a request
HttpRequestWorker *Backend::createRequest(const QString &url, const HttpRequestWorker::HttpRequestHandler &handler)
{
    return new HttpRequestWorker(manager_, url, handler);
}

/*
 * Public Slots
 */

// Start the backend process
void Backend::start()
{
    qDebug() << "Starting backend process " << process_.program() << " with arguments ";
    for (const auto &arg : process_.arguments())
        qDebug() << arg;

    process_.start();
    if (process_.waitForStarted())
    {
        qDebug() << "Backend process started with pid " << process_.processId();
        emit(started("OK"));
    }
    else
    {
        qDebug() << "Error starting backend " << process_.errorString();
        emit(started(process_.errorString()));
    }
}

// Stop the backend process
void Backend::stop()
{
    qDebug() << "Stopping backend process with pid " << process_.processId();

    // Gracefully inform the backend to quit
    createRequest(createRoute("shutdown"));

    // Send the TERM signal to gunicorn, but send the KILL signal to waitress. Dontcha just love cross-platform development?
    if (waitressBackend_)
        process_.kill();
    else
        process_.terminate();

    process_.waitForFinished();
}

/*
 * Server Endpoints
 */

// Ping backend to see if its alive
void Backend::ping(const HttpRequestWorker::HttpRequestHandler &handler) { createRequest(createRoute("ping"), handler); }

/*
 * Journal Endpoints
 */

// Get journal index for the specified source
void Backend::getJournalIndex(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    postRequest(createRoute("journals/index"), source->sourceObjectData(), handler);
}

// Get current journal file for the specified source
void Backend::getJournal(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    postRequest(createRoute("journals/get"), source->currentJournalObjectData(), handler);
}

// Get any updates to the specified current journal in the specified source
void Backend::getJournalUpdates(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    postRequest(createRoute("journals/getUpdates"), source->currentJournalObjectData(), handler);
}

// Get number of uncached journals for specified source
void Backend::getUncachedJournalCount(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    postRequest(createRoute("journals/getUncachedJournalCount"), source->currentJournalObjectData(), handler);
}

// Search across all journals for matching runs
void Backend::search(const JournalSource *source, const std::map<QString, QString> &searchTerms,
                     const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();

    QJsonObject query;
    query["caseSensitive"] = "false";
    for (const auto &[key, value] : searchTerms)
        query[key] = value;

    data["valueMap"] = query;

    postRequest(createRoute("journals/search"), data, handler);
}

// Find journal containing specified run number
void Backend::findJournal(const JournalSource *source, int runNo, const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();
    data["runNumbers"] = QJsonArray({QJsonValue(runNo)});

    postRequest(createRoute("journals/findJournal"), data, handler);
}

// Get all journals for source in background
void Backend::acquireAllJournals(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    postRequest(createRoute("acquire"), source->sourceObjectData(), handler);
}

// Request update on background journal acquisition scan
void Backend::acquireAllJournalsUpdate(const HttpRequestWorker::HttpRequestHandler &handler)
{
    createRequest(createRoute("acquire/update"), handler);
}

// Stop background journal acquisition scan
void Backend::acquireAllJournalsStop(const HttpRequestWorker::HttpRequestHandler &handler)
{
    createRequest(createRoute("acquire/stop"), handler);
}

/*
 * NeXuS Endpoints
 */

// Get NeXuS log values present in specified run files
void Backend::getNexusFields(const JournalSource *source, const std::vector<int> &runNos,
                             const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getLogValues"), data, handler);
}

// Get NeXuS log value data for specified run files
void Backend::getNexusLogValueData(const JournalSource *source, const std::vector<int> &runNos, const QString &logValue,
                                   const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;
    data["logValue"] = logValue;

    postRequest(createRoute("runData/nexus/getLogValueData"), data, handler);
}

// Get NeXuS spectrum count for specified run number
void Backend::getNexusSpectrumCount(const JournalSource *source, const QString &spectrumType, int runNo,
                                    const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();
    data["runNumbers"] = QJsonArray({QJsonValue(runNo)});
    data["spectrumType"] = spectrumType;

    postRequest(createRoute("runData/nexus/getSpectrumCount"), data, handler);
}

// Get NeXuS spectrum for specified run numbers
void Backend::getNexusSpectrum(const JournalSource *source, const QString &spectrumType, int monitorId,
                               const std::vector<int> &runNos, const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();
    data["spectrumId"] = monitorId;
    data["spectrumType"] = spectrumType;

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getSpectrum"), data, handler);
}

// Get NeXuS detector spectra analysis for specified run numbers in the given cycle [FIXME - bad name]
void Backend::getNexusDetectorAnalysis(const JournalSource *source, int runNo,
                                       const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->sourceObjectData();
    data["runNumbers"] = runNo;

    postRequest(createRoute("runData/nexus/getDetectorAnalysis"), data, handler);
}

/*
 * Generation Endpoints
 */

// Return text for journal generation style
QString Backend::journalGenerationStyle(JournalGenerationStyle style)
{
    switch (style)
    {
        case (JournalGenerationStyle::Full):
            return "full";
        case (JournalGenerationStyle::UpdateAll):
            return "updateAll";
        default:
            throw(std::runtime_error("Unrecognised JournalGenerationStyle so can't return text for it.\n"));
    }
}

// Generate data file list for the specified source
void Backend::generateList(const JournalSource *source, const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->currentJournalObjectData();
    data["rootRegExpSelector"] = source->runDataRootRegExp();

    postRequest(createRoute("generate/list"), data, handler);
}

// Scan data files discovered in the specified source
void Backend::generateScan(const JournalSource *source, JournalGenerationStyle generationStyle,
                           const HttpRequestWorker::HttpRequestHandler &handler)
{
    // Only for disk-based sources
    if (source->type() == JournalSource::IndexingType::Network)
        throw(std::runtime_error("Can't generate journals for a network source.\n"));

    auto data = source->currentJournalObjectData();
    data["sortKey"] = JournalSource::dataOrganisationTypeSortKey(source->dataOrganisation());
    data["scanType"] = journalGenerationStyle(generationStyle);

    postRequest(createRoute("generate/scan"), data, handler);
}

// Request update on background scan
void Backend::generateScanUpdate(const HttpRequestWorker::HttpRequestHandler &handler)
{
    createRequest(createRoute("generate/scanUpdate"), handler);
}

// Stop background scan
void Backend::generateScanStop(const HttpRequestWorker::HttpRequestHandler &handler)
{
    createRequest(createRoute("generate/stopScan"), handler);
}

// Finalise journals from scanned data
void Backend::generateFinalise(const JournalSource *source, JournalGenerationStyle generationStyle,
                               const HttpRequestWorker::HttpRequestHandler &handler)
{
    auto data = source->currentJournalObjectData();
    data["sortKey"] = JournalSource::dataOrganisationTypeSortKey(source->dataOrganisation());
    data["scanType"] = journalGenerationStyle(generationStyle);

    postRequest(createRoute("generate/finalise"), data, handler);
}
