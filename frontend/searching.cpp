// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

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

    // configure caching [FIXME]
    //    cachedMassSearch_.append(std::make_tuple(nullptr, text));
}
//    massSearch("RB No.", "experiment_identifier");

// void MainWindow::on_actionMassSearchTitle_triggered() { massSearch("Title", "title"); }
//
// void MainWindow::on_actionMassSearchUser_triggered() { massSearch("User name", "user_name"); }
//
// void MainWindow::on_actionMassSearchRunRange_triggered() { massSearch("Run Range", "run_number"); }
//
// void MainWindow::on_actionMassSearchDateRange_triggered() { massSearch("Date Range", "start_date"); }

/*
 * Network Handlers
 */

// Handle search result
void MainWindow::handleSearchResult(HttpRequestWorker *worker)
{
    runData_ = QJsonArray();
    runDataModel_.setData(runData_);

    // Check network reply
    if (networkRequestHasError(worker, "trying to search across journals"))
    {
        updateForCurrentSource(JournalSource::JournalSourceState::NetworkError);
        return;
    }

    // Turn off grouping
    if (ui_.GroupRunsButton->isChecked())
        ui_.GroupRunsButton->setChecked(false);

    // Get desired fields and titles from config files
    runDataColumns_ = currentInstrument() ? currentInstrument()->get().runDataColumns()
                                          : Instrument::runDataColumns(Instrument::InstrumentType::Neutron);
    runData_ = worker->jsonArray;

    // Set table data
    runDataModel_.setHorizontalHeaders(runDataColumns_);
    runDataModel_.setData(runData_);

    ui_.RunDataTable->resizeColumnsToContents();
    updateSearch(searchString_);
    ui_.RunFilterEdit->clear();

    // Set the journal state
    currentJournalSource().setShowingSearchedData();

    updateForCurrentSource(JournalSource::JournalSourceState::OK);
}
