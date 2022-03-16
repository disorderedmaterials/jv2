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

void GraphWidget::on_binWidths_toggled(bool checked)
{
    if (checked)
    {
        toggleOptions("binWidths");
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ &#181;s");
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
            for (auto j = 0; j < points.count(); j++)
                points[j].setY(points[j].y() / binWidths_[i][j]);
        else
            for (auto j = 0; j < points.count(); j++)
                points[j].setY(points[j].y() * binWidths_[i][j]);
        xySeries->append(points);
    }
}

void GraphWidget::toggleOptions(QString option)
{
    if (ui_->binWidths->isChecked() && option != "binWidths")
        ui_->binWidths->toggle();
    if (ui_->muAmps->isChecked() && option != "muAmps")
        ui_->muAmps->toggle();
    if (ui_->runDivide->isChecked() && option != "runDivide")
        ui_->runDivide->toggle();
    if (ui_->monDivide->isChecked() && option != "monDivide")
        ui_->monDivide->toggle();
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
    emit muAmps(chartRuns_, checked);
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
    emit runDivide(chartDetector_, run_, checked);
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

void GraphWidget::modify(QString values, bool checked)
{
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        double val;
        if (values.split(";").count() > 1)
            val = values.split(";")[i].toDouble();
        else
            val = values.toDouble();
        qDebug() << val;
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
        {

            for (auto j = 0; j < points.count(); j++)
                points[j].setY(points[j].y() / val);
        }
        else
        {
            for (auto j = 0; j < points.count(); j++)
                points[j].setY(points[j].y() * val);
        }
        xySeries->append(points);
    }
}

void GraphWidget::modifyAgainstRun(HttpRequestWorker *worker, bool checked)
{
    QJsonArray runArray;
    runArray = worker->json_array[1].toArray();
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

void GraphWidget::modifyAgainstMon(HttpRequestWorker *worker, bool checked)
{
    QJsonArray monArray;
    monArray = worker->json_array;
    monArray.removeFirst();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        auto runArray = monArray[i].toArray();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        qDebug() << "Points: " << points.count() << " found: " << runArray.count();
        xySeries->clear();
        if (checked)
        {
            qDebug() << "Divide time?";
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                if (val != 0)
                {
                    qDebug() << "Doing a divide";
                    qDebug() << points[i].y() << " " << val;
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
                    points[i].setY(points[i].y() * val);
            }
        }
        xySeries->append(points);
    }
}