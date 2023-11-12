// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "chartView.h"
#include "graphWidget.h"
#include "mainWindow.h"
#include <QAction>
#include <QCategoryAxis>
#include <QChartView>
#include <QDateTimeAxis>
#include <QInputDialog>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QValueAxis>
#include <algorithm>

void MainWindow::toggleAxis(int state)
{
    auto *toggleBox = qobject_cast<QCheckBox *>(sender());
    auto *graphParent = toggleBox->parentWidget();
    auto tabCharts = graphParent->findChildren<QChartView *>();
    if (toggleBox->isChecked())
    {
        tabCharts[0]->hide();
        tabCharts[1]->show();
        tabCharts[1]->setFocus();
    }
    else
    {
        tabCharts[0]->show();
        tabCharts[0]->setFocus();
        tabCharts[1]->hide();
    }
}

void MainWindow::getField()
{
    auto *action = qobject_cast<QAction *>(sender());
    auto *graphParent = ui_.MainTabs->currentWidget();
    auto tabCharts = graphParent->findChildren<QChartView *>();

    backend_.getNexusLogValueData(currentJournalSource(), selectedRunNumbers(), action->data().toString(),
                                  [=](HttpRequestWorker *worker)
                                  {
                                      dynamic_cast<ChartView *>(tabCharts[0])->addSeries(worker);
                                      dynamic_cast<ChartView *>(tabCharts[1])->addSeries(worker);
                                  });
}

void MainWindow::showStatus(qreal x, qreal y, QString title)
{
    QString message;
    auto *chartView = qobject_cast<ChartView *>(sender());
    auto yVal = QString::number(y);
    if (QString(chartView->chart()->axes(Qt::Vertical)[0]->metaObject()->className()) == QString("QCategoryAxis"))
    {
        auto *yAxis = qobject_cast<QCategoryAxis *>(chartView->chart()->axes(Qt::Vertical)[0]);
        yVal = yAxis->categoriesLabels()[(int)y];
    }
    if (QString(chartView->chart()->axes(Qt::Horizontal)[0]->metaObject()->className()) == QString("QDateTimeAxis"))
        message = QDateTime::fromMSecsSinceEpoch(x).toString("yyyy-MM-dd HH:mm:ss") + ", " + yVal;
    else
        message = QString::number(x) + ", " + yVal;
    statusBar()->showMessage("Run " + title + ": " + message);
}

void MainWindow::handleSpectraCharting(HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "trying to plot a spectrum"))
        return;

    auto *chart = new QChart();
    auto *window = new GraphWidget(this, chart, "Detector");
    connect(window, SIGNAL(muAmps(QString, bool, QString)), this, SLOT(muAmps(QString, bool, QString)));
    connect(window, SIGNAL(runDivide(QString, QString, bool)), this, SLOT(runDivide(QString, QString, bool)));
    connect(window, SIGNAL(monDivide(QString, QString, bool)), this, SLOT(monDivide(QString, QString, bool)));
    ChartView *chartView = window->getChartView();

    auto workerArray = worker->jsonResponse().array();
    QString field = "Detector ";
    auto metaData = workerArray[0].toArray();
    QString runs = metaData[0].toString();
    window->setChartRuns(metaData[0].toString());
    window->setChartDetector(metaData[1].toString());
    field += metaData[1].toString();
    workerArray.removeFirst();
    window->setChartData(workerArray);

    foreach (const auto &run, workerArray)
    {
        auto runArray = run.toArray();
        // For each plot point
        auto *series = new QLineSeries();

        connect(series, &QLineSeries::hovered,
                [=](const QPointF point, bool hovered) { chartView->setHovered(point, hovered, series->name()); });
        connect(chartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this, SLOT(showStatus(qreal, qreal, QString)));
        connect(chartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

        for (auto i = 0; i < runArray.count() - 1; i++)
        {
            auto centreBin =
                runArray.at(i)[0].toDouble() + (runArray.at(i + 1)[0].toDouble() - runArray.at(i)[0].toDouble()) / 2;
            series->append(centreBin, runArray.at(i)[1].toDouble());
        }
        chart->addSeries(series);
    }
    for (auto i = 0; i < chart->series().count(); i++)
    {
        auto *series = chart->series()[i];
        series->setName(runs.split(";")[i]);
    }
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal)[0]->setTitleText("Time of flight, &#181;s");
    chart->axes(Qt::Vertical)[0]->setTitleText("Counts");
    QString tabName = field;
    ui_.MainTabs->addTab(window, tabName);
    ui_.MainTabs->setCurrentIndex(ui_.MainTabs->count() - 1);
    QString toolTip = field + "\n" + runs;
    ui_.MainTabs->setTabToolTip(ui_.MainTabs->count() - 1, toolTip);
    chartView->setFocus();

    QString cycle = currentJournal().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");
}

void MainWindow::handleMonSpectraCharting(HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "trying to plot a monitor spectrum"))
        return;

    auto *chart = new QChart();
    auto *window = new GraphWidget(this, chart, "Monitor");
    connect(window, SIGNAL(muAmps(QString, bool, QString)), this, SLOT(muAmps(QString, bool, QString)));
    connect(window, SIGNAL(runDivide(QString, QString, bool)), this, SLOT(runDivide(QString, QString, bool)));
    connect(window, SIGNAL(monDivide(QString, QString, bool)), this, SLOT(monDivide(QString, QString, bool)));
    ChartView *chartView = window->getChartView();

    auto workerArray = worker->jsonResponse().array();
    QString field = "Monitor ";
    auto metaData = workerArray[0].toArray();
    QString runs = metaData[0].toString();
    window->setChartRuns(metaData[0].toString());
    window->setChartDetector(metaData[1].toString());
    field += metaData[1].toString();
    workerArray.removeFirst();
    window->setChartData(workerArray);

    foreach (const auto &run, workerArray)
    {
        auto runArray = run.toArray();
        // For each plot point
        auto *series = new QLineSeries();

        connect(series, &QLineSeries::hovered,
                [=](const QPointF point, bool hovered) { chartView->setHovered(point, hovered, series->name()); });
        connect(chartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this, SLOT(showStatus(qreal, qreal, QString)));
        connect(chartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

        for (auto i = 0; i < runArray.count() - 1; i++)
        {
            auto centreBin =
                runArray.at(i)[0].toDouble() + (runArray.at(i + 1)[0].toDouble() - runArray.at(i)[0].toDouble()) / 2;
            series->append(centreBin, runArray.at(i)[1].toDouble());
        }
        chart->addSeries(series);
    }
    for (auto i = 0; i < chart->series().count(); i++)
    {
        auto *series = chart->series()[i];
        series->setName(runs.split(";")[i]);
    }
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal)[0]->setTitleText("Time of flight, &#181;s");
    chart->axes(Qt::Vertical)[0]->setTitleText("Counts");
    QString tabName = field;
    ui_.MainTabs->addTab(window, tabName);
    ui_.MainTabs->setCurrentIndex(ui_.MainTabs->count() - 1);
    QString toolTip = field + "\n" + runs;
    ui_.MainTabs->setTabToolTip(ui_.MainTabs->count() - 1, toolTip);
    chartView->setFocus();
}

void MainWindow::getSpectrumCount()
{
    backend_.getNexusSpectrumRange(currentJournalSource(), selectedRunNumbers().front(),
                                   [=](HttpRequestWorker *worker) { plotSpectra(worker); });
}

void MainWindow::getMonitorCount()
{
    backend_.getNexusMonitorRange(currentJournalSource(), selectedRunNumbers().front(),
                                  [=](HttpRequestWorker *worker) { plotMonSpectra(worker); });
}

void MainWindow::plotSpectra(HttpRequestWorker *count)
{
    auto spectraCount = count->response().toUtf8();
    bool valid;
    auto spectrumNumber = QInputDialog::getInt(this, tr("Plot Detector Spectrum"),
                                               tr("Enter detector spectrum to plot (0-" + spectraCount + "):"), 0, 0,
                                               count->response().toInt() - 1, 1, &valid);
    if (!valid)
        return;

    backend_.getNexusDetector(currentJournalSource(), selectedRunNumbers(), spectrumNumber,
                              [=](HttpRequestWorker *worker) { handleSpectraCharting(worker); });
}

void MainWindow::plotMonSpectra(HttpRequestWorker *count)
{
    auto monCount = count->response().toUtf8();
    bool valid;
    auto monNumber =
        QInputDialog::getInt(this, tr("Plot Monitor Spectrum"), tr("Enter monitor spectrum to plot (0-" + monCount + "):"), 0,
                             0, count->response().toInt() - 1, 1, &valid);
    if (!valid)
        return;

    backend_.getNexusMonitor(currentJournalSource(), selectedRunNumbers(), monNumber,
                             [=](HttpRequestWorker *worker) { handleMonSpectraCharting(worker); });
}

void MainWindow::muAmps(QString runs, bool checked, QString modified)
{
    auto *window = qobject_cast<GraphWidget *>(sender());
    QString modifier = "/muAmps";
    auto yAxisTitle = window->getChartView()->chart()->axes(Qt::Vertical)[0]->titleText();

    // if (modified != "-1")
    // url_str += ";" + modified;

    if (checked)
        yAxisTitle.append(modifier);
    else
        yAxisTitle.remove(modifier);
    window->getChartView()->chart()->axes(Qt::Vertical)[0]->setTitleText(yAxisTitle);

    // For each run number extract the proton_charge data from the run data
    QString result;
    QStringList muAmps;
    auto runNumbers = runs.split(";");
    foreach (auto run, runNumbers)
    {
        auto runData = dataForRunNumber(run.toInt());
        muAmps.append(runData ? (*runData)["proton_charge"].toString() : "1.0");
    }

    window->modifyAgainstString(muAmps.join(";"), checked);
}

void MainWindow::runDivide(QString currentDetector, QString run, bool checked)
{
    auto *window = qobject_cast<GraphWidget *>(sender());
    QString modifier = "/run " + run;

    auto yAxisTitle = window->getChartView()->chart()->axes(Qt::Vertical)[0]->titleText();
    if (checked)
        yAxisTitle.append(modifier);
    else
        yAxisTitle.remove(modifier);
    window->getChartView()->chart()->axes(Qt::Vertical)[0]->setTitleText(yAxisTitle);

    QString cycle = currentJournal().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusDetector(currentJournalSource(), {run.toInt()}, currentDetector.toInt(),
                              [=](HttpRequestWorker *worker) { window->modifyAgainstWorker(worker, checked); });
}

void MainWindow::monDivide(QString currentRun, QString mon, bool checked)
{
    auto *window = qobject_cast<GraphWidget *>(sender());
    QString modifier = "/mon " + mon;

    auto yAxisTitle = window->getChartView()->chart()->axes(Qt::Vertical)[0]->titleText();
    if (checked)
        yAxisTitle.append(modifier);
    else
        yAxisTitle.remove(modifier);
    window->getChartView()->chart()->axes(Qt::Vertical)[0]->setTitleText(yAxisTitle);

    QString cycle = currentJournal().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusMonitor(currentJournalSource(), {currentRun.toInt()}, mon.toInt(),
                             [=](HttpRequestWorker *worker) { window->modifyAgainstWorker(worker, checked); });
}
