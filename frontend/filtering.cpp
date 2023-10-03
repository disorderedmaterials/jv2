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
        runDataModel_.setHorizontalHeaders(groupedTableHeaders_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        runDataModel_.setData(runData_);
        runDataModel_.setHorizontalHeaders(header_);

        for (auto i = 0; i < ui_.RunDataTable->horizontalHeader()->count(); ++i)
        {
            auto index = runDataModel_.headerData(i, Qt::Horizontal, Qt::UserRole).toString();
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [index](const auto &data) { return data.first == index; });
            if (it == desiredHeader_.end())
                ui_.RunDataTable->setColumnHidden(i, true);
        }

        // Re-sort columns on change
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_.RunDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_.RunDataTable->horizontalHeader()->logicalIndex(j);
                if (desiredHeader_[i].first == runDataModel_.headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
                    ui_.RunDataTable->horizontalHeader()->swapSections(j, i);
            }
        }
        ui_.RunDataTable->resizeColumnsToContents();
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
    if (!currentInstrument_)
        return;
    auto &inst = currentInstrument_->get();

    QString textInput = QInputDialog::getText(this, tr("Find"), tr("Run No: "), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QString url_str = "http://127.0.0.1:5000/getGoToCycle/" + inst.lowerCaseName() + "/" + textInput;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { goTo(workerProxy, textInput); });
    worker->execute(input);
    setLoadScreen(true);
}
