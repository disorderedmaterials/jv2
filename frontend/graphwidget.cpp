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
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
    ui_->binWidths->setText("Counts/ µs");
}

GraphWidget::~GraphWidget() {}

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::on_binWidths_clicked(bool checked)
{
    checked ? ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ &#181;s")
            : ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    for (auto *series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
        {

            for (auto i = 0; i < points.count() - 1; i++)
            {
                auto binWidth = points[i + 1].x() - points[i].x();
                points[i].setY(points[i].y() / binWidth);
            }
        }
        else
        {
            for (auto i = 0; i < points.count() - 1; i++)
            {
                auto binWidth = points[i + 1].x() - points[i].x();
                points[i].setY(points[i].y() * binWidth);
            }
        }
        xySeries->append(points);
    }
}

void GraphWidget::on_muAmps_clicked(bool checked)
{
    checked ? ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ Total &#181;Amps")
            : ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit test(checked);
}

void GraphWidget::on_runDivide_clicked(bool checked)
{
    if (checked)
    {
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ run x value");
        run_ = QInputDialog::getText(this, tr("Run"), tr("Run No: "), QLineEdit::Normal);
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit runDivide(run_, checked);
}

void GraphWidget::on_monDivide_clicked(bool checked)
{
    if (checked)
    {
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ mon x value");
        run_ = QInputDialog::getText(this, tr("Mon"), tr("Mon No: "), QLineEdit::Normal);
    }
    else
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    emit monDivide(run_, checked);
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

            for (auto i = 0; i < points.count() - 1; i++)
                points[i].setY(points[i].y() / val);
        }
        else
        {
            for (auto i = 0; i < points.count() - 1; i++)
                points[i].setY(points[i].y() * val);
        }
        xySeries->append(points);
    }
}

void GraphWidget::modifyAgainstRun(HttpRequestWorker *worker, bool checked)
{
    auto runArray = worker->json_array[1].toArray();
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
                else
                    points[i].setY(1);
            }
        }
        else
        {
            for (auto i = 0; i < points.count(); i++)
            {
                auto val = runArray.at(i)[1].toDouble();
                    points[i].setY(points[i].y() * val);
            }
        }
        qDebug() << "Points: " << points.count();
        xySeries->append(points);
    }
}