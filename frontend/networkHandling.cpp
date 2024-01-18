// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QNetworkReply>
#include <QTimer>

// Perform error check on http result
bool MainWindow::networkReplyHasError(HttpRequestWorker *worker, const QString &taskDescription)
{
    // Communications error with the backend?
    if (worker->errorType() != QNetworkReply::NoError)
    {
        statusBar()->showMessage(QString("Network error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage("Network Error", QString("A network error was encountered while %1.\nThe error returned was: %2")
                                          .arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // Response error?
    auto response = worker->jsonResponse().object();
    if (response.contains("Error"))
    {
        statusBar()->showMessage(QString("Response error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage("Response Error", QString("The backend failed while %1.\nThe response returned was: %2")
                                           .arg(taskDescription, response["Error"].toString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    return false;
}
