// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

// Perform error check on http result
bool MainWindow::networkRequestHasError(HttpRequestWorker *worker, const QString &taskDescription)
{
    // Network error?
    if (worker->errorType != QNetworkReply::NoError)
    {
        statusBar()->showMessage("Network Error");
        QMessageBox::warning(this, "Network Error",
                             QString("A network error was encountered while %1.\nThe error returned was: %2")
                                 .arg(taskDescription, worker->errorString));
        return true;
    }

    // Response error?
    auto response = worker->response;
    if (response.contains("Error"))
    {
        statusBar()->showMessage("Response Error");
        QMessageBox::warning(
            this, "Response Error",
            QString("The backend failed while %1.\nThe response returned was: %2").arg(taskDescription, response));
        return true;
    }

    return false;
}

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

        // Get default journal sources
        getDefaultJournalSources();

        setLoadScreen(false);
    }
    else
        waitForBackend();
}

// Handle get journal updates result
void MainWindow::handleGetJournalUpdates(HttpRequestWorker *worker)
{
    // A null response indicates no change
    if (worker->response.startsWith("null"))
        return;

    // The main body of the request contains any run numbers we don't currently have.
    // If we are currently displaying grouped data we append the new data directly then refresh the grouping
    if (ui_.GroupRunsButton->isChecked())
    {
        foreach (const auto &item, worker->jsonArray)
            runData_.append(item);

        generateGroupedData();

        runDataModel_.setData(groupedRunData_);
        runDataModel_.setHorizontalHeaders(groupedRunDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        // Update via the model
        runDataModel_.appendData(worker->jsonArray);
    }
}

// Handle returned journal information for an instrument
void MainWindow::handleListJournals(HttpRequestWorker *worker)
{
    setLoadScreen(false);

    journalsMenu_->clear();
    journals_.clear();

    // Check network reply
    if (networkRequestHasError(worker, "trying to list journals"))
        return;

    // Special case - for disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response.startsWith("\"Index File Not Found\""))
    {
        QMessageBox::question(this, "Index File Doesn't Exist",
                              QString("No index file %1/%2 currently exists.\nWould you like to generate it now?")
                                  .arg(currentJournalSource().rootUrl(), currentJournalSource().indexFile()));
        return;
    }

    // Add returned journals
    for (auto i = worker->jsonArray.count() - 1; i >= 0; i--)
    {
        auto value = worker->jsonArray[i].toObject();

        addJournal(value["filename"].toString(),
                   {value["server_root"].toString(), value["directory"].toString(), value["filename"].toString()});
    }

    // If there is no current journal, set one
    if (!currentJournal_ && !journals_.empty())
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

    // Check network reply
    if (networkRequestHasError(worker, "trying to retrieve run data for the journal"))
        return;

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

    // Check network reply
    if (networkRequestHasError(worker, "trying to select run number within journal"))
        return;

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

    for (auto i = 0; i < journalsMenu_->actions().count(); i++)
    {
        if (currentJournal_ && currentJournal_->get().name() == worker->response)
        {
            setCurrentJournal(journalsMenu_->actions()[i]->text());
            highlightRunNumber(runNumber);
        }
    }
}
