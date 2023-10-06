// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

// Object for handling an http request
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


    void execute(const QString &url);

    signals:
    void on_execution_finished(HttpRequestWorker *worker);

    private:
    QNetworkAccessManager *manager_;

    private slots:
    void on_manager_finished(QNetworkReply *reply);
};
