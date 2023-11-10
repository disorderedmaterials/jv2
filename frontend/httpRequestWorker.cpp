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
    errorType_ = reply_->error();
    if (errorType_ == QNetworkReply::NoError)
    {
        response_ = reply_->readAll();
        jsonResponse_ = QJsonDocument::fromJson(response_.toUtf8());
    }
    else
        errorString_ = reply_->errorString();

    reply_->deleteLater();

    emit requestFinished(this);
}

/*
 * Result Data
 */

// Return raw response string
const QString &HttpRequestWorker::response() const { return response_; }

// Return esponse formatted as JSON
const QJsonDocument &HttpRequestWorker::jsonResponse() const { return jsonResponse_; }

// Return error type
QNetworkReply::NetworkError HttpRequestWorker::errorType() const { return errorType_; }

// Return error string (if available)
const QString &HttpRequestWorker::errorString() const { return errorString_; }
