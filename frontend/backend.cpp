// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "backend.h"
#include "args.h"
#include "dataSource.h"
#include "httpRequestWorker.h"
#include "locator.h"
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

// List available journals in the specified directory
void Backend::listJournals(const DataSource &source, const QString &journalDirectory,
                           HttpRequestWorker::HttpRequestHandler handler)
{
    QJsonObject data;
    data["rootUrl"] = source.rootUrl();
    data["directory"] = journalDirectory;
    data["dataDirectory"] = source.networkDataDirectory();
    if (!source.indexFile().isEmpty())
        data["indexFile"] = source.indexFile();

    postRequest(createRoute("journals/list"), data, handler);
}

// Get journal file at the specified location
void Backend::getJournal(const Locator &location, HttpRequestWorker::HttpRequestHandler handler)
{
    QJsonObject data;
    data["rootUrl"] = location.rootUrl();
    data["directory"] = location.directory();
    data["journalFile"] = location.filename();

    postRequest(createRoute("journals/get"), data, handler);
}

// Get any updates to the specified journal
void Backend::getJournalUpdates(const Locator &location, HttpRequestWorker::HttpRequestHandler handler)
{
    QJsonObject data;
    data["rootUrl"] = location.rootUrl();
    data["directory"] = location.directory();
    data["journalFile"] = location.filename();

    postRequest(createRoute("journals/getUpdates"), data, handler);
}

// Search all journals for matching runs
void Backend::findRuns(const QString &journalDirectory, const QString &value, const QString &textInput, const QString options,
                       HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("journals/findRuns", journalDirectory, value, textInput, options), handler);
}

// Get total uAmps for run numbers in the given cycle
void Backend::getRunTotalMuAmps(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                                HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("journals/getTotalMuAmps", dataDirectory, cycle, runNos), handler);
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
void Backend::getNexusFields(const Locator &location, const std::vector<int> &runNos,
                             HttpRequestWorker::HttpRequestHandler handler)
{
    QJsonObject data;
    data["rootUrl"] = location.rootUrl();
    data["directory"] = location.directory();

    QJsonArray runNumbers;
    for (auto i : runNos)
        runNumbers.append(i);
    data["runNumbers"] = runNumbers;

    postRequest(createRoute("runData/nexus/getLogValues"), data, handler);
}

// Get NeXuS log value data for specified run files
void Backend::getNexusLogValueData(const QString &dataDirectory, const QString &cycles, const QString &runNos,
                                   const QString &logValue, HttpRequestWorker::HttpRequestHandler handler)
{
    // Log values typically contain '/' in their name as they are paths, so swap with ':' so we can handle it properly
    // [FIXME Can we escape this, or use %2F, or encode it in some other way]
    auto cleanedLogValue = logValue;
    cleanedLogValue.replace("/", ":");
    createRequest(createRoute("runData/nexus/getLogValueData", dataDirectory, cycles, runNos, cleanedLogValue), handler);
}

// Get NeXuS monitor range for specified run numbers in the given cycle
void Backend::getNexusMonitorRange(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                                   HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("runData/nexus/getMonitorRange", dataDirectory, cycle, runNos), handler);
}

// Get NeXuS monitor spectrum for specified run numbers in the given cycle
void Backend::getNexusMonitor(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                              const QString &spectrumID, HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("runData/nexus/getMonitorSpectrums", dataDirectory, cycle, runNos, spectrumID), handler);
}

// Get NeXuS spectrum range for specified run numbers in the given cycle
void Backend::getNexusSpectrumRange(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                                    HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("runData/nexus/getSpectrumRange", dataDirectory, cycle, runNos), handler);
}

// Get NeXuS detector spectra for specified run numbers in the given cycle
void Backend::getNexusDetector(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                               const QString &spectrumID, HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("runData/nexus/getSpectrum", dataDirectory, cycle, runNos, spectrumID), handler);
}

// Get NeXuS detector spectra analysis for specified run numbers in the given cycle [FIXME - bad name]
void Backend::getNexusDetectorAnalysis(const QString &dataDirectory, const QString &runNos, const QString &cycle,
                                       HttpRequestWorker::HttpRequestHandler handler)
{
    createRequest(createRoute("runData/nexus/getDetectorAnalysis", dataDirectory, runNos, cycle), handler);
}
