// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
#include <QtCharts/QChartView>
#include <QtWidgets/QRubberBand>

class ChartView : public QChartView
{
    Q_OBJECT

    public:
    ChartView(QChart *chart, QWidget *parent = nullptr);
    ChartView(QWidget *parent = nullptr);
    void assignChart(QChart *chart);

    public slots:
    void setHovered(const QPointF point, bool hovered, QString title);
    void addSeries(HttpRequestWorker *worker);

    signals:
    void showCoordinates(qreal x, qreal y, QString title);
    void clearCoordinates();

    protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    private slots:
    void setGraphics(QChart *chart);

    private:
    QPointF lastMousePos_;
    QString hovered_;
    QGraphicsSimpleTextItem *coordLabelX_;
    QGraphicsSimpleTextItem *coordLabelY_;
    QGraphicsSimpleTextItem *coordStartLabelX_;
    QGraphicsSimpleTextItem *coordStartLabelY_;
};
