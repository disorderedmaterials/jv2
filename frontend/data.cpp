// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
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

// Returns run and cycle values for selected runs
QString MainWindow::getRunNos()
{
    // Gathers all selected runs
    auto selectedRuns = ui_.RunDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColumn;
    int cycleColumn;
    for (auto i = 0; i < ui_.RunDataTable->horizontalHeader()->count(); ++i)
    {
        if (runDataModel_.headerData(i, Qt::Horizontal, Qt::UserRole).toString() == "run_number")
            runNoColumn = i;
        else if (runDataModel_.headerData(i, Qt::Horizontal, Qt::UserRole).toString() == "isis_cycle")
            cycleColumn = i;
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;
    QString cycles = "";
    QString cycle;

    // Concats runs
    for (auto run : selectedRuns)
    {
        runNo = runDataFilterProxy_.index(run.row(), runNoColumn).data().toString();
        cycle = runDataFilterProxy_.index(run.row(), cycleColumn).data().toString();
        if (runNo.contains("-") || runNo.contains(","))
        {
            QString groupedRuns;
            auto runArr = runNo.split(",");
            foreach (const auto &string, runArr)
            {
                if (string.contains("-"))
                {
                    for (auto i = string.split("-")[0].toInt(); i <= string.split("-")[1].toInt(); i++)
                        groupedRuns += QString::number(i) + ";";
                }
                else
                    groupedRuns += string + ";";
            }
            groupedRuns.chop(1);
            runNos.append(groupedRuns + ";");
        }
        else
        {
            runNos.append(runNo + ";");
            cycles.append("cycle_" + cycle + ";");
        }
    }

    // Removes final ";"
    runNos.chop(1);
    cycles.chop(1);
    return runNos + "-" + cycles;
}

// Return the run data model index under the mouse, accounting for the effects of the filter proxy
const QModelIndex MainWindow::runDataIndexAtPos(const QPoint pos) const
{
    auto index = ui_.RunDataTable->indexAt(pos);
    return index.isValid() ? runDataFilterProxy_.mapToSource(index) : QModelIndex();
}

void MainWindow::checkForUpdates()
{
    QString url_str = "http://127.0.0.1:5000/pingCycle/" + currentInstrument().lowerCaseName();
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { handleCycleUpdate(workerProxy->response); });
    worker->execute(input);
}

/*
 * HTTP Worker Handling
 */

// Handle cycle update result
void MainWindow::handleCycleUpdate(QString response)
{
    // TODO
    if (response != "")
    {
        qDebug() << "Update";
        // currentInstrumentChanged(currentInstrument().name());
        if (cyclesMap_[cyclesMenu_->actions()[0]->text()] != response) // if new cycle found
        {
            auto displayName = "Cycle " + response.split("_")[1] + "/" + response.split("_")[2].remove(".xml");
            cyclesMap_[displayName] = response;

            auto *action = new QAction(displayName, this);
            connect(action, &QAction::triggered, [=]() { setCurrentCycle(displayName); });
            cyclesMenu_->insertAction(cyclesMenu_->actions()[0], action);
        }
        else if (cyclesMap_[ui_.cycleButton->text()] == response) // if current opened cycle changed
        {
            QString url_str = "http://127.0.0.1:5000/updateJournal/" + currentInstrument().lowerCaseName() + "/" + response +
                              "/" + runData_.last().toObject()["run_number"].toString();
            HttpRequestInput input(url_str);
            auto *worker = new HttpRequestWorker(this);
            connect(worker, &HttpRequestWorker::on_execution_finished,
                    [=](HttpRequestWorker *workerProxy) { handleRunData(workerProxy); });
            worker->execute(input);
        }
    }
    else
    {
        qDebug() << "no change";
        return;
    }
}

// Handle JSON run data returned from workers
void MainWindow::handleRunData(HttpRequestWorker *worker)
{
    runData_ = worker->jsonArray;
    runDataModel_.setData(runData_);
}

// Handle returned cycle information for an instrument
void MainWindow::handleGetCycles(HttpRequestWorker *worker)
{
    setLoadScreen(false);

    cyclesMenu_->clear();
    cyclesMap_.clear();

    if (worker->errorType != QNetworkReply::NoError)
    {
        // an error occurred
        QString msg = "Error1: " + worker->errorString;
        QMessageBox::information(this, "", msg);
        return;
    }

    auto response = worker->response;

    QJsonValue value;
    for (auto i = worker->jsonArray.count() - 1; i >= 0; i--)
    {
        value = worker->jsonArray[i];
        // removes header_ file
        if (value.toString() != "journal.xml")
        {
            auto displayName = "Cycle " + value.toString().split("_")[1] + "/" + value.toString().split("_")[2].remove(".xml");
            cyclesMap_[displayName] = value.toString();

            auto *action = new QAction(displayName, this);
            connect(action, &QAction::triggered, [=]() { setCurrentCycle(displayName); });
            cyclesMenu_->addAction(action);
        }
    }

    if (init_)
    {
        // Sets cycle to most recently viewed
        recentCycle();
        init_ = false;
        return;
    }
    // Keep cycle over instruments
    for (QAction *action : cyclesMenu_->actions())
    {
        if (action->text() == ui_.cycleButton->text())
        {
            action->trigger();
            return;
        }
    }
    cyclesMenu_->actions()[0]->trigger();
}

// Handle run data returned for a whole cycle
void MainWindow::handleCycleRunData(HttpRequestWorker *worker)
{
    setLoadScreen(false);

    runData_ = QJsonArray();
    runDataModel_.setData(runData_);

    // Network error?
    if (worker->errorType != QNetworkReply::NoError)
    {
        statusBar()->showMessage("Network error!");
        QMessageBox::warning(
            this, "Network Error",
            "A network error was encountered while trying to retrieve run data for the cycle\nThe error returned was: " +
                worker->errorString);
        return;
    }

    // Source error?
    if (worker->response.contains("invalid source"))
    {
        statusBar()->showMessage("Invalid journal source!");
        QMessageBox::warning(this, "Invalid Journal Source", "The journal could not be retrieved.");
        return;
    }

    // Error handling
    if (ui_.GroupRunsButton->isChecked())
        ui_.GroupRunsButton->setChecked(false);

    // Get desired fields and titles from config files
    runDataColumns_ = currentInstrument().runDataColumns();
    runData_ = worker->jsonArray;

    // Set table data
    runDataModel_.setHorizontalHeaders(runDataColumns_);
    runDataModel_.setData(runData_);

    // Fills viewMenu_ with all columns
    viewMenu_->clear();
    // viewMenu_->addAction("Save column state", this, SLOT(savePref()));
    // viewMenu_->addAction("Reset column state to default", this, SLOT(clearPref()));
    viewMenu_->addSeparator();
    auto jsonObject = runData_.at(0).toObject();
    foreach (const QString &key, jsonObject.keys())
    {
        auto *checkBox = new QCheckBox(viewMenu_);
        auto *checkableAction = new QWidgetAction(viewMenu_);
        checkableAction->setDefaultWidget(checkBox);
        checkBox->setText(key);
        checkBox->setCheckState(Qt::PartiallyChecked);
        viewMenu_->addAction(checkableAction);
        connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(columnHider(int)));

        // // Filter table based on desired headers
        // auto it =
        //     std::find_if(desiredHeader_.begin(), desiredHeader_.end(), [key](const auto &data) { return data.first == key;
        //     });
        // // If match found
        // if (it != desiredHeader_.end())
        //     checkBox->setCheckState(Qt::Checked);
        // else
        //     checkBox->setCheckState(Qt::Unchecked);
    }
    // int logIndex;
    // for (auto i = 0; i < desiredHeader_.size(); ++i)
    // {
    //     for (auto j = 0; j < ui_.RunDataTable->horizontalHeader()->count(); ++j)
    //     {
    //         logIndex = ui_.RunDataTable->horizontalHeader()->logicalIndex(j);
    //         // If index matches model data, swap columns in view
    //         if (desiredHeader_[i].first == runDataModel_.headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
    //         {
    //             ui_.RunDataTable->horizontalHeader()->swapSections(j, i);
    //         }
    //     }
    // }
    ui_.RunDataTable->resizeColumnsToContents();
    updateSearch(searchString_);
    ui_.RunFilterEdit->clear();
    emit tableFilled();
}

/*
 * UI
 */

// Set current cycle being displayed
void MainWindow::setCurrentCycle(QString cycleName)
{
    if (cycleName[0] == '[')
    {
        auto it = std::find_if(cachedMassSearch_.begin(), cachedMassSearch_.end(),
                               [cycleName](const auto &tuple)
                               { return std::get<1>(tuple) == cycleName.mid(1, cycleName.length() - 2); });
        if (it != cachedMassSearch_.end())
        {
            ui_.cycleButton->setText(cycleName);
            setLoadScreen(true);
            handleCycleRunData(std::get<0>(*it));
        }
        return;
    }
    ui_.cycleButton->setText(cycleName);

    QString url_str = "http://127.0.0.1:5000/getJournal/" + currentInstrument().lowerCaseName() + "/" + cyclesMap_[cycleName];
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);

    // Call result handler when request completed
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handleCycleRunData(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (cyclesMenu_->actions().count() == 0)
        QWidget::setEnabled(false);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentCycle = settings.value("recentCycle").toString();
    // Sets cycle to last used/ most recent if unavailable
    for (QAction *action : cyclesMenu_->actions())
    {
        if (action->text() == recentCycle)
        {
            action->trigger();
            return;
        }
    }
    cyclesMenu_->actions()[0]->trigger();
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

    // auto index = ui_.RunDataTable->indexAt(pos);
    // auto runNos = getRunNos().split("-")[0];
    // auto cycles = getRunNos().split("-")[1];
    // if (cycles == "")
    // {
    //     for (auto run : runNos.split(";"))
    //         cycles.append(" ;");
    //     cycles.chop(1);
    // }

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

        auto *worker = new HttpRequestWorker(this);
        connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handlePlotSELogValue(HttpRequestWorker *)));
        worker->execute({"http://127.0.0.1:5000/getNexusFields/" + currentInstrument().archiveDirectory() + "/" + "CYCLEFIXME" +
                         "/" + "RUNNOSFIXME"});
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
