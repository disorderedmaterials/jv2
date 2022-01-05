// SPDX-License-Identifier: GPL-3.0-or-later
<<<<<<< HEAD
<<<<<<< HEAD
// Copyright (c) 2022 E. Devlin and T. Youngs
=======
// Copyright (c) 2021 E. Devlin and T. Youngs
>>>>>>> d771ac8... first commit
=======
// Copyright (c) 2022 E. Devlin and T. Youngs
>>>>>>> 6afd308... format fixes

#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include <QtCharts/QChartView>
#include <QtWidgets/QRubberBand>

class ChartView : public QChartView
{
    public:
<<<<<<< HEAD
<<<<<<< HEAD
    ChartView(QChart *chart, QWidget *parent = 0);

    protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
=======
        ChartView(QChart *chart, QWidget *parent = 0);
    
    protected:
        void keyPressEvent(QKeyEvent *event);
        void keyReleaseEvent(QKeyEvent *event);
        void wheelEvent(QWheelEvent *event);
        void mousePressEvent(QMouseEvent *event);
>>>>>>> d771ac8... first commit
=======
    ChartView(QChart *chart, QWidget *parent = 0);

    protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
>>>>>>> 6afd308... format fixes
};

#endif // CHARTVIEW_H
