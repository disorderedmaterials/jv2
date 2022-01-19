// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "chartview.h"
#include <QApplication>
#include <QBrush>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDebug>
#include <QFont>
#include <QGraphicsSimpleTextItem>
#include <QValueAxis>
#include <QtGui/QMouseEvent>

ChartView::ChartView(QChart *chart, QWidget *parent) : QChartView(chart, parent)
{
    setRubberBand(QChartView::HorizontalRubberBand);
    setDragMode(QGraphicsView::NoDrag);
    this->setMouseTracking(true);
    hovered_ = false;
    coordLabelX_ = new QGraphicsSimpleTextItem(nullptr, chart);
    coordLabelY_ = new QGraphicsSimpleTextItem(nullptr, chart);
    coordStartLabelX_ = new QGraphicsSimpleTextItem(nullptr, chart);
    coordStartLabelY_ = new QGraphicsSimpleTextItem(nullptr, chart);
    coordLabelX_->setBrush(QColor(0, 0, 0, 127));
    coordLabelY_->setBrush(QColor(0, 0, 0, 127));
    coordLabelX_->setFont(QFont("Helvetica", 8));
    coordLabelY_->setFont(QFont("Helvetica", 8));
    coordStartLabelX_->setBrush(QColor(0, 0, 0, 127));
    coordStartLabelY_->setBrush(QColor(0, 0, 0, 127));
    coordStartLabelX_->setFont(QFont("Helvetica", 8));
    coordStartLabelY_->setFont(QFont("Helvetica", 8));
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    {
        switch (event->key())
        {
            case Qt::Key_Control:
                setRubberBand(QChartView::VerticalRubberBand);
                break;
            case Qt::Key_Left:
                chart()->scroll(-10, 0);
                break;
            case Qt::Key_Right:
                chart()->scroll(10, 0);
                break;
            case Qt::Key_Up:
                chart()->scroll(0, 10);
                break;
            case Qt::Key_Down:
                chart()->scroll(0, -10);
                break;
            default:
                QGraphicsView::keyPressEvent(event);
                break;
        }
    }
}

void ChartView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::wheelEvent(QWheelEvent *event)
{
    qreal factor;
    if (event->angleDelta().y() > 0)
        factor = 1.1;
    else
        factor = 0.91;

    auto graphArea = QRectF(chart()->plotArea().left(), chart()->plotArea().top(), chart()->plotArea().width() / factor,
                            chart()->plotArea().height() / factor);
    auto mousePos = mapFromGlobal(QCursor::pos());
    graphArea.moveCenter(mousePos);
    chart()->zoomIn(graphArea);
    auto delta = chart()->plotArea().center() - mousePos;
    chart()->scroll(delta.x(), -delta.y());
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        chart()->zoomIn();
        chart()->zoomOut();
        QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
        lastMousePos_ = event->pos();
        event->accept();
    }
    QChartView::mousePressEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        chart()->zoomReset();
        return;
    }
    if (event->button() == Qt::MiddleButton)
    {
        QApplication::restoreOverrideCursor();
    }
    if (event->button() == Qt::LeftButton)
    {
        coordLabelX_->setText("");
        coordLabelY_->setText("");
        coordStartLabelX_->setText("");
        coordStartLabelY_->setText("");
    }
    QChartView::mouseReleaseEvent(event);
}

void ChartView::setHovered(const QPointF point, bool hovered) { hovered_ = hovered; }

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    // pan the chart with a middle mouse drag
    if (event->buttons() & Qt::MiddleButton)
    {
        auto dPos = event->pos() - lastMousePos_;
        chart()->scroll(-dPos.x(), dPos.y());

        lastMousePos_ = event->pos();
        event->accept();
    }
    else if (event->buttons() & Qt::LeftButton)
    {
        auto x = (event->pos()).x();
        auto y = (event->pos()).y();

        auto xVal = chart()->mapToValue(event->pos()).x();
        auto yVal = chart()->mapToValue(event->pos()).y();

        qreal maxX;
        qreal minX;
        qreal maxY;
        qreal minY;
        if (chart()->axes(Qt::Horizontal)[0]->type() == QAbstractAxis::AxisTypeValue)
        {
            QValueAxis *axis = qobject_cast<QValueAxis *>(chart()->axes(Qt::Horizontal)[0]);
            maxX = axis->max();
            minX = axis->min();
        }
        else
        {
            QDateTimeAxis *axis = qobject_cast<QDateTimeAxis *>(chart()->axes(Qt::Horizontal)[0]);
            maxX = axis->max().toMSecsSinceEpoch();
            minX = axis->min().toMSecsSinceEpoch();
        }
        if (chart()->axes(Qt::Vertical)[0]->type() == QAbstractAxis::AxisTypeCategory)
        {
            maxY = yVal;
            minY = yVal;
        }
        else
        {
            maxY = qobject_cast<QValueAxis *>(chart()->axes(Qt::Vertical)[0])->max();
            minY = qobject_cast<QValueAxis *>(chart()->axes(Qt::Vertical)[0])->min();
        }

        if (xVal <= maxX && xVal >= minX && yVal <= maxY && yVal >= minY)
        {
            auto xPosOnAxis = chart()->mapToPosition(QPointF(x, minY));
            auto yPosOnAxis = chart()->mapToPosition(QPointF(minX, y));

            coordLabelX_->setPos(x, xPosOnAxis.y() + 5);
            coordLabelY_->setPos(yPosOnAxis.x() - 27, y);
            if (coordStartLabelX_->text() == "")
                coordStartLabelX_->setPos(x, xPosOnAxis.y() + 5);
            if (coordStartLabelY_->text() == "")
                coordStartLabelY_->setPos(yPosOnAxis.x() - 27, y);

            if (rubberBand() == QChartView::HorizontalRubberBand)
            {
                if (chart()->axes(Qt::Horizontal)[0]->type() == QAbstractAxis::AxisTypeValue)
                {
                    coordLabelX_->setText(QString::number(xVal));
                    if (coordStartLabelX_->text() == "")
                        coordStartLabelX_->setText(QString::number(xVal));
                }
                else
                {
                    coordLabelX_->setText(QDateTime::fromMSecsSinceEpoch(xVal).toString("yyyy-MM-dd HH:mm:ss"));
                    if (coordStartLabelX_->text() == "")
                        coordStartLabelX_->setText(QDateTime::fromMSecsSinceEpoch(xVal).toString("yyyy-MM-dd HH:mm:ss"));
                }
            }
            else
            {
                if (chart()->axes(Qt::Vertical)[0]->type() == QAbstractAxis::AxisTypeCategory)
                    coordLabelY_->setText("");
                else
                {
                    coordLabelY_->setText(QString::number(yVal));
                    if (coordStartLabelY_->text() == "")
                        coordStartLabelY_->setText(QString::number(yVal));
                }
            }
        }
    }
    else
    {
        if (hovered_)
            emit showCoordinates(chart()->mapToValue(event->pos()).x(), chart()->mapToValue(event->pos()).y());
        else
            emit clearCoordinates();
    }
    event->accept();

    QChartView::mouseMoveEvent(event);
}