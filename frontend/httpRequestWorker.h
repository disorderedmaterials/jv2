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
    
    protected:
    explicit HttpRequestWorker(QNetworkAccessManager &manager);

    private:
    // Parent network manager
    QNetworkAccessManager &manager_;

    public:
    QString response;
    QNetworkReply::NetworkError errorType;
    QString errorString;
    QJsonDocument jsonResponse;
    QJsonArray jsonArray;

    void execute(const QString &url);

    signals:
    void on_execution_finished(HttpRequestWorker *worker);

    private slots:
    void on_manager_finished(QNetworkReply *reply);
};
