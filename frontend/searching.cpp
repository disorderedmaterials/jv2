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



    // mass search for data
    QString searchOptions;
    QString sensitivityText = "caseSensitivity=";
    sensitivityText.append(caseSensitivity ? "true" : "false");
    searchOptions.append(sensitivityText);

    backend_.findRuns(inst.journalDirectory(), value, textInput, searchOptions,
                      [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });

    // configure caching [FIXME]
    cachedMassSearch_.append(std::make_tuple(nullptr, text));
}
//    massSearch("RB No.", "experiment_identifier");

//void MainWindow::on_actionMassSearchTitle_triggered() { massSearch("Title", "title"); }
//
//void MainWindow::on_actionMassSearchUser_triggered() { massSearch("User name", "user_name"); }
//
//void MainWindow::on_actionMassSearchRunRange_triggered() { massSearch("Run Range", "run_number"); }
//
//void MainWindow::on_actionMassSearchDateRange_triggered() { massSearch("Date Range", "start_date"); }
