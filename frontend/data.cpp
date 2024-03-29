// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

/*
 * Private Functions
 */

// Clear all run data
void MainWindow::clearRunData()
{
    runData_ = QJsonArray();
    runDataModel_.setData(runData_);
    groupedRunData_ = QJsonArray();
    ui_.GroupRunsButton->setDown(false);
}

// Get data for specified run number
std::optional<QJsonObject> MainWindow::dataForRunNumber(int runNumber) const
{
    for (const auto &value : runData_)
    {
        auto valueObj = value.toObject();
        if (valueObj["run_number"].toInt() == runNumber)
            return valueObj;
    }

    return {};
}

// Generate grouped run data from current run data
void MainWindow::generateGroupedData()
{
    // holds data in tuple as QJson referencing is incomplete
    std::vector<std::tuple<QString, QString, QString>> groupedData;
    for (const auto &value : runData_)
    {
        const QJsonObject &valueObj = value.toObject();
        bool unique = true;

        // add duplicate title data to stack
        for (std::tuple<QString, QString, QString> &data : groupedData)
        {
            if (std::get<0>(data) == valueObj["title"].toString())
            {
                auto currentTotal = QTime::fromString(std::get<1>(data), "HH:mm:ss");
                // convert duration to seconds
                auto newTime = QTime(0, 0, 0).secsTo(QTime::fromString(valueObj["duration"].toString(), "HH:mm:ss"));
                auto totalRunTime = currentTotal.addSecs(newTime).toString("HH:mm:ss");
                std::get<1>(data) = QString(totalRunTime);
                std::get<2>(data) += ";" + valueObj["run_number"].toString();
                unique = false;
                break;
            }
        }
        if (unique)
            groupedData.emplace_back(valueObj["title"].toString(), valueObj["duration"].toString(),
                                     valueObj["run_number"].toString());
    }

    // Clear existing grouped data and generate new
    groupedRunData_ = QJsonArray();
    for (const auto &group : groupedData)
    {
        auto groupData = QJsonObject({qMakePair(QString("title"), QJsonValue(std::get<0>(group))),
                                      qMakePair(QString("duration"), QJsonValue(std::get<1>(group))),
                                      qMakePair(QString("run_number"), QJsonValue(std::get<2>(group)))});
        groupedRunData_.push_back(QJsonValue(groupData));
    }
}

// Return the run data model index under the mouse, accounting for the effects of the filter proxy
const QModelIndex MainWindow::runDataIndexAtPos(const QPoint pos) const
{
    auto index = ui_.RunDataTable->indexAt(pos);
    return index.isValid() ? runDataFilterProxy_.mapToSource(index) : QModelIndex();
}

// Return integer list of currently-selected run numbers
std::vector<int> MainWindow::selectedRunNumbers() const
{
    // Get current selection
    auto selectedRuns = ui_.RunDataTable->selectionModel()->selectedRows();
    std::vector<int> runNumbers;

    // Concats runs
    for (const auto &runIndex : selectedRuns)
    {
        auto runNo = runDataFilterProxy_.getData("run_number", runIndex);

        // Account for grouped run information
        auto runNoArray = runNo.split(",");
        for (const auto &n : runNoArray)
            runNumbers.push_back(n.toInt());
    }

    return runNumbers;
}

// Select and show specified run number in table (if it exists)
void MainWindow::highlightRunNumber(int runNumber)
{
    // Get the index of the specified run number in the underlying data
    auto index = runDataModel_.indexOfData("run_number", QString::number(runNumber));
    if (!index.isValid())
        return;

    auto filterIndex = runDataFilterProxy_.mapFromSource(index);
    if (!filterIndex.isValid())
        return;

    ui_.RunDataTable->selectionModel()->setCurrentIndex(filterIndex,
                                                        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ui_.RunDataTable->scrollTo(filterIndex);

    statusBar()->showMessage("Jumped to run " + QString::number(runNumber) + " in " + currentJournal().name(), 5000);
}

/*
 * UI
 */

void MainWindow::on_actionRefreshJournal_triggered()
{
    if (!currentJournalSource_)
        return;

    if (currentJournalSource_->type() == JournalSource::IndexingType::Network)
        backend_.getJournalUpdates(currentJournalSource_, [=](HttpRequestWorker *worker) { handleGetJournalUpdates(worker); });
    else
    {
        if (sourceBeingGenerated_)
            ui_.statusbar->showMessage("Can't refresh a generated source while another is being generated...");
        else
        {
            sourceBeingGenerated_ = currentJournalSource_;
            backend_.generateList(currentJournalSource_, [=](HttpRequestWorker *worker)
                                  { handleGenerateList(worker, Backend::JournalGenerationStyle::UpdateAll); });
        }
    }
}

// Jump to run number
void MainWindow::on_actionJumpTo_triggered()
{
    if (!currentInstrument())
        return;
    auto &inst = currentInstrument()->get();

    auto ok = false;
    int runNo = QInputDialog::getInt(this, tr("Jump To"), tr("Run number to jump to:"), 1, 1, 2147483647, 1, &ok);
    if (!ok)
        return;

    backend_.findJournal(currentJournalSource(), runNo,
                         [=](HttpRequestWorker *workerProxy) { handleJumpToJournal(workerProxy); });
}

// Run data context menu requested
void MainWindow::runDataContextMenuRequested(QPoint pos)
{
    QMenu contextMenu;

    // Selection
    auto *selectSameTitle = contextMenu.addAction("Select identical titles");
    contextMenu.addSeparator();

    // SE log plotting options
    auto *plotSELog = contextMenu.addAction("Plot SE log values...");
    contextMenu.addSeparator();

    // Spectrum plotting
    auto *plotDetector = contextMenu.addAction("Plot detector...");
    auto *plotMonitor = contextMenu.addAction("Plot monitor...");

    auto *selectedAction = contextMenu.exec(ui_.RunDataTable->mapToGlobal(pos));

    if (selectedAction == selectSameTitle)
    {
        auto title = runDataModel_.getData("title", runDataIndexAtPos(pos));

        // Iterate over displayed rows (i.e. via filter proxy)
        for (auto i = 0; i < runDataFilterProxy_.rowCount(); ++i)
        {
            if (runDataModel_.getData("title", runDataFilterProxy_.mapToSource(runDataFilterProxy_.index(i, 0))) == title)
                ui_.RunDataTable->selectionModel()->setCurrentIndex(runDataFilterProxy_.index(i, 0),
                                                                    QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        statusBar()->showMessage(QString("Selected %1 runs titled \"%2\".")
                                     .arg(ui_.RunDataTable->selectionModel()->selectedRows().count())
                                     .arg(title));
    }
    else if (selectedAction == plotSELog)
    {
        backend_.getNexusFields(currentJournalSource(), selectedRunNumbers(),
                                [=](HttpRequestWorker *worker) { handlePlotSELogValue(worker); });
    }
    else if (selectedAction == plotDetector)
    {
        backend_.getNexusSpectrumCount(currentJournalSource(), "detector", selectedRunNumbers().front(),
                                       [=](HttpRequestWorker *worker) { plotSpectra(worker); });
    }
    else if (selectedAction == plotMonitor)
    {
        backend_.getNexusSpectrumCount(currentJournalSource(), "monitor", selectedRunNumbers().front(),
                                       [=](HttpRequestWorker *worker) { plotMonSpectra(worker); });
    }
}
