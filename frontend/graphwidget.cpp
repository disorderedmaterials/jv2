// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "graphwidget.h"
#include "./ui_graphwidget.h"
#include "chartview.h"
#include "mainwindow.h"
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QDebug>
#include <QInputDialog>
#include <QJsonArray>
#include <QValueAxis>
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart, QString type) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    type_ = type;
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
    connect(ui_->divideByRunRadio, &QRadioButton::toggled, [=]() { runDivideSpinHandling(); });
    connect(ui_->divideByRunSpin, &QSpinBox::editingFinished, [=]() { runDivideSpinHandling(); });
    connect(ui_->divideByMonitorSpin, &QSpinBox::editingFinished, [=]() { monDivideSpinHandling(); });
    connect(ui_->divideByMonitorRadio, &QRadioButton::toggled, [=]() { monDivideSpinHandling(); });

    modified_ = "-1";
    ui_->divideByRunSpin->setSpecialValueText(tr(" "));
    ui_->divideByMonitorSpin->setSpecialValueText(tr(" "));
    ui_->divideByRunSpin->setValue(-1);
    ui_->divideByMonitorSpin->setValue(-1);
}

GraphWidget::~GraphWidget() {}

QString GraphWidget::getChartRuns() { return chartRuns_; }
QString GraphWidget::getChartDetector() { return chartDetector_; }
QJsonArray GraphWidget::getChartData() { return chartData_; }

void GraphWidget::setChartRuns(QString chartRuns) { chartRuns_ = chartRuns; }
void GraphWidget::setChartDetector(QString chartDetector) { chartDetector_ = chartDetector; }
void GraphWidget::setChartData(QJsonArray chartData)
{
    chartData_ = chartData;
    getBinWidths();
}
void GraphWidget::setLabel(QString label) { ui_->statusLabel->setText(label); }

void GraphWidget::getBinWidths()
{
    binWidths_.clear();
    for (auto run : chartData_)
    {
        QVector<double> binWidths;
        auto runArray = run.toArray();
        for (auto i = 0; i < runArray.count() - 1; i++)
        {
            double binWidth = runArray.at(i + 1)[0].toDouble() - runArray.at(i)[0].toDouble();
            binWidths.append(binWidth);
        }
        binWidths_.append(binWidths);
    }
}

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::runDivideSpinHandling()
{
    QString value = QString::number(ui_->divideByRunSpin->value());
    if (modified_ == value && ui_->divideByRunSpin->isEnabled())
        return;

    if (modified_ != "-1")
    {
        if (type_ == "Detector")
            emit runDivide(chartDetector_, modified_, false);
        else
            emit monDivide(modified_, chartDetector_, false);
    }

    if (!ui_->divideByRunSpin->isEnabled())
    {
        modified_ = "-1";
        ui_->divideByRunSpin->setValue(-1);
        return;
    }

    if (value != "-1")
    {
        if (type_ == "Detector")
            emit runDivide(chartDetector_, value, true);
        else
            emit monDivide(value, chartDetector_, true);
        modified_ = value;
    }
}

void GraphWidget::monDivideSpinHandling()
{
    QString value = QString::number(ui_->divideByMonitorSpin->value());
    if (modified_ == value && ui_->divideByMonitorSpin->isEnabled())
        return;

    if (modified_ != "-1")
        emit monDivide(modified_, chartDetector_, false);

    if (!ui_->divideByMonitorSpin->isEnabled())
    {
        modified_ = "-1";
        ui_->divideByMonitorSpin->setValue(-1);
        return;
    }

    if (value != "-1")
    {
        emit monDivide(value, chartDetector_, true);
        modified_ = value;
    }
}

void GraphWidget::on_countsPerMicrosecondCheck_stateChanged(int state)
{
    auto max = qobject_cast<QValueAxis *>(ui_->chartView->chart()->axes()[1])->max();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (state == Qt::Checked)
            for (auto j = 0; j < points.count(); j++)
            {
                if (points[j].y() == max)
                    ui_->chartView->chart()->axes()[1]->setMax(points[j].y() / binWidths_[i][j]);
                points[j].setY(points[j].y() / binWidths_[i][j]);
            }
        else
            for (auto j = 0; j < points.count(); j++)
            {
                if (points[j].y() == max)
                    ui_->chartView->chart()->axes()[1]->setMax(points[j].y() * binWidths_[i][j]);
                points[j].setY(points[j].y() * binWidths_[i][j]);
            }
        xySeries->replace(points);
    }
}

void GraphWidget::on_countsPerMicroAmpCheck_stateChanged(int state)
{
    if (state == Qt::Checked)
        emit muAmps(chartRuns_, true);
    else
        emit muAmps(chartRuns_, false);
}

void GraphWidget::modify(QString values, bool checked)
{
    auto max = qobject_cast<QValueAxis *>(ui_->chartView->chart()->axes()[1])->max();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        double val;
        if (values.split(";").count() > 1)
            val = values.split(";")[i].toDouble();
        else
            val = values.toDouble();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (checked)
        {

            for (auto j = 0; j < points.count(); j++)
            {
                if (points[j].y() == max)
                    ui_->chartView->chart()->axes()[1]->setMax(points[j].y() / val);
                points[j].setY(points[j].y() / val);
            }
        }
        else
        {
            for (auto j = 0; j < points.count(); j++)
            {
                if (points[j].y() == max)
                    ui_->chartView->chart()->axes()[1]->setMax(points[j].y() * val);
                points[j].setY(points[j].y() * val);
            }
        }
        xySeries->replace(points);
    }
}

void GraphWidget::modifyAgainstRun(HttpRequestWorker *worker, bool checked)
{
    QJsonArray inputArray;
    QJsonArray valueArray;
    valueArray = worker->json_array[1].toArray();
    valueArray = valueArray;
    auto max = qobject_cast<QValueAxis *>(ui_->chartView->chart()->axes()[1])->max();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        if (ui_->divideByMonitorRadio->isChecked())
            valueArray = valueArray[i].toArray();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (checked)
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = valueArray.at(i)[1].toDouble();
                if (val != 0)
                {
                    if (points[i].y() == max)
                        ui_->chartView->chart()->axes()[1]->setMax(points[i].y() / val);
                    points[i].setY(points[i].y() / val);
                }
            }
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = valueArray.at(i)[1].toDouble();
                if (val != 0)
                {
                    if (points[i].y() == max)
                        ui_->chartView->chart()->axes()[1]->setMax(points[i].y() * val);
                    points[i].setY(points[i].y() * val);
                }
            }
        }
        xySeries->replace(points);
    }
}

void GraphWidget::modifyAgainstMon(HttpRequestWorker *worker, bool checked)
{
    QJsonArray monArray;
    monArray = worker->json_array;
    monArray.removeFirst();
    auto max = qobject_cast<QValueAxis *>(ui_->chartView->chart()->axes()[1])->max();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        auto runArray = monArray[i].toArray();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (checked)
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                if (val != 0)
                {
                    if (points[i].y() == max)
                        ui_->chartView->chart()->axes()[1]->setMax(points[i].y() / val);
                    points[i].setY(points[i].y() / val);
                }
            }
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                if (val != 0)
                {
                    if (points[i].y() == max)
                        ui_->chartView->chart()->axes()[1]->setMax(points[i].y() * val);
                    points[i].setY(points[i].y() * val);
                }
            }
        }
        xySeries->replace(points);
    }
}