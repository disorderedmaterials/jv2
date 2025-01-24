// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "plotLogDataWidget.h"
#include <QSplitter>
#include <set>

namespace JV2
{
PlotLogDataWidget::PlotLogDataWidget(MainWindow *parent, Backend &backend, const JournalSource *journalSource,
                                     const std::vector<int> &runNumbers)
    : QWidget(parent), mainWindow_(parent), backend_(backend), journalSource_(journalSource), runNumbers_(runNumbers),
      availableLogValueFilterProxy_(logValueModel_), shownLogValueFilterProxy_(logValueModel_)
{
    ui_.setupUi(this);

    connect(&logValueModel_, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)), this,
            SLOT(logValueChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)));

    // Create LogValueGroups for each run we've been given and set up the relevant list model
    for (auto runNumber : runNumbers_)
        logValueGroups_.emplace_back(QString::number(runNumber), true);
    ui_.RunNumberList->setModel(&logValueGroupModel_);
    logValueGroupModel_.setData(logValueGroups_);

    connect(&logValueGroupModel_, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)), this,
            SLOT(logValueGroupChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)));

    // Set up the shown log value list and model
    shownLogValueFilterProxy_.setSelectedStateBehaviour(LogValueFilterProxy::SelectedStateBehaviour::ShowOnlySelected);
    shownLogValueFilterProxy_.sort(0);
    ui_.ShownLogValueList->setModel(&shownLogValueFilterProxy_);
    connect(ui_.ShownLogValueList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this, SLOT(shownLogValuesSelectionChanged(const QItemSelection &, const QItemSelection &)));

    // Acquire the available log data
    backend_.getNeXuSLogValues(journalSource_, runNumbers_,
                               [=](HttpRequestWorker *worker) { handleRetrieveSELogValues(worker); });

    // Set up the available log value list and model
    availableLogValueFilterProxy_.setSelectedStateBehaviour(LogValueFilterProxy::SelectedStateBehaviour::HideSelected);
    availableLogValueFilterProxy_.sort(0);
    ui_.AvailableLogValueList->setModel(&availableLogValueFilterProxy_);
    connect(ui_.AvailableLogValueList->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this,
            SLOT(availableLogValuesSelectionChanged(const QItemSelection &, const QItemSelection &)));

    // Acquire the available log data
    backend_.getNeXuSLogValues(journalSource_, runNumbers_,
                               [=](HttpRequestWorker *worker) { handleRetrieveSELogValues(worker); });
}

/*
 * Private Functions
 */

void PlotLogDataWidget::handleRetrieveSELogValues(HttpRequestWorker *worker)
{
    // Check for errors
    if (mainWindow_->handleRequestError(worker, "retrieving log values from run") != Backend::NoError)
        return;

    // Iterate over log values extracted from the target run data and create a unique set of those available
    std::set<QString> uniqueValues;
    foreach (const auto &log, worker->jsonResponse().array())
    {
        auto logArray = log.toArray();
        if (logArray.size() < 2)
            continue;

        // Remove the name item and proceed to iterate over log values
        logArray.removeFirst();

        auto logArrayVar = logArray.toVariantList();
        foreach (const auto &block, logArrayVar)
            uniqueValues.insert(block.toString());
    }

    // Copy the set to our vector
    logValues_.clear();
    logValues_.resize(uniqueValues.size());
    std::transform(uniqueValues.begin(), uniqueValues.end(), logValues_.begin(),
                   [](const auto &blockPath) { return LogValue(blockPath.split("/").last(), blockPath); });

    logValueModel_.setData(logValues_);
}

// Handle retrieved log value data
void PlotLogDataWidget::handleRetrieveSELogValueData(HttpRequestWorker *worker)
{
    // Check network reply
    if (mainWindow_->handleRequestError(worker, "trying to retrieve log value data") != Backend::NoError)
    {
        ui_.AvailableLogValueList->setEnabled(true);
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
        ui_.AvailableLogValueList->setEnabled(true);
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
        auto startSecs = startTime.toSecsSinceEpoch();

        // Get time / value vectors
        // TODO Need to check / detect enumerated data here
        const auto fieldDataArray = run[QString("data")].toArray();
        std::vector<double> epochTimes;
        epochTimes.reserve(1024);
        std::vector<double> values;
        values.reserve(1024);
        foreach (const auto &dataPair, fieldDataArray)
        {
            auto dataPairArray = dataPair.toArray();
            epochTimes.push_back(dataPairArray[0].toDouble());
            values.push_back(dataPairArray[1].toDouble());
        }

        // Push the new data
        logValue.addData(dataName, {startTime, endTime, epochTimes, values});
    }

    // Add the data to the plot
    showData(logValue);

    ui_.AvailableLogValueList->setEnabled(true);
}

// Show data from the supplied LogValue on the plot
void PlotLogDataWidget::showData(const LogValue &logValue)
{
    // Record whether the plot is currently empty
    auto plotEmpty = ui_.Plot->nDataEntities() == 0;

    // Add each contained per-run dataset to the plot
    for (auto &&[dataName, data] : logValue.data())
    {
        // Create a display group with some default policies
        auto group = ui_.Plot->addDisplayGroup();
        group->setSingleColour({255, 0, 200, 255});

        // Create a renderable and add it to the group
        auto *renderable = ui_.Plot->addData1D((dataName + "/" + logValue.name()));
        renderable->setData(data.times(), data.values());
        group->addTarget(renderable);
    }

    // If the plot was empty when we started, auto-scale it now
    if (plotEmpty)
        ui_.Plot->showAllData();

    emit(summaryTextChanged(summaryText(logValue.name())));
}

// Hide data from the supplied LogValue from the plot
void PlotLogDataWidget::hideData(const LogValue &logValue)
{
    for (auto &&[dataName, data] : logValue.data())
    {
        ui_.Plot->removeData1D((dataName + "/" + logValue.name()));
    }
}

/*
 * Private Slots
 */

// Log value model data changed
void PlotLogDataWidget::logValueChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    auto optLogValue = logValueModel_.getData(topLeft);
    auto &logValue = optLogValue->get();

    // If the logValue has been selected, then either redisplay or retrieve the data
    if (logValue.isSelected())
    {
        // We might already have the data, so check before we go off retrieving it again...
        if (!logValue.data().empty())
        {
            showData(logValue);
            return;
        }

        // Disable the property list for now
        ui_.AvailableLogValueList->setDisabled(true);

        // Request the log value data
        backend_.getNexusLogValueData(journalSource_, runNumbers_, logValue.neXuSLocation(),
                                      [=](HttpRequestWorker *worker) { handleRetrieveSELogValueData(worker); });
    }
    else
    {
        // Just hide the data as this value is no longer selected
        hideData(logValue);
    }
}

// Log value group model data changed
void PlotLogDataWidget::logValueGroupChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                             const QList<int> &roles)
{
    auto optGroup = logValueGroupModel_.getData(topLeft);
    auto &group = optGroup->get();

    // We have tagged every data entity on the plot as "RunNumber/Property" so we just need to request that those
    // with a matching tag are shown / hidden
    ui_.Plot->setDataEnabled(QRegularExpression(QString("^%1/.*").arg(group.name())), group.isSelected());
}

// Log values selection changed
void PlotLogDataWidget::availableLogValuesSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    ui_.ShowLogValueButton->setDisabled(selected.isEmpty());
}

void PlotLogDataWidget::shownLogValuesSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    ui_.HideLogValueButton->setDisabled(selected.isEmpty());
}

void PlotLogDataWidget::on_ShowLogValueButton_clicked(bool checked)
{
    logValueModel_.setSelected(
        availableLogValueFilterProxy_.mapSelectionToSource(ui_.AvailableLogValueList->selectionModel()->selection()).indexes(),
        true);
}

void PlotLogDataWidget::on_HideLogValueButton_clicked(bool checked)
{
    logValueModel_.setSelected(
        shownLogValueFilterProxy_.mapSelectionToSource(ui_.ShownLogValueList->selectionModel()->selection()).indexes(), false);
}

/*
 * Public
 */

// Create summary text for the plot
QString PlotLogDataWidget::summaryText(const QString &lastProperty) const
{
    if (runNumbers_.empty())
        return "Nothing";

    // Run number (count)
    auto result = QString("%1").arg(runNumbers_.front());
    if (runNumbers_.size() > 1)
        result += QString("(+%1)").arg(runNumbers_.size() - 1);

    // Last Property
    if (!lastProperty.isEmpty())
        result += QString(" / %1").arg(lastProperty);

    return result;
}

} // namespace JV2
