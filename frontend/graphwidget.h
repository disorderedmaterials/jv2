// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>
#include <QChartView>
#include <QChart>
#include "chartview.h"

namespace Ui {
    class GraphWidget;
}

class GraphWidget : public QWidget
{
    Q_OBJECT

    public:
    GraphWidget(QWidget *parent = nullptr, QChart *chart = nullptr);
    ~GraphWidget();
    ChartView* getChartView();
    private:
    Ui::GraphWidget *ui_;
};

#endif
