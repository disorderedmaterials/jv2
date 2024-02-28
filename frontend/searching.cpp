// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include "searchDialog.h"
#include <QMessageBox>

/*
 * UI
 */

void MainWindow::on_actionSearchEverywhere_triggered()
{
    backend_.getUncachedJournalCount(currentJournalSource(), [=](HttpRequestWorker *worker) { handlePreSearchResult(worker); });
}

void MainWindow::on_AcquisitionCancelButton_clicked(bool checked)
{
    if (!sourceBeingGenerated_)
        return;

    if (QMessageBox::question(
            this, "Stop Journal Acquisition?",
            QString("Are you sure you want to cancel journal acquisition for '%1'?").arg(sourceBeingGenerated_->sourceID())) ==
        QMessageBox::StandardButton::Yes)
        backend_.generateBackgroundScanStop([&](HttpRequestWorker *worker) { handleGenerateBackgroundScanStop(worker); });
}

// Update journal acquisition page for specified source
void MainWindow::updateAcquisitionPage(int nCompleted, const QString &lastJournalProcessed)
{
    ui_.AcquisitionProgressBar->setValue(nCompleted);
    ui_.AcquisitionInfoLabel->setText(QString("Last journal processed was '%1'...").arg(lastJournalProcessed));
}

/*
 * Network Handlers
 */

// Handle pre-search result
void MainWindow::handlePreSearchResult(HttpRequestWorker *worker)
{
    auto nUncached = worker->response().toInt();
    qDebug() << "Number of uncached journals = " << nUncached;
    if (nUncached != 0)
    {
        if (QMessageBox::question(this, "Acquire All Journals?",
                                  "Before a Search Everywhere query can be run all journals for the source must be cached.\n"
                                  "This only needs to be done once. Do you want to do this now? ") ==
            QMessageBox::StandardButton::No)
            return;

        sourceBeingAcquired_ = currentJournalSource();

        // Update the GUI
        ui_.AcquisitionPageLabel->setText(QString("Acquiring Journals for Source '%1'...\nSource has %2 journals in total.")
                                              .arg(sourceBeingAcquired_->name())
                                              .arg(sourceBeingAcquired_->journals().size()));
        ui_.AcquisitionProgressBar->setMaximum(nUncached);
        updateAcquisitionPage(0, "<No Journal Acquired>");

        updateForCurrentSource(JournalSource::JournalSourceState::Acquiring);

        backend_.acquireAllJournals(currentJournalSource(),
                                    [=](HttpRequestWorker *worker) { handleAcquireAllJournalsForSearch(); });
    }
    else
    {
        SearchDialog searchDialog(this);

        auto queryParameters = searchDialog.getQuery();
        if (queryParameters.empty())
            return;
        backend_.search(currentJournalSource(), queryParameters,
                        [=](HttpRequestWorker *worker) { handleSearchResult(worker); });
    }
}

// Handle acquire all journal data for search
void MainWindow::handleAcquireAllJournalsForSearch()
{
    // Create a timer to ping the backend for an update after 1000 ms
    auto *pingTimer = new QTimer;
    pingTimer->setSingleShot(true);
    pingTimer->setInterval(1000);
    connect(
        pingTimer, &QTimer::timeout,
        [&]()
        {
            backend_.acquireAllJournalsUpdate(
                [this](HttpRequestWorker *worker)
                {
                    if (worker->response().startsWith("\"NOT_RUNNING"))
                    {
                        statusBar()->showMessage("Acquisition of journals failed...", 5000);
                        if (currentJournalSource() == sourceBeingAcquired_)
                            updateForCurrentSource(JournalSource::JournalSourceState::Error);
                        sourceBeingAcquired_ = nullptr;
                    }
                    else
                    {
                        // Update the generator page of the stack
                        auto nCompleted = worker->jsonResponse()["num_completed"].toInt();
                        auto lastFilename = worker->jsonResponse()["last_filename"].toString();
                        updateAcquisitionPage(nCompleted, lastFilename);

                        // Complete?
                        if (worker->jsonResponse()["complete"].toBool())
                        {
                            sourceBeingAcquired_->setState(JournalSource::JournalSourceState::Loading);

                            statusBar()->showMessage(
                                QString("Journal acquisition completed for source '%1'.\n").arg(sourceBeingAcquired_->name()));

                            sourceBeingAcquired_ = nullptr;

                            updateForCurrentSource(JournalSource::JournalSourceState::OK);

                            on_actionSearchEverywhere_triggered();
                        }
                        else
                            handleAcquireAllJournalsForSearch();
                    }
                });
        });
    pingTimer->start();
}

// Handle search result
void MainWindow::handleSearchResult(HttpRequestWorker *worker)
{
    runData_ = QJsonArray();
    runDataModel_.setData(runData_);

    // Check network reply
    if (handleRequestError(worker, "trying to search across journals") != NoError)
        return;

    // Turn off grouping
    if (ui_.GroupRunsButton->isChecked())
        ui_.GroupRunsButton->setChecked(false);

    // Get desired fields and titles from config files
    runDataColumns_ = currentInstrument() ? currentInstrument()->get().runDataColumns()
                                          : Instrument::runDataColumns(Instrument::InstrumentType::Neutron);
    runData_ = worker->jsonResponse().array();

    // Set table data
    runDataModel_.setHorizontalHeaders(runDataColumns_);
    runDataModel_.setData(runData_);

    ui_.RunDataTable->resizeColumnsToContents();
    updateSearch(searchString_);
    ui_.RunFilterEdit->clear();

    // Set the journal state
    currentJournalSource()->setShowingSearchedData();

    updateForCurrentSource(JournalSource::JournalSourceState::OK);
}
