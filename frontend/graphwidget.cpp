// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "graphwidget.h"
#include "./ui_graphwidget.h"
#include "chartview.h"
#include <QChart>
#include <QChartView>
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
}

GraphWidget::~GraphWidget() {}

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::on_binWidths_clicked(bool checked)
{
    for (auto* series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries*>(series);
        auto points = xySeries->points();
        xySeries->clear();
        for (auto point : points)
        {
            point.setY(point.y()+1);
            xySeries->append(point);
        }
    }
}