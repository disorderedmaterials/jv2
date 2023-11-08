// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "backend.h"
#include "args.h"
#include "httpRequestWorker.h"
#include "journalSource.h"
#include <QCommandLineParser>
#include <QProcessEnvironment>

namespace
{
// This must match that defined in backend/config module
constexpr auto ENVIRON_NAME_PREFIX = "JV2_";

/**
 * Take a program argument name and convert to a backend environment variable name.
 * Replace '-' with '_' and add prefix
 */
QString argToEnvironName(QString argName) { return ENVIRON_NAME_PREFIX + argName.replace("-", "_").toUpper(); }
} // namespace

Backend::Backend(const QCommandLineParser &args) : process_()
{
    configureProcessArgs(args);
    configureEnvironment(args);
}

/*
 * Private Functions
 */

// Return the backend bind address
QString Backend::bindAddress() const { return "127.0.0.1:5000"; };

// Create a POST request
HttpRequestWorker *Backend::postRequest(const QString &url, const QJsonObject &data,
                                        HttpRequestWorker::HttpRequestHandler handler)
{
    return new HttpRequestWorker(manager_, url, data, handler);
}

// Create a request
HttpRequestWorker *Backend::createRequest(const QString &url, HttpRequestWorker::HttpRequestHandler handler)
{
    return new HttpRequestWorker(manager_, url, handler);
}

// Configure backend process arguments
void Backend::configureProcessArgs(const QCommandLineParser &args)
{
    process_.setProgram("gunicorn");
    QStringList backendArgs;

    backendArgs << "--bind" << Backend::bindAddress() << "--graceful-timeout"
                << "120"
                << "--timeout"
                << "120";
    if (!args.isSet(Args::LogLevel))
        backendArgs << "--log-level"
                    << "debug";
    backendArgs << "jv2backend.app:create_app()";

    process_.setArguments(backendArgs);
    process_.setProcessChannelMode(QProcess::ForwardedChannels);
}

// Configure backend process environments
void Backend::configureEnvironment(const QCommandLineParser &args)
{
    QProcessEnvironment env;
    if (args.isSet(Args::RunLocatorClass))
        env.insert(argToEnvironName(Args::RunLocatorClass), args.value(Args::RunLocatorClass));
    if (args.isSet(Args::RunLocatorPrefix))
        env.insert(argToEnvironName(Args::RunLocatorPrefix), args.value(Args::RunLocatorPrefix));

    if (env.isEmpty())
    {
        qDebug() << "Configured additional environment variables for backend:";
        for (const auto &keyValue : env.toStringList())
            qDebug() << keyValue;
    }
    env.insert(QProcessEnvironment::systemEnvironment());
    process_.setProcessEnvironment(env);
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
    process_.terminate();
    process_.waitForFinished();
}

/*
 * Server Endpoints
 */

// Ping backend to see if its alive
void Backend::ping(HttpRequestWorker::HttpRequestHandler handler) { createRequest(createRoute("ping"), handler); }

/*
 * Journal Endpoints
 */

// Get journal index for the specified source
void Backend::getJournalIndex(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler)
{
    postRequest(createRoute("journals/index"), source.sourceObjectData(), handler);
}

// Get current journal file for the specified source
void Backend::getJournal(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler)
{
    postRequest(createRoute("journals/get"), source.currentJournalObjectData(), handler);
}

// Get any updates to the specified current journal in the specified source
void Backend::getJournalUpdates(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler)
{
    postRequest(createRoute("journals/getUpdates"), source.currentJournalObjectData(), handler);
}

// Search across all journals for matching runs
void Backend::search(const JournalSource &source, const std::map<QString, QString> &searchTerms,
                     HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();

    QJsonObject query;
    query["caseSensitive"] = "false";
    for (const auto &[key, value] : searchTerms)
        query[key] = value;

    data["valueMap"] = query;

    postRequest(createRoute("journals/search"), data, handler);
}

// Go to cycle containing specified run number
void Backend::goToCycle(const QString &journalDirectory, const QString &runNo, HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("journals/goToCycle", journalDirectory, runNo), handler);
}

/*
 * NeXuS Endpoints
 */

// Get NeXuS log values present in specified run files
void Backend::getNexusFields(const JournalSource &source, const std::vector<int> &runNos,
                             HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getLogValues"), data, handler);
}

// Get NeXuS log value data for specified run files
void Backend::getNexusLogValueData(const JournalSource &source, const std::vector<int> &runNos, const QString &logValue,
                                   HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;
    data["logValue"] = logValue;

    postRequest(createRoute("runData/nexus/getLogValueData"), data, handler);
}

// Get NeXuS monitor range for specified run numbers in the given cycle
void Backend::getNexusMonitorRange(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();
    data["runNumber"] = runNo;

    postRequest(createRoute("runData/nexus/getMonitorRange"), data, handler);
}

// Get NeXuS monitor spectrum for specified run numbers in the given cycle
void Backend::getNexusMonitor(const JournalSource &source, const std::vector<int> &runNos, int monitorId,
                              HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();
    data["spectrumId"] = monitorId;

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getMonitorSpectrum"), data, handler);
}

// Get NeXuS spectrum range for specified run number
void Backend::getNexusSpectrumRange(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();
    data["runNumber"] = runNo;

    postRequest(createRoute("runData/nexus/getSpectrumRange"), data, handler);
}

// Get NeXuS detector spectra for specified run numbers in the given cycle
void Backend::getNexusDetector(const JournalSource &source, const std::vector<int> &runNos, int monitorId,
                               HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();
    data["spectrumId"] = monitorId;

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getSpectrum"), data, handler);
}

// Get NeXuS detector spectra analysis for specified run numbers in the given cycle [FIXME - bad name]
void Backend::getNexusDetectorAnalysis(const JournalSource &source, int runNo, HttpRequestWorker::HttpRequestHandler handler)
{
    auto data = source.sourceObjectData();
    data["runNumber"] = runNo;

    postRequest(createRoute("runData/nexus/getDetectorAnalysis"), data, handler);
}

/*
 * Generation Endpoints
 */

// List data directory for the specified source
void Backend::listDataDirectory(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler)
{
    postRequest(createRoute("generate/list"), source.sourceObjectData(), handler);
}

// Generate journals for the specified source
void Backend::generateJournals(const JournalSource &source, HttpRequestWorker::HttpRequestHandler handler)
{
    // Only for disk-based sources
    if (source.type() == JournalSource::IndexingType::Network)
        throw(std::runtime_error("Can't generate journals for a network source.\n"));

    auto data = source.sourceObjectData();
    data["sortKey"] = JournalSource::dataOrganisationTypeSortKey(source.runDataOrganisation());

    postRequest(createRoute("generate/go"), data, handler);
}
