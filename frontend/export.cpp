// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

void MainWindow::exportRunDataAsText()
{
    // Save selection or all items?
    auto saveSelectionOnly =
        ui_.RunDataTable->selectionModel()->selection().count() > 0 &&
        QMessageBox::question(this, "Export as Text",
                              "There are selected items in the table - would you like to export just these?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;

    // Get a file name to save under
    static QDir currentDirectory;
    auto fileName = QFileDialog::getSaveFileName(this, "Save data as text file", currentDirectory.path(), {}, {},
                                                 QFileDialog::DontUseNativeDialog);
    if (fileName.isEmpty())
        return;

    // Save dir for later
    currentDirectory.setPath(fileName);

    // Open the file
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream textStream(&file);

    // Get selected runs (we might not use them)
    auto selectedRuns = ui_.RunDataTable->selectionModel()->selectedRows();
    auto nData = saveSelectionOnly ? selectedRuns.size() : runDataFilterProxy_.rowCount();
    for (auto i = 0; i < nData; ++i)
    {
        // Get the run number displayed in ith row or the ith selected model index
        auto runNo =
            runDataFilterProxy_.getData("run_number", saveSelectionOnly ? selectedRuns[i] : runDataFilterProxy_.index(i, 0));
        for (auto col = 0; col < runDataFilterProxy_.columnCount(); ++col)
        {
            textStream << runDataFilterProxy_
                              .data(runDataFilterProxy_.index(saveSelectionOnly ? selectedRuns[i].row() : i, col))
                              .toString()
                       << "  ";
        }
    }

    file.close();
}

/*
 * UI
 */

void MainWindow::on_actionExportAsText_triggered() { exportRunDataAsText(); }
