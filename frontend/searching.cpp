// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <tuple>

// Search table data
void MainWindow::on_searchBox_textChanged(const QString &arg1)
{
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (arg1.isEmpty())
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }
    // Find all occurences of search string in table elements
    for (auto i = 0; i < proxyModel_->columnCount(); ++i)
    {
        auto location = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (ui_->runDataTable->isColumnHidden(location) == false)
            foundIndices_.append(
                proxyModel_->match(proxyModel_->index(0, location), Qt::DisplayRole, arg1, -1, Qt::MatchContains));
    }
    // Select first match
    if (foundIndices_.size() > 0)
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

// Select previous match
void MainWindow::on_findUp_clicked()
{
    // Boundary/ error handling
    if (foundIndices_.size() > 0)
    {
        if (currentFoundIndex_ >= 1)
            currentFoundIndex_ -= 1;
        else
            currentFoundIndex_ = 0;
        goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                                 QString::number(foundIndices_.size()) + " Results");
    }
}

// Select next match
void MainWindow::on_findDown_clicked()
{
    // Boundary/ error handling
    if (foundIndices_.size() > 0)
    {
        if (currentFoundIndex_ < foundIndices_.size() - 1)
            currentFoundIndex_ += 1;
        goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                                 QString::number(foundIndices_.size()) + " Results");
    }
}

// Select all matches
void MainWindow::on_searchAll_clicked()
{
    // Error handling
    if (foundIndices_.size() > 0)
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        currentFoundIndex_ = -1;
        for (auto i = 0; i < foundIndices_.size(); ++i)
        {
            ui_->runDataTable->selectionModel()->setCurrentIndex(foundIndices_[i],
                                                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        statusBar()->showMessage("Find \"" + searchString_ + "\": Selecting " + QString::number(foundIndices_.size()) +
                                 " Results");
    }
}
void MainWindow::goToCurrentFoundIndex(QModelIndex index)
{
    ui_->runDataTable->selectionModel()->setCurrentIndex(index,
                                                         QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}