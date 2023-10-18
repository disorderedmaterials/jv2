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

HttpRequestWorker::HttpRequestWorker(QNetworkAccessManager &manager, const QString &url, const QJsonObject &data,
                                     HttpRequestHandler handler)
{
    // Set up the request
    request_.setUrl(url);
    request_.setHeader(QNetworkRequest::UserAgentHeader, "JournalViewer2");
    request_.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (handler)
        connect(this, &HttpRequestWorker::requestFinished, [=](HttpRequestWorker *workerProxy) { handler(workerProxy); });

    // Create POST data from the supplied QJsonObject
    postData_ = QJsonDocument(data).toJson();

    // Execute the request and connect the reply
    reply_ = manager.post(request_, postData_);
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
