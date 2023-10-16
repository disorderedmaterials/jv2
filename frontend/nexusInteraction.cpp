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

// Configure and populate graphing window
void MainWindow::handle_result_contextGraph(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    auto *window = new QWidget;
    auto *dateTimeChart = new QChart();
    auto *dateTimeChartView = new ChartView(dateTimeChart, window);
    auto *relTimeChart = new QChart();
    auto *relTimeChartView = new ChartView(relTimeChart, window);
    auto *fieldsMenu = new QMenu("fieldsMenu", window);

    // Check network reply
    if (networkRequestHasError(worker, "trying to graph a log value"))
        return;

    foreach (const QJsonValue &log, worker->jsonArray[0].toArray())
    {
        auto logArray = log.toArray();
        auto name = logArray.first().toString().toUpper();
        name.chop(2);
        auto formattedName = name.append("og");
        auto *subMenu = new QMenu("Add data from " + formattedName);
        logArray.removeFirst();
        if (!logArray.empty())
            fieldsMenu->addMenu(subMenu);

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
        {
            // Fills contextMenu with all columns
            QString path = block.toString();
            auto *action = new QAction(path.right(path.size() - path.lastIndexOf("/") - 1), this);
            action->setData(path);
            connect(action, SIGNAL(triggered()), this, SLOT(getField()));
            subMenu->addAction(action);
        }
    }

    worker->jsonArray.removeFirst();
    auto *timeAxis = new QDateTimeAxis();
    timeAxis->setFormat("yyyy-MM-dd<br>H:mm:ss");
    dateTimeChart->addAxis(timeAxis, Qt::AlignBottom);

    auto *dateTimeYAxis = new QValueAxis();
    dateTimeYAxis->setRange(0, 0);

    auto *dateTimeStringAxis = new QCategoryAxis();
    QStringList categoryValues;

    auto *relTimeXAxis = new QValueAxis();
    relTimeXAxis->setTitleText("Relative Time (s)");
    relTimeChart->addAxis(relTimeXAxis, Qt::AlignBottom);

    auto *relTimeYAxis = new QValueAxis();

    auto *relTimeStringAxis = new QCategoryAxis();

    QList<QString> chartFields;
    bool firstRun = true;
    // For each Run
    foreach (const auto &runFields, worker->jsonArray)
    {
        auto runFieldsArray = runFields.toArray();

        auto startTime = QDateTime::fromString(runFieldsArray.first()[0].toString(), "yyyy-MM-dd'T'HH:mm:ss");
        auto endTime = QDateTime::fromString(runFieldsArray.first()[1].toString(), "yyyy-MM-dd'T'HH:mm:ss");
        runFieldsArray.removeFirst();

        if (firstRun)
        {
            timeAxis->setRange(startTime, endTime);
            relTimeXAxis->setRange(0, 0);
        }

        foreach (const auto &fieldData, runFieldsArray)
        {
            auto fieldDataArray = fieldData.toArray();
            fieldDataArray.removeFirst();
            if (!fieldDataArray.first()[1].isString())
                break;
            foreach (const auto &dataPair, fieldDataArray)
            {
                auto dataPairArray = dataPair.toArray();
                categoryValues.append(dataPairArray[1].toString());
            }
        }

        if (!categoryValues.isEmpty())
        {
            categoryValues.removeDuplicates();
            categoryValues.sort();
        }
        if (firstRun)
        {
            if (!categoryValues.isEmpty())
            {
                dateTimeChart->addAxis(dateTimeStringAxis, Qt::AlignLeft);
                relTimeChart->addAxis(relTimeStringAxis, Qt::AlignLeft);
            }
            else
            {
                dateTimeChart->addAxis(dateTimeYAxis, Qt::AlignLeft);
                relTimeChart->addAxis(relTimeYAxis, Qt::AlignLeft);
            }
            firstRun = false;
        }
        // For each field
        foreach (const auto &fieldData, runFieldsArray)
        {
            auto fieldDataArray = fieldData.toArray();

            // For each plot point
            auto *dateSeries = new QLineSeries();
            auto *relSeries = new QLineSeries();

            connect(dateSeries, &QLineSeries::hovered,
                    [=](const QPointF point, bool hovered)
                    { dateTimeChartView->setHovered(point, hovered, dateSeries->name()); });
            connect(dateTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                    SLOT(showStatus(qreal, qreal, QString)));
            connect(dateTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));
            connect(relSeries, &QLineSeries::hovered,
                    [=](const QPointF point, bool hovered)
                    { relTimeChartView->setHovered(point, hovered, relSeries->name()); });
            connect(relTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                    SLOT(showStatus(qreal, qreal, QString)));
            connect(relTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

            // Set dateSeries ID
            QString name = fieldDataArray.first()[0].toString();
            QString field = fieldDataArray.first()[1].toString().section(':', -1);
            if (!chartFields.contains(field))
                chartFields.append(field);
            dateSeries->setName(name);
            relSeries->setName(name);
            fieldDataArray.removeFirst();

            if (fieldDataArray.first()[1].isString())
            {
                foreach (const auto &dataPair, fieldDataArray)
                {
                    auto dataPairArray = dataPair.toArray();
                    dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                       categoryValues.indexOf(dataPairArray[1].toString()));
                    relSeries->append(dataPairArray[0].toDouble(), categoryValues.indexOf(dataPairArray[1].toString()));
                }
            }
            else
            {
                foreach (const auto &dataPair, fieldDataArray)
                {
                    auto dataPairArray = dataPair.toArray();
                    dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                       dataPairArray[1].toDouble());
                    relSeries->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
                    if (dateTimeYAxis->min() == 0 && dateTimeYAxis->max() == 0)
                        dateTimeYAxis->setRange(dataPairArray[1].toDouble(), dataPairArray[1].toDouble());
                    if (dataPairArray[1].toDouble() < dateTimeYAxis->min())
                        dateTimeYAxis->setMin(dataPairArray[1].toDouble());
                    if (dataPairArray[1].toDouble() > dateTimeYAxis->max())
                        dateTimeYAxis->setMax(dataPairArray[1].toDouble());
                }
            }
            if (startTime.addSecs(startTime.secsTo(QDateTime::fromMSecsSinceEpoch(dateSeries->at(0).x()))) < timeAxis->min())
                timeAxis->setMin(startTime.addSecs(startTime.secsTo(QDateTime::fromMSecsSinceEpoch(dateSeries->at(0).x()))));
            if (endTime > timeAxis->max())
                timeAxis->setMax(endTime);

            if (relSeries->at(0).x() < relTimeXAxis->min())
                relTimeXAxis->setMin(relSeries->at(0).x());
            if (relSeries->at(relSeries->count() - 1).x() > relTimeXAxis->max())
                relTimeXAxis->setMax(relSeries->at(relSeries->count() - 1).x());

            dateTimeChart->addSeries(dateSeries);
            dateSeries->attachAxis(timeAxis);
            relTimeChart->addSeries(relSeries);
            relSeries->attachAxis(relTimeXAxis);
            if (categoryValues.isEmpty())
            {
                dateSeries->attachAxis(dateTimeYAxis);
                relSeries->attachAxis(relTimeYAxis);
            }
            else
            {
                dateSeries->attachAxis(dateTimeStringAxis);
                relSeries->attachAxis(relTimeStringAxis);
            }
        }
    }

    if (!categoryValues.isEmpty())
    {
        dateTimeStringAxis->setRange(0, categoryValues.count() - 1);
        relTimeStringAxis->setRange(0, categoryValues.count() - 1);
        for (auto i = 0; i < categoryValues.count(); i++)
        {
            dateTimeStringAxis->append(categoryValues[i], i);
            relTimeStringAxis->append(categoryValues[i], i);
        }
        dateTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        relTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    }

    relTimeYAxis->setRange(dateTimeYAxis->min(), dateTimeYAxis->max());

    auto *gridLayout = new QGridLayout(window);
    auto *axisToggleCheck = new QCheckBox("Plot relative to run start times", window);
    auto *addFieldButton = new QPushButton("Add field", window);

    addFieldButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(axisToggleCheck, SIGNAL(stateChanged(int)), this, SLOT(toggleAxis(int)));
    connect(addFieldButton, &QPushButton::clicked,
            [=]() { fieldsMenu->exec(addFieldButton->mapToGlobal(QPoint(0, addFieldButton->height()))); });

    gridLayout->addWidget(dateTimeChartView, 1, 0, -1, -1);
    gridLayout->addWidget(relTimeChartView, 1, 0, -1, -1);
    relTimeChartView->hide();
    gridLayout->addWidget(axisToggleCheck, 0, 0);
    gridLayout->addWidget(addFieldButton, 0, 1);
    QString tabName;
    for (auto i = 0; i < chartFields.size(); i++)
    {
        tabName += chartFields[i];
        if (i < chartFields.size() - 1)
            tabName += ",";
    }
    if (!categoryValues.isEmpty())
    {
        dateTimeStringAxis->setTitleText(tabName);
        relTimeStringAxis->setTitleText(tabName);
    }
    else
    {
        dateTimeYAxis->setTitleText(tabName);
        relTimeYAxis->setTitleText(tabName);
    }
    ui_.MainTabs->addTab(window, tabName);
    QString runs;
    for (auto series : dateTimeChart->series())
        runs.append(series->name() + ", ");
    runs.chop(2);
    QString toolTip = currentInstrument().name() + "\n" + tabName + "\n" + runs;
    ui_.MainTabs->setTabToolTip(ui_.MainTabs->count() - 1, toolTip);
    ui_.MainTabs->setCurrentIndex(ui_.MainTabs->count() - 1);
    dateTimeChartView->setFocus();
}

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

    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();

    if (cycles == "")
    {
        for (auto run : runNos.split(";"))
            cycles.append(" ;");
        cycles.chop(1);
    }

    backend_.getNexusLogValueData(currentJournal().location(), selectedRunNumbers(), action->data().toString(),
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

    auto workerArray = worker->jsonArray;
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

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusDetectorAnalysis(currentInstrument().dataDirectory(), cycle, runs,
                                      [=](HttpRequestWorker *worker) { window->setLabel(worker->response); });
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

    auto workerArray = worker->jsonArray;
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
    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();

    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusSpectrumRange(currentInstrument().dataDirectory(), cycle, runNos,
                                   [=](HttpRequestWorker *worker) { plotSpectra(worker); });

    setLoadScreen(true);
}

void MainWindow::getMonitorCount()
{
    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();

    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusMonitorRange(currentInstrument().dataDirectory(), cycle, runNos,
                                  [=](HttpRequestWorker *worker) { plotMonSpectra(worker); });

    setLoadScreen(true);
}

void MainWindow::plotSpectra(HttpRequestWorker *count)
{
    setLoadScreen(false);
    auto spectraCount = count->response.toUtf8();
    bool valid;
    auto spectrumNumber = QInputDialog::getInt(this, tr("Plot Detector Spectrum"),
                                               tr("Enter detector spectrum to plot (0-" + spectraCount + "):"), 0, 0,
                                               count->response.toInt() - 1, 1, &valid);
    if (!valid)
        return;
    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();
    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusDetector(currentInstrument().dataDirectory(), cycle, runNos, QString::number(spectrumNumber),
                              [=](HttpRequestWorker *worker) { handleSpectraCharting(worker); });
}

void MainWindow::plotMonSpectra(HttpRequestWorker *count)
{
    setLoadScreen(false);
    auto monCount = count->response.toUtf8();
    bool valid;
    auto monNumber =
        QInputDialog::getInt(this, tr("Plot Monitor Spectrum"), tr("Enter monitor spectrum to plot (0-" + monCount + "):"), 0,
                             0, count->response.toInt() - 1, 1, &valid);
    if (!valid)
        return;
    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();
    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusMonitor(currentInstrument().dataDirectory(), cycle, runNos, QString::number(monNumber),
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

    backend_.getRunTotalMuAmps(currentInstrument().dataDirectory(), currentJournal_->get().location().filename(), runs,
                               [=](HttpRequestWorker *worker) { window->modifyAgainstString(worker->response, checked); });
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

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusDetector(currentInstrument().dataDirectory(), cycle, run, currentDetector,
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

    QString cycle = currentJournal_->get().location().filename();
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    backend_.getNexusMonitor(currentInstrument().dataDirectory(), cycle, currentRun, mon,
                             [=](HttpRequestWorker *worker) { window->modifyAgainstWorker(worker, checked); });
}
