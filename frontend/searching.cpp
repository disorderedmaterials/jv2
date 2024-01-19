// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include "searchDialog.h"

/*
 * UI
 */

void MainWindow::on_actionSearchEverywhere_triggered()
{
    SearchDialog searchDialog(this);

    auto queryParameters = searchDialog.getQuery();

    backend_.search(currentJournalSource(), queryParameters, [=](HttpRequestWorker *worker) { handleSearchResult(worker); });
}

/*
 * Network Handlers
 */

// Handle search result
void MainWindow::handleSearchResult(HttpRequestWorker *worker)
{
    runData_ = QJsonArray();
    runDataModel_.setData(runData_);

    // Check network reply
    if (handleRequestErrors(worker, "trying to search across journals"))
    {
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return;
    }

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
