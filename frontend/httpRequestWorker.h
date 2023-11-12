// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QObject>
#include <QString>

// Forward Declarations
class QNetworkAccessManager;
class QNetworkReply;

// Object for handling an http request
class HttpRequestWorker : public QObject
{
    Q_OBJECT

    friend class Backend;

    public:
    // Typedef for worker handling function
    using HttpRequestHandler = std::function<void(HttpRequestWorker *)>;

    protected:
    HttpRequestWorker(QNetworkAccessManager &manager, const QString &url, HttpRequestHandler handler = {});
    HttpRequestWorker(QNetworkAccessManager &manager, const QString &url, const QJsonObject &data,
                      HttpRequestHandler handler = {});

    private:
    // Network request object
    QNetworkRequest request_;
    // Network reply
    QNetworkReply *reply_{nullptr};
    // Post data (if specified)
    QByteArray postData_;

    /*
     * Result Data
     */
    private:
    // Raw response string
    QString response_;
    // Error type
    QNetworkReply::NetworkError errorType_{QNetworkReply::NoError};
    // Error string (if available)
    QString errorString_;
    // Response formatted as JSON
    QJsonDocument jsonResponse_;

    public:
    // Return raw response string
    const QString &response() const;
    // Return esponse formatted as JSON
    const QJsonDocument &jsonResponse() const;
    // Return error type
    QNetworkReply::NetworkError errorType() const;
    // Return error string (if available)
    const QString &errorString() const;

    signals:
    void requestFinished(HttpRequestWorker *worker);

    private slots:
    // Process request once its complete
    void requestComplete();
};
