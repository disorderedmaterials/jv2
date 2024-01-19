// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QNetworkReply>
#include <QTimer>

// Perform check for errors on http request
bool MainWindow::handleRequestErrors(HttpRequestWorker *worker, const QString &taskDescription)
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

    auto response = worker->jsonResponse().object();

    // Invalid request?
    if (response.contains(InvalidRequestError))
    {
        statusBar()->showMessage("Request Error", 3000);
        setErrorPage(
            "Invalid Request Error",
            QString("The backend didn't like our request while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // Network error?
    if (response.contains(NetworkError))
    {
        statusBar()->showMessage("Network Error", 3000);
        setErrorPage(
            "Invalid Request Error",
            QString("Network file retrieval failed while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // XML parsing error?
    if (response.contains(XMLParseError))
    {
        statusBar()->showMessage("Parsing Error", 3000);
        setErrorPage(
            "XML Error",
            QString("XML parsing failed while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // Collection not found?
    if (response.contains(CollectionNotFoundError))
    {
        statusBar()->showMessage(QString("Collection error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage(
            "Collection Not Found",
            QString("Collection not found while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // Journal not found?
    if (response.contains(JournalNotFoundError))
    {
        statusBar()->showMessage(QString("Journal error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage(
            "Journal Not Found",
            QString("Journal not found while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    // File not found?
    if (response.contains(FileNotFoundError))
    {
        statusBar()->showMessage(QString("File error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage(
            "File Not Found",
            QString("File not found while %1.\nThe error returned was: %2").arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return true;
    }

    return false;
}
