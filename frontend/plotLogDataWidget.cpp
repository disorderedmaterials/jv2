// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "plotLogDataWidget.h"
#include <QSplitter>

namespace JV2
{
PlotLogDataWidget::PlotLogDataWidget(MainWindow *parent, Backend &backend, const JournalSource *journalSource,
                                     const std::vector<int> &runNumbers)
    : QWidget(parent), mainWindow_(parent), backend_(backend), journalSource_(journalSource), runNumbers_(runNumbers),
      logValueFilterProxy_(logValueModel_)
{
    ui_.setupUi(this);

    ui_.PropertyList->setModel(&logValueFilterProxy_);
    ui_.PropertyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(&logValueModel_, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)), this,
            SLOT(logValuesChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)));

    // Acquire the available log data
    backend_.getNeXuSLogValues(journalSource_, runNumbers_,
                               [=](HttpRequestWorker *worker) { handleRetrieveSELogValues(worker); });
}

PlotLogDataWidget::~PlotLogDataWidget() {}

/*
 * Private Functions
 */

void PlotLogDataWidget::handleRetrieveSELogValues(HttpRequestWorker *worker)
{
    // Check for errors
    if (mainWindow_->handleRequestError(worker, "retrieving log values from run") != Backend::NoError)
        return;

    // Iterate over log values extracted from the target run data and create a vector of all those available
    logValues_.reserve(1024);
    logValues_.clear();
    foreach (const auto &log, worker->jsonResponse().array())
    {
        auto logArray = log.toArray();
        if (logArray.size() < 2)
            continue;

        // Remove the name item and proceed to iterate over log values
        logArray.removeFirst();

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
            logValues_.emplace_back(block.toString().split("/").last(), block.toString());
    }

    logValueModel_.setData(logValues_);
}

// Handle retrieved log value data
void PlotLogDataWidget::handleRetrieveSELogValueData(HttpRequestWorker *worker)
{
    // Check network reply
    if (mainWindow_->handleRequestError(worker, "trying to retrieve log value data") != Backend::NoError)
    {
        ui_.PropertyList->setEnabled(true);
        return;
    }

    /* The expected result from the backend is as follows:
     *
     * result = {
     *              logValue: "name_of_log_value",
     *              runNumbers: { run1, run2, run3 ... runN }
     *              data: {
     *                  run1: {
     *                      timeRange: [ datetime, datetime ],
     *                      data: [ (x,y), (x2,y2), ..., (xn,yn) ]
     *                  },
     *                  ...
     *                  runN: {
     *                      ...
     *                  }
     *              }
     */

    const auto responseData = worker->jsonResponse().object();
    auto logValueName = responseData["logValue"].toString().section('/', -1);
    qDebug() << logValueName;

    // Find the associated LogValue
    auto valueIt = std::find_if(logValues_.begin(), logValues_.end(),
                                [logValueName](auto &value) { return value.name() == logValueName; });
    if (valueIt == logValues_.end())
    {
        ui_.PropertyList->setEnabled(true);
        return;
    }
    auto &logValue = *valueIt;

    const auto data = responseData["data"].toObject();

    foreach (const auto &run, data)
    {
        // Get the data name (run number)
        const auto dataName = run[QString("runNumber")].toString();
        qDebug() << dataName;

        // Extract the time range data
        const auto timeRange = run[QString("timeRange")].toArray();

        // Get start and end times
        auto startTime = QDateTime::fromString(timeRange.first()[0].toString(), "yyyy-MM-dd'T'HH:mm:ss");
        auto endTime = QDateTime::fromString(timeRange.first()[1].toString(), "yyyy-MM-dd'T'HH:mm:ss");

        // Get time / value vectors
        // TODO Need to check / detect enumerated data here
        const auto fieldDataArray = run[QString("data")].toArray();
        std::vector<double> epochTimes(1024);
        std::vector<double> values(1024);
        foreach (const auto &dataPair, fieldDataArray)
        {
            auto dataPairArray = dataPair.toArray();
            epochTimes.push_back(startTime.addMSecs(dataPairArray[0].toDouble() * 1000).toMSecsSinceEpoch());
            values.push_back(dataPairArray[1].toDouble());
        }

        // Push the new data
        logValue.addData(dataName, {startTime, endTime, epochTimes, values});
    }

    ui_.PropertyList->setEnabled(true);
}

/*
 * Private Slots
 */

// Log value selection changed
void PlotLogDataWidget::logValuesChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    auto optData = logValueModel_.getData(topLeft);
    auto &data = optData->get();
    qDebug() << "Toggled data was " + data.name();

    // We might already have the data, so check before we go off retrieving it again...
    // TODO

    // Disable the property list for now
    ui_.PropertyList->setDisabled(true);

    // Request the log value data
    backend_.getNexusLogValueData(journalSource_, runNumbers_, data.neXuSLocation(),
                                  [=](HttpRequestWorker *worker) { handleRetrieveSELogValueData(worker); });
}
} // namespace JV2
