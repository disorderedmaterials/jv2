// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

// Handle backend ping result
void MainWindow::handleBackendPingResult(HttpRequestWorker *worker)
{
    if (worker->response.contains("READY"))
    {
        // Connect up an update timer
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [=]() { on_actionRefresh_triggered(); });
        timer->start(30000);

        // Update the GUI
        fillInstruments();

        // Last used instrument?
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
        auto recentInstrument = settings.value("recentInstrument", instruments_.front().name()).toString();
        setCurrentInstrument(recentInstrument);

        setLoadScreen(false);
    }
    else
        waitForBackend();
}

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
            backend_.updateJournal(currentInstrument().journalDirectory(), response,
                                   runData_.last().toObject()["run_number"].toString(),
                                   [this](HttpRequestWorker *worker) { handleBackendPingResult(worker); });
            auto *worker = backend_.TESTCreateHttpRequestWorker(this);
            // connect(worker, &HttpRequestWorker::requestFinished,
            //         [=](HttpRequestWorker *workerProxy) { handleRunData(workerProxy); });
            // worker->execute("http://127.0.0.1:5000/updateJournal/" + currentInstrument().journalDirectory() + "/" + response
            // +
            //                 "/" + runData_.last().toObject()["run_number"].toString());
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
void MainWindow::handleListCycles(HttpRequestWorker *worker)
{
    setLoadScreen(false);

    cyclesMenu_->clear();
    cyclesMap_.clear();

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

    // Other error?
    auto response = worker->response;
    if (response.contains("Error"))
    {
        statusBar()->showMessage("Network error!");
        QMessageBox::warning(this, "An Error Occurred", response);
        return;
    }

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

    // Turn off grouping
    if (ui_.GroupRunsButton->isChecked())
        ui_.GroupRunsButton->setChecked(false);

    // Get desired fields and titles from config files
    runDataColumns_ = currentInstrument().runDataColumns();
    runData_ = worker->jsonArray;

    // Set table data
    runDataModel_.setHorizontalHeaders(runDataColumns_);
    runDataModel_.setData(runData_);

    ui_.RunDataTable->resizeColumnsToContents();
    updateSearch(searchString_);
    ui_.RunFilterEdit->clear();
}

// Handle jump to specified run numbers
void MainWindow::handleSelectRunNoInCycle(HttpRequestWorker *worker, int runNumber)
{
    setLoadScreen(false);
    QString msg;

    if (worker->errorType == QNetworkReply::NoError)
    {
        if (worker->response == "Not Found")
        {
            statusBar()->showMessage("Search query not found", 5000);
            return;
        }
        if (cyclesMap_[ui_.cycleButton->text()] == worker->response)
        {
            printf("HERE\n");
            highlightRunNumber(runNumber);
            return;
        }

        for (auto i = 0; i < cyclesMenu_->actions().count(); i++)
        {
            if (cyclesMap_[cyclesMenu_->actions()[i]->text()] == worker->response)
            {
                setCurrentCycle(cyclesMenu_->actions()[i]->text());
                highlightRunNumber(runNumber);
            }
        }
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->errorString;
        QMessageBox::information(this, "", msg);
    }
}
