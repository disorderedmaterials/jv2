// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "httpRequestWorker.h"
#include <QBuffer>
#include <QJsonDocument>
#include <QUrl>

HttpRequestWorker::HttpRequestWorker(QNetworkAccessManager &manager, const QString &url, HttpRequestHandler handler) : QObject()
{
    // Set up the request
    request_.setUrl(url);
    request_.setRawHeader("User-Agent", "JournalViewer 2");

    if (handler)
        connect(this, &HttpRequestWorker::requestFinished, [=](HttpRequestWorker *workerProxy) { handler(workerProxy); });

    // Execute the request and connect the reply
    reply_ = manager.get(request_);
    connect(reply_, &QNetworkReply::finished, this, &HttpRequestWorker::requestComplete);
}

// Process request
void HttpRequestWorker::requestComplete()
{
    errorType = reply_->error();
    if (errorType == QNetworkReply::NoError)
    {
        response = reply_->readAll();
        jsonResponse = QJsonDocument::fromJson(response.toUtf8());
        jsonArray = jsonResponse.array();
    }
    else
        errorString = reply_->errorString();

    reply_->deleteLater();

    emit requestFinished(this);
}
