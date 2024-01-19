// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QNetworkReply>

// Perform check for errors on http request, returning the handled error
QString MainWindow::handleRequestError(HttpRequestWorker *worker, const QString &taskDescription)
{
    // Communications error with the backend?
    if (worker->errorType() != QNetworkReply::NoError)
    {
        statusBar()->showMessage(QString("Network error for source %1").arg(currentJournalSource()->name()), 3000);
        setErrorPage("Network Error", QString("A network error was encountered while %1.\nThe error returned was: %2")
                                          .arg(taskDescription, worker->errorString()));
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return QNetworkReplyError;
    }

    // Error message parts (formatted error type, error page pre-text) associated to error return types
    std::map<QString, std::pair<QString, QString>> errorParts = {
        {InvalidRequestError, {"Invalid Request", "The backend didn't like our request"}},
        {NetworkError, {"Network Error", "Network file retrieval failed"}},
        {XMLParseError, {"XML Parse Error", "XML parsing failed"}},
        {CollectionNotFoundError, {"Collection Not Found", "Collection not found"}},
        {JournalNotFoundError, {"Journal Not Found", "Journal not found"}},
        {FileNotFoundError, {"File Not Found", "File not found"}}};

    // Check response for recognised error types
    auto response = worker->jsonResponse().object();
    for (auto &&[errorKey, errorData] : errorParts)
    {
        if (response.contains(errorKey))
        {
            statusBar()->showMessage(errorData.first, 3000);
            setErrorPage(errorData.first, QString("%1 while %2.\nThe error returned was: %3")
                                              .arg(errorData.second, taskDescription, response[errorKey].toString()));
            updateForCurrentSource(JournalSource::JournalSourceState::Error);
            return errorKey;
        }
    }

    return NoError;
}

// Update the error page
void MainWindow::setErrorPage(const QString &errorTitle, const QString &errorText)
{
    ui_.ErrorLabel->setText(errorTitle);
    ui_.ErrorInfoLabel->setText(errorText);
}

void MainWindow::on_ErrorOKButton_clicked(bool checked) { updateForCurrentSource(JournalSource::JournalSourceState::OK); }
