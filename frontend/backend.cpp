// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "backend.h"
#include "args.h"
#include "httpRequestWorker.h"
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
 * Endpoint Access
 */

// Create a request
HttpRequestWorker *Backend::createRequest(const QString &url, WorkerHandlingFunction handler)
{
    auto *worker = new HttpRequestWorker(manager_);

     if (handler)
        connect(worker, &HttpRequestWorker::on_execution_finished,
                [=](HttpRequestWorker *workerProxy) { handler(workerProxy); });
    worker->execute(createRoute("ping"));

    return worker;
}

// Ping backend to see if its alive
void Backend::ping(WorkerHandlingFunction handler)
{
    createRequest(createRoute("ping"), handler);
}
