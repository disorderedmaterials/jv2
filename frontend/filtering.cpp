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
        runDataModel_.groupData();
        for (auto i = 0; i < ui_.runDataTable->horizontalHeader()->count(); ++i)
            ui_.runDataTable->setColumnHidden(i, false);
        ui_.runDataTable->resizeColumnsToContents();
        // Make view match desired order
        ui_.runDataTable->horizontalHeader()->swapSections(ui_.runDataTable->horizontalHeader()->visualIndex(0), 0);
        ui_.runDataTable->horizontalHeader()->swapSections(ui_.runDataTable->horizontalHeader()->visualIndex(1), 1);
    }
    else
    {
        runDataModel_.unGroupData();
        for (auto i = 0; i < ui_.runDataTable->horizontalHeader()->count(); ++i)
        {
            auto index = runDataModel_.headerData(i, Qt::Horizontal, Qt::UserRole).toString();
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [index](const auto &data) { return data.first == index; });
            if (it == desiredHeader_.end())
                ui_.runDataTable->setColumnHidden(i, true);
        }
        // Re-sort columns on change
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_.runDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_.runDataTable->horizontalHeader()->logicalIndex(j);
                if (desiredHeader_[i].first == runDataModel_.headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
                    ui_.runDataTable->horizontalHeader()->swapSections(j, i);
            }
        }
        ui_.runDataTable->resizeColumnsToContents();
    }
    updateSearch(searchString_);
}

// Clears filter parameters
void MainWindow::on_RunFilterClearButton_clicked(bool checked) { ui_.RunFilterEdit->clear(); }

void MainWindow::goTo(HttpRequestWorker *worker, QString runNumber)
{
    setLoadScreen(false);
    QString msg;

    if (worker->errorType == QNetworkReply::NoError)
    {
        if (worker->response == "Not Found")
        {
            statusBar()->showMessage("Search query not found", 5000);
            return;
        }
        if (cyclesMap_[ui_.cycleButton->text()] == worker->response)
        {
            selectIndex(runNumber);
            return;
        }
        connect(this, &MainWindow::tableFilled, [=]() { selectIndex(runNumber); });
        for (auto i = 0; i < cyclesMenu_->actions().count(); i++)
        {
            if (cyclesMap_[cyclesMenu_->actions()[i]->text()] == worker->response)
                changeCycle(cyclesMenu_->actions()[i]->text());
        }
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->errorString;
        QMessageBox::information(this, "", msg);
    }
}

// Go-To run number
void MainWindow::on_actionRun_Number_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Find"), tr("Run No: "), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QString url_str = "http://127.0.0.1:5000/getGoToCycle/" + instName_ + "/" + textInput;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { goTo(workerProxy, textInput); });
    worker->execute(input);
    setLoadScreen(true);
}
