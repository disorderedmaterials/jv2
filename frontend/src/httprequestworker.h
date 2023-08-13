// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 Team JournalViewer and contributors

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

// Object for request URL
class HttpRequestInput
{

    public:
    QString url_str;

    HttpRequestInput(QString v_url_str);
};

// Object for handling http request
class HttpRequestWorker : public QObject
{
    Q_OBJECT

    public:
    QString response;
    QNetworkReply::NetworkError errorType;
    QString errorString;
    QJsonDocument jsonResponse;
    QJsonArray jsonArray;

    explicit HttpRequestWorker(QObject *parent = 0);

    void execute(HttpRequestInput input);

    signals:
    void on_execution_finished(HttpRequestWorker *worker);

    private:
    QNetworkAccessManager *manager_;

    private slots:
    void on_manager_finished(QNetworkReply *reply);
};

