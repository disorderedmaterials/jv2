// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs
#include "backend.h"
#include <QDebug>

/**
 * Configure process for start to be called
 */
Backend::Backend()
{
    process_.setProgram("gunicorn");
    QStringList args;
    // clang-format off
    args << "--bind" << Backend::bind_address()
         << "--graceful-timeout" << "120"
         << "--timeout" << "120"
         << "jv2backend.app:create_app";
    // clang-format on
    process_.setArguments(args);
    process_.setProcessChannelMode(QProcess::ForwardedChannels);
}

/**
 * Start the backend process and hold the process open
 */
void Backend::start()
{
    qDebug() << "Starting backend";
    process_.start();
    process_.waitForStarted();
}

/**
 * Stop the process
 */
void Backend::stop()
{
    qDebug() << "Stopping backend.";
    process_.terminate();
    process_.waitForFinished();
}
