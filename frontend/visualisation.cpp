// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "chartView.h"
#include "mainWindow.h"
#include "seLogChooserDialog.h"
#include <QCategoryAxis>
#include <QChart>
#include <QDateTimeAxis>
#include <QLineSeries>
#include <QMessageBox>
#include <QValueAxis>

// Handle extracted SE log values for plotting
void MainWindow::handlePlotSELogValue(HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "retrieving log values from run"))
        return;

    // Iterate over logs extracted from the target run data and construct our mapped values
    auto *rootItem = new GenericTreeItem({"Log Value", "Full Path"});
    foreach (const auto &log, worker->jsonArray)
    {
        auto logArray = log.toArray();
        if (logArray.size() < 2)
            continue;

        // First item in the array is the name of the log value set / section
        auto *sectionItem = rootItem->appendChild({logArray.first().toString(), ""});

        // Remove the name item and proceed to iterate over log values
        logArray.removeFirst();

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
            sectionItem->appendChild({block.toString().split("/").last(), block.toString()});
    }

    // Create the dialog
    SELogChooserDialog chooserDialog(this, rootItem);

    auto logValue = chooserDialog.getValue();
    qDebug() << logValue;
    if (logValue.isEmpty())
        return;

    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();
    if (runNos.size() == 0)
        return;

    // Request the log value data
    backend_.getNexusLogValueData(currentJournal().location(), selectedRunNumbers(), logValue,
                                  [=](HttpRequestWorker *worker) { handleCreateSELogPlot(worker); });

    setLoadScreen(true);
}

// Handle plotting of SE log data
void MainWindow::handleCreateSELogPlot(HttpRequestWorker *worker)
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

    // The expected result from the backend is as follows:
    //
    // result = {
    //              logValue: "name_of_log_value",
    //              runNumbers: { run1, run2, run3 ... runN }
    //              data: {
    //                  run1: {
    //                      timeRange: [ datetime, datetime ],
    //                      data: [ (x,y), (x2,y2), ..., (xn,yn) ]
    //                  },
    //                  ...
    //                  runN: {
    //                      ...
    //                  }
    //              }

    const auto receivedData = worker->jsonResponse.object();
    auto logValueName = receivedData["logValue"].toString().section('/', -1);
    qDebug() << logValueName;

    const auto logValueData = receivedData["data"].toObject();

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
    foreach (const auto &run, logValueData)
    {
        // Get the data name (run number)
        const auto dataName = run[QString("runNumber")].toString();
        qDebug() << run[QString("runNumber")].toString();

        // Extract the time range data
        const auto timeRange = run[QString("timeRange")].toArray();

        auto startTime = QDateTime::fromString(timeRange.first()[0].toString(), "yyyy-MM-dd'T'HH:mm:ss");
        auto endTime = QDateTime::fromString(timeRange.first()[1].toString(), "yyyy-MM-dd'T'HH:mm:ss");

        if (firstRun)
        {
            timeAxis->setRange(startTime, endTime);
            relTimeXAxis->setRange(0, 0);
        }

        const auto fieldDataArray = run[QString("data")].toArray();

        // foreach (const auto &fieldData, runFieldsArray)
        // {
        //     auto fieldDataArray = fieldData.toArray();
        //     fieldDataArray.removeFirst();
        //     if (!fieldDataArray.first()[1].isString())
        //         break;
        //     foreach (const auto &dataPair, fieldDataArray)
        //     {
        //         auto dataPairArray = dataPair.toArray();
        //         categoryValues.append(dataPairArray[1].toString());
        //     }
        // }

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

        // For each plot point
        auto *dateSeries = new QLineSeries();
        auto *relSeries = new QLineSeries();

        connect(dateSeries, &QLineSeries::hovered,
                [=](const QPointF point, bool hovered) { dateTimeChartView->setHovered(point, hovered, dateSeries->name()); });
        connect(dateTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                SLOT(showStatus(qreal, qreal, QString)));
        connect(dateTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));
        connect(relSeries, &QLineSeries::hovered,
                [=](const QPointF point, bool hovered) { relTimeChartView->setHovered(point, hovered, relSeries->name()); });
        connect(relTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                SLOT(showStatus(qreal, qreal, QString)));
        connect(relTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

        // Set dateSeries ID
        if (!chartFields.contains(logValueName))
            chartFields.append(logValueName);
        dateSeries->setName(dataName);
        relSeries->setName(dataName);

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
