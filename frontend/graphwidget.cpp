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
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
    ui_->binWidths->setText("Counts/ µs");
}

GraphWidget::~GraphWidget() {}

QString GraphWidget::getChartRuns() { return chartRuns_; }
QString GraphWidget::getChartDetector() { return chartDetector_; }
QJsonArray GraphWidget::getChartData() { return chartData_; }

void GraphWidget::setChartRuns(QString chartRuns) { chartRuns_ = chartRuns; }
void GraphWidget::setChartDetector(QString chartDetector) { chartDetector_ = chartDetector; }
void GraphWidget::setChartData(QJsonArray chartData) { chartData_ = chartData; }

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::on_binWidths_toggled(bool checked)
{
    if (checked)
    {
        toggleOptions("binWidths");
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ &#181;s");
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    for (auto *series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
        {

            for (auto i = 0; i < points.count(); i++)
            {
                const auto &dataPairTOFStart = chartData_.at(i);
                const auto &dataPairTOFEnd = chartData_.at(i + 1);
                auto binWidth = dataPairTOFEnd[0].toDouble() - dataPairTOFStart[0].toDouble();
                points[i].setY(points[i].y() / binWidth);
            }
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
            {
                const auto &dataPairTOFStart = chartData_.at(i);
                const auto &dataPairTOFEnd = chartData_.at(i + 1);
                auto binWidth = dataPairTOFEnd[0].toDouble() - dataPairTOFStart[0].toDouble();
                points[i].setY(points[i].y() * binWidth);
            }
        }
        xySeries->append(points);
    }
}

void GraphWidget::toggleOptions(QString option)
{
    if (ui_->binWidths->isChecked() && option != "binWidths")  ui_->binWidths->toggle();
    if (ui_->muAmps->isChecked() && option != "muAmps")  ui_->muAmps->toggle();
    if (ui_->runDivide->isChecked() && option != "runDivide")  ui_->runDivide->toggle();
    if (ui_->monDivide->isChecked() && option != "monDivide")  ui_->monDivide->toggle();
}

void GraphWidget::on_muAmps_toggled(bool checked)
{
    if (checked)
    {
        toggleOptions("muAmps");
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ Total &#181;Amps");
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit test(checked);
}

void GraphWidget::on_runDivide_toggled(bool checked)
{
    if (checked)
    {
        toggleOptions("runDivide");
        run_ = QInputDialog::getText(this, tr("Run"), tr("Run No: "), QLineEdit::Normal);
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ run " + run_ + " value");
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit runDivide(run_, checked);
}

void GraphWidget::on_monDivide_toggled(bool checked)
{
    if (checked)
    {
        toggleOptions("monDivide");
        run_ = QInputDialog::getText(this, tr("Mon"), tr("Mon No: "), QLineEdit::Normal);
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ monitor " + run_ + " value");
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit monDivide(chartRuns_, run_, checked);
}

void GraphWidget::modify(double val, bool checked)
{
    for (auto *series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
        {

            for (auto i = 0; i < points.count(); i++)
                points[i].setY(points[i].y() / val);
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
                points[i].setY(points[i].y() * val);
        }
        xySeries->append(points);
    }
}

void GraphWidget::modifyAgainstRun(HttpRequestWorker *worker, bool checked)
{
    QJsonArray runArray;
    if (worker->json_array.count() == 1)
        runArray = worker->json_array[0].toArray();
    else
        runArray = worker->json_array[1].toArray();
    runArray.removeFirst();
    for (auto *series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        auto points = xySeries->points();
        qDebug() << "Points: " << points.count() << " found: " << runArray.count();
        xySeries->clear();
        if (checked)
        {

            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                if (val != 0)
                    points[i].setY(points[i].y() / val);
            }
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                if (val != 0)
                    points[i].setY(points[i].y() * val);
            }
        }
        xySeries->append(points);
    }
}