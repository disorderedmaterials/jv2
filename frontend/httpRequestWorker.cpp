// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "httpRequestWorker.h"
#include <QBuffer>
#include <QJsonDocument>
#include <QUrl>

HttpRequestWorker::HttpRequestWorker(QNetworkAccessManager &manager) : QObject(), manager_(manager)
{
    connect(&manager_, SIGNAL(finished(QNetworkReply *)), this, SLOT(on_manager_finished(QNetworkReply *)));
}

// Execute request
void HttpRequestWorker::execute(const QString &url)
{
    // reset variables
    response = "";
    errorType = QNetworkReply::NoError;
    errorString = "";

    // execute connection
    QNetworkRequest request = QNetworkRequest(url);
    request.setRawHeader("User-Agent", "Agent name goes here");
    manager_.get(request);
}

// Process request
void HttpRequestWorker::on_manager_finished(QNetworkReply *reply)
{
    errorType = reply->error();
    if (errorType == QNetworkReply::NoError)
    {
        response = reply->readAll();
        jsonResponse = QJsonDocument::fromJson(response.toUtf8());
        jsonArray = jsonResponse.array();
    }
    else
        errorString = reply->errorString();

    reply->deleteLater();

    emit on_execution_finished(this);
}
