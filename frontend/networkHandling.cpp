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

// Handle journal ping result
void MainWindow::handlePingJournals(QString response)
{
    // A null response indicates no change
    if (response == "")
        return;

    qDebug() << response;
    qDebug() << currentJournal_->get().name();

    // The response contains the updated / most recent journal name
    if (response == journals_.front().name())
    {
        qDebug() << "Journals up to date, but runs are not\n";
        // The most recent journal has been updated, probably with new run data.
        // If we're currently displaying that journal, update with the new run data
        if (currentJournal_ && currentJournal_->get().name() == response)
        {
            backend_.updateJournal(currentInstrument().journalDirectory(), response,
                                   runData_.last().toObject()["run_number"].toString(),
                                   [this](HttpRequestWorker *worker) { handleBackendPingResult(worker); });
        }
    }
    else
    {
        auto nameParts = response.split("_");
        addJournal("Cycle " + nameParts[1] + "/" + nameParts[2].remove(".xml"), Journal::JournalLocation::ISISServer, response);
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
    journals_.clear();

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

        // Ignore the main index file
        if (value.toString() == "journal.xml")
            continue;

        auto nameParts = value.toString().split("_");
        addJournal("Cycle " + nameParts[1] + "/" + nameParts[2].remove(".xml"), Journal::JournalLocation::ISISServer,
                   value.toString());
    }

    // If there is no current journal, set one
    if (!currentJournal_)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
        auto optJournal = findJournal(settings.value("recentJournal").toString());
        if (optJournal)
            setCurrentJournal(*optJournal);
        else
            setCurrentJournal(journals_.front());
    }
}

// Handle run data returned for a whole journal
void MainWindow::handleCompleteJournalRunData(HttpRequestWorker *worker)
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
        if (currentJournal_ && currentJournal_->get().name() == worker->response)
        {
            highlightRunNumber(runNumber);
            return;
        }

        for (auto i = 0; i < cyclesMenu_->actions().count(); i++)
        {
            if (currentJournal_ && currentJournal_->get().name() == worker->response)
            {
                setCurrentJournal(cyclesMenu_->actions()[i]->text());
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
