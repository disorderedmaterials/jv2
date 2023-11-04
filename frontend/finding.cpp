// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>

/*
 * Private Functions
 */

// Search table data
void MainWindow::updateSearch(const QString &arg1)
{
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (arg1.isEmpty())
    {
        ui_.RunDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }
    // Find all occurrences of search string in table elements
    for (auto i = 0; i < runDataFilterProxy_.columnCount(); ++i)
    {
        auto location = ui_.RunDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_.RunDataTable->isColumnHidden(location))
            foundIndices_.append(runDataFilterProxy_.match(runDataFilterProxy_.index(0, location), Qt::DisplayRole, arg1, -1,
                                                           Qt::MatchContains));
    }

    // Select first match
    if (!foundIndices_.empty())
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_.RunDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

// Select previous match
void MainWindow::findUp()
{
    if (foundIndices_.empty())
        return;

    if (currentFoundIndex_ >= 1)
        currentFoundIndex_ -= 1;
    else
        currentFoundIndex_ = foundIndices_.size() - 1;
    goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
    statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                             QString::number(foundIndices_.size()) + " Results");
}

// Select next match
void MainWindow::findDown()
{
    if (foundIndices_.empty())
        return;

    currentFoundIndex_ = ++currentFoundIndex_ % foundIndices_.size();
    goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
    statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                             QString::number(foundIndices_.size()) + " Results");
}

// Select all matches
void MainWindow::selectAllSearches()
{
    if (foundIndices_.empty())
        return;

    ui_.RunDataTable->selectionModel()->clearSelection();
    currentFoundIndex_ = -1;
    for (auto i = 0; i < foundIndices_.size(); ++i)
    {
        ui_.RunDataTable->selectionModel()->setCurrentIndex(foundIndices_[i],
                                                            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    statusBar()->showMessage("Find \"" + searchString_ + "\": Selecting " + QString::number(foundIndices_.size()) + " Results");
}

void MainWindow::goToCurrentFoundIndex(QModelIndex index)
{
    ui_.RunDataTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

/*
 * UI
 */

void MainWindow::on_actionFind_triggered()
{
    QString textInput =
        QInputDialog::getText(this, tr("Find"), tr("Find in current run data (RB, user, title,...):"), QLineEdit::Normal);
    searchString_ = textInput;
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (textInput.isEmpty())
    {
        ui_.RunDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }

    // Find all occurrences of search string in table elements
    for (auto i = 0; i < runDataFilterProxy_.columnCount(); ++i)
    {
        auto location = ui_.RunDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_.RunDataTable->isColumnHidden(location))
            foundIndices_.append(runDataFilterProxy_.match(runDataFilterProxy_.index(0, location), Qt::DisplayRole, textInput,
                                                           -1, Qt::MatchContains));
    }

    // Select first match
    if (!foundIndices_.empty())
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_.RunDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

void MainWindow::on_actionFindNext_triggered() { findDown(); }
void MainWindow::on_actionFindPrevious_triggered() { findUp(); }
void MainWindow::on_actionSelectAllFound_triggered() { selectAllSearches(); }
