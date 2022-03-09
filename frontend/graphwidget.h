// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "chartview.h"
#include "httprequestworker.h"
#include <QChart>
#include <QChartView>
#include <QWidget>

namespace Ui
{
class GraphWidget;
}

class GraphWidget : public QWidget
{
    Q_OBJECT

    public:
    GraphWidget(QWidget *parent = nullptr, QChart *chart = nullptr);
    ~GraphWidget();
    ChartView *getChartView();

    public slots:
    void modify(double val, bool checked);
    void modifyAgainstRun(HttpRequestWorker *worker, bool checked);

    private slots:
    void on_binWidths_clicked(bool checked);
    void on_muAmps_clicked(bool checked);
    void on_runDivide_clicked(bool checked);
    void on_monDivide_clicked(bool checked);

    private:
    Ui::GraphWidget *ui_;
    QString run_;

    signals:
    void test(bool checked);
    void runDivide(QString run, bool checked);
    void monDivide(QString currentRun, QString mon, bool checked);
};

#endif
