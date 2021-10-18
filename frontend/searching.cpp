#include "mainwindow.h"
#include <tuple>

// Search table data
void MainWindow::on_searchBox_textChanged(const QString &arg1)
{
    foundIndices.clear();
    currentFoundIndex = 0;
    if (arg1 == "")
    {
        ui->runDataTable->selectionModel()->clearSelection();
        return;
    }
    // Find all occurences of search string in table elements
    for (int i = 0; i < proxyModel->rowCount(); i++)
    {
        if (ui->runDataTable->isColumnHidden(i) == false)
        {
            foundIndices.append(proxyModel->match(proxyModel->index(0, i), Qt::DisplayRole, arg1, -1, Qt::MatchContains));
        }
    }
    // Select first match
    if (foundIndices.size() > 0)
    {
        goToCurrentFoundIndex(foundIndices[0]);
    }
}

// Select previous match
void MainWindow::on_findUp_clicked()
{
    // Boundary/ error handling
    if (foundIndices.size() > 0)
    {
        if (currentFoundIndex >= 1)
        {
            currentFoundIndex -= 1;
        }
        else
        {
            currentFoundIndex = 0;
        }
        goToCurrentFoundIndex(foundIndices[currentFoundIndex]);
    }
}

// Select next match
void MainWindow::on_findDown_clicked()
{
    // Boundary/ error handling
    if (foundIndices.size() > 0)
    {
        if (currentFoundIndex < foundIndices.size() - 1)
        {
            currentFoundIndex += 1;
        }
        goToCurrentFoundIndex(foundIndices[currentFoundIndex]);
    }
}

// Select all matches
void MainWindow::on_searchAll_clicked()
{
    // Error handling
    if (foundIndices.size() > 0)
    {
        ui->runDataTable->selectionModel()->clearSelection();
        currentFoundIndex = -1;
        for (int i = 0; i < foundIndices.size(); i++)
        {
            ui->runDataTable->selectionModel()->setCurrentIndex(foundIndices[i],
                                                                QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}
void MainWindow::goToCurrentFoundIndex(QModelIndex index)
{
    ui->runDataTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}