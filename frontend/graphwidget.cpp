// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "graphwidget.h"
#include "./ui_graphwidget.h"
#include <QPainter>
#include <QTime>
#include <QTimer>

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    ui_->setupUi(this);
}

GraphWidget::~GraphWidget() {}