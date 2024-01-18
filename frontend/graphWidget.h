// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include "chartView.h"
#include "httpRequestWorker.h"
#include "ui_graphWidget.h"
#include <QChart>
#include <QChartView>
#include <QWidget>

class GraphWidget : public QWidget
{
    Q_OBJECT

    public:
    GraphWidget(QWidget *parent = nullptr, QChart *chart = nullptr, QString type = nullptr);
    ~GraphWidget();

    private:
    Ui::GraphWidget ui_;
    QString run_;
    QString chartRuns_;
    QString chartDetector_;
    QJsonArray chartData_;
    QVector<QVector<double>> binWidths_;
    QString type_;
    QString modified_;

    public:
    ChartView *getChartView();

    QString getChartRuns();
    QString getChartDetector();
    QJsonArray getChartData();

    void setChartRuns(QString chartRuns);
    void setChartDetector(QString chartDetector);
    void setChartData(QJsonArray chartData);
    void setLabel(QString label);

    public slots:
    void modifyAgainstString(QString values, bool checked);
    void modifyAgainstWorker(HttpRequestWorker *worker, bool checked);

    private:
    void getBinWidths();

    private slots:
    void runDivideSpinHandling(); // Handle normalisation conflicts
    void monDivideSpinHandling(); // Handle normalisation conflicts
    void on_countsPerMicrosecondCheck_stateChanged(int state);
    void on_countsPerMicroAmpCheck_stateChanged(int state);

    signals:
    void muAmps(QString runs, bool checked, QString modified);
    void runDivide(QString currentDetector, QString run, bool checked);
    void monDivide(QString currentRun, QString mon, bool checked);
};
