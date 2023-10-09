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

    private:
    // Network request object
    QNetworkRequest request_;
    // Network reply
    QNetworkReply *reply_{nullptr};

    public:
    QString response;
    QNetworkReply::NetworkError errorType{QNetworkReply::NoError};
    QString errorString;
    QJsonDocument jsonResponse;
    QJsonArray jsonArray;

    signals:
    void requestFinished(HttpRequestWorker *worker);

    private slots:
    // Process request once its complete
    void requestComplete();
};
