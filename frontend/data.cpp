// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

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

// Get selected run / cycle information [LEGACY, TO FIX]
std::pair<QString, QString> MainWindow::selectedRunNumbersAndCycles() const
{
    // Get selected run numbers / cycles
    auto selectedRuns = ui_.RunDataTable->selectionModel()->selectedRows();
    QString runNos, cycles;

    // Concats runs
    for (const auto &runIndex : selectedRuns)
    {
        auto runNo = runDataFilterProxy_.getData("run_number", runIndex);
        auto cycle = runDataFilterProxy_.getData("isis_cycle", runIndex);

        // Account for grouped run information
        auto runNoArray = runNo.split(",");
        for (auto n : runNoArray)
        {
            runNos.append(runNos.isEmpty() ? runNo : (";" + runNo));
            cycles.append((cycles.isEmpty() ? "cycle_" : ";cycle_") + cycle);
        }
    }
    qDebug() << runNos;
    qDebug() << cycles;
    return {runNos, cycles};
}

// Select and show specified run number in table (if it exists)
bool MainWindow::highlightRunNumber(int runNumber)
{
    // ui_.RunDataTable->selectionModel()->clearSelection();

    // searchString_ = "";
    // updateSearch(searchString_);

    // Find the run number in the current run data
    for (auto row = 0; row < runDataModel_.rowCount(); ++row)
        if (runDataModel_.getData("run_number", row).toInt() == runNumber)
        {
            // If grouping is enabled, turn it off
            if (ui_.GroupRunsButton->isChecked())
                ui_.GroupRunsButton->click();

            ui_.RunDataTable->selectionModel()->setCurrentIndex(
                runDataModel_.index(row, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

            statusBar()->showMessage("Jumped to run " + QString::number(runNumber) + " in " + ui_.cycleButton->text(), 5000);
            return true;
        }

    statusBar()->showMessage("Run " + QString::number(runNumber) + " not present in " + ui_.cycleButton->text(), 5000);

    return false;
}

/*
 * UI
 */

void MainWindow::on_actionRefresh_triggered()
{
    if (journalSource_ == Journal::JournalLocation::ISISServer)
    {
        backend_.pingJournals(currentInstrument().journalDirectory(),
                              [=](HttpRequestWorker *worker) { handlePingJournals(worker->response); });
    }
}

// Jump to run number
void MainWindow::on_actionJumpTo_triggered()
{
    if (!currentInstrument_)
        return;
    auto &inst = currentInstrument_->get();

    auto ok = false;
    int runNo = QInputDialog::getInt(this, tr("Jump To"), tr("Run number to jump to:"), 0, 1, 2147483647, 1, &ok);
    if (!ok)
        return;

    backend_.goToCycle(inst.journalDirectory(), QString::number(runNo),
                       [=](HttpRequestWorker *workerProxy) { handleSelectRunNoInCycle(workerProxy, runNo); });

    setLoadScreen(true);
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

    // Get selected run numbers / cycles
    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();

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
        backend_.getNexusFields(currentInstrument().dataDirectory(), cycles, runNos,
                                [=](HttpRequestWorker *worker) { handlePlotSELogValue(worker); });
    }
    else if (selectedAction == plotDetector)
    {
        getSpectrumCount();
    }
    else if (selectedAction == plotMonitor)
    {
        getMonitorCount();
    }
}
