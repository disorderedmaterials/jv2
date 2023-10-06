// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>
#include <QMessageBox>

/*
 * UI
 */

void MainWindow::on_RunFilterEdit_textChanged(const QString &arg1)
{
    runDataFilterProxy_.setFilterString(arg1.trimmed());
    runDataFilterProxy_.setFilterKeyColumn(-1);

    // Update search to new data
    if (searchString_ != "")
        updateSearch(searchString_);
}

void MainWindow::on_RunFilterCaseSensitivityButton_clicked(bool checked) { runDataFilterProxy_.setCaseSensitivity(checked); }

// Groups table data
void MainWindow::on_GroupRunsButton_clicked(bool checked)
{
    if (checked)
    {
        generateGroupedData();

        runDataModel_.setData(groupedRunData_);
        runDataModel_.setHorizontalHeaders(groupedRunDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        runDataModel_.setData(runData_);
        runDataModel_.setHorizontalHeaders(runDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }

    updateSearch(searchString_);
}

// Clears filter parameters
void MainWindow::on_RunFilterClearButton_clicked(bool checked) { ui_.RunFilterEdit->clear(); }
