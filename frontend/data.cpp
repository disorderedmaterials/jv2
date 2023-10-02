// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

// Fills cycles box on request completion
void MainWindow::handle_result_instruments(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    QString msg;
    if (worker->errorType == QNetworkReply::NoError)
    {
        auto response = worker->response;

        cyclesMenu_->clear();
        cyclesMap_.clear();
        QJsonValue value;
        for (auto i = worker->jsonArray.count() - 1; i >= 0; i--)
        {
            value = worker->jsonArray[i];
            // removes header_ file
            if (value.toString() != "journal.xml")
            {
                auto displayName =
                    "Cycle " + value.toString().split("_")[1] + "/" + value.toString().split("_")[2].remove(".xml");
                cyclesMap_[displayName] = value.toString();

                auto *action = new QAction(displayName, this);
                connect(action, &QAction::triggered, [=]() { changeCycle(displayName); });
                cyclesMenu_->addAction(action);
            }
        }

        if (init_)
        {
            // Sets cycle to most recently viewed
            recentCycle();
            init_ = false;
            return;
        }
        // Keep cycle over instruments
        for (QAction *action : cyclesMenu_->actions())
        {
            if (action->text() == ui_.cycleButton->text())
            {
                action->trigger();
                return;
            }
        }
        cyclesMenu_->actions()[0]->trigger();
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->errorString;
        QMessageBox::information(this, "", msg);
    }
}

// Fills table view with run
void MainWindow::handle_result_cycles(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    QString msg;

    if (worker->errorType == QNetworkReply::NoError)
    {
        if (worker->response == "\"invalid source\"\n")
        {
            statusBar()->showMessage("invalid source");
            validSource_ = false;
            return;
        }
        else
            validSource_ = true;
        // Error handling
        if (ui_.GroupRunsButton->isChecked())
            ui_.GroupRunsButton->setChecked(false);

        // Get desired fields and titles from config files
        desiredHeader_ = getFields(instName_, instType_);
        auto jsonArray = worker->jsonArray;
        auto jsonObject = jsonArray.at(0).toObject();
        // Add columns to header and give titles where applicable
        header_.clear();
        foreach (const QString &key, jsonObject.keys())
        {
            if (headersMap_[key].isEmpty())
                headersMap_[key] = key;
            // Find matching indices
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [key](const auto &data) { return data.first == key; });
            if (it != desiredHeader_.end())
                header_.push_back(JsonTableModel::Heading({{"title", it->second}, {"index", key}}));
            else
                header_.push_back(JsonTableModel::Heading({{"title", headersMap_[key]}, {"index", key}}));
        }

        // Sets and fills table data
        model_ = new JsonTableModel(header_, this);
        proxyModel_ = new JSONTableFilterProxy(this);
        proxyModel_->setSourceModel(model_);
        connect(ui_.FilterCaseSensitivityButton, SIGNAL(clicked(bool)), proxyModel_, SLOT(toggleCaseSensitivity(bool)));
        connect(proxyModel_, &JSONTableFilterProxy::updateFilter,
                [=]() { on_RunFilterEdit_textChanged(ui_.RunFilterEdit->text()); }); // refresh filter on toggle
        ui_.runDataTable->setModel(proxyModel_);
        model_->setJson(jsonArray);
        ui_.runDataTable->show();

        // Fills viewMenu_ with all columns
        viewMenu_->clear();
        viewMenu_->addAction("Save column state", this, SLOT(savePref()));
        viewMenu_->addAction("Reset column state to default", this, SLOT(clearPref()));
        viewMenu_->addSeparator();
        foreach (const QString &key, jsonObject.keys())
        {

            auto *checkBox = new QCheckBox(viewMenu_);
            auto *checkableAction = new QWidgetAction(viewMenu_);
            checkableAction->setDefaultWidget(checkBox);
            checkBox->setText(headersMap_[key]);
            checkBox->setCheckState(Qt::PartiallyChecked);
            viewMenu_->addAction(checkableAction);
            connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(columnHider(int)));

            // Filter table based on desired headers
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [key](const auto &data) { return data.first == key; });
            // If match found
            if (it != desiredHeader_.end())
                checkBox->setCheckState(Qt::Checked);
            else
                checkBox->setCheckState(Qt::Unchecked);
        }
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_.runDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_.runDataTable->horizontalHeader()->logicalIndex(j);
                // If index matches model data, swap columns in view
                if (desiredHeader_[i].first == model_->headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
                {
                    ui_.runDataTable->horizontalHeader()->swapSections(j, i);
                }
            }
        }
        ui_.runDataTable->resizeColumnsToContents();
        updateSearch(searchString_);
        ui_.RunFilterEdit->clear();
        emit tableFilled();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->errorString;
        QMessageBox::information(this, "", msg);
    }
}

// Update cycles list when Instrument changed
void MainWindow::currentInstrumentChanged(const QString &arg1)
{
    cachedMassSearch_.clear();

    // Configure api call
    QString url_str = "http://127.0.0.1:5000/getCycles/" + arg1;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);

    // Call result handler when request completed
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_instruments(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Populate table with cycle data
void MainWindow::changeCycle(QString value)
{
    if (value[0] == '[')
    {
        auto it = std::find_if(cachedMassSearch_.begin(), cachedMassSearch_.end(),
                               [value](const auto &tuple) { return std::get<1>(tuple) == value.mid(1, value.length() - 2); });
        if (it != cachedMassSearch_.end())
        {
            ui_.cycleButton->setText(value);
            setLoadScreen(true);
            handle_result_cycles(std::get<0>(*it));
        }
        return;
    }
    ui_.cycleButton->setText(value);

    QString url_str = "http://127.0.0.1:5000/getJournal/" + instName_ + "/" + cyclesMap_[value];
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);

    // Call result handler when request completed
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handle_result_cycles(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (cyclesMenu_->actions().count() == 0)
        QWidget::setEnabled(false);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentCycle = settings.value("recentCycle").toString();
    // Sets cycle to last used/ most recent if unavailable
    for (QAction *action : cyclesMenu_->actions())
    {
        if (action->text() == recentCycle)
        {
            action->trigger();
            return;
        }
    }
    cyclesMenu_->actions()[0]->trigger();
}

// Fill instrument list
void MainWindow::fillInstruments(QList<std::tuple<QString, QString, QString>> instruments)
{
    // Only allow calls after initial population
    instrumentsMenu_ = new QMenu("instrumentsMenu");
    cyclesMenu_ = new QMenu("cyclesMenu");

    connect(ui_.instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_.instrumentButton->mapToGlobal(QPoint(0, ui_.instrumentButton->height()))); });
    connect(ui_.cycleButton, &QPushButton::clicked,
            [=]() { cyclesMenu_->exec(ui_.cycleButton->mapToGlobal(QPoint(0, ui_.cycleButton->height()))); });
    foreach (const auto instrument, instruments)
    {
        auto *action = new QAction(std::get<2>(instrument), this);
        connect(action, &QAction::triggered, [=]() { changeInst(instrument); });
        instrumentsMenu_->addAction(action);
    }
}

// Handle Instrument selection
void MainWindow::changeInst(std::tuple<QString, QString, QString> instrument)
{
    instType_ = std::get<1>(instrument);
    instName_ = std::get<0>(instrument);
    instDisplayName_ = std::get<2>(instrument);
    ui_.instrumentButton->setText(instDisplayName_);
    currentInstrumentChanged(instName_);
}

void MainWindow::checkForUpdates()
{
    QString url_str = "http://127.0.0.1:5000/pingCycle/" + instName_;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { refresh(workerProxy->response); });
    worker->execute(input);
}

void MainWindow::refresh(QString status)
{
    if (status != "")
    {
        qDebug() << "Update";
        currentInstrumentChanged(instName_);
        if (cyclesMap_[cyclesMenu_->actions()[0]->text()] != status) // if new cycle found
        {
            auto displayName = "Cycle " + status.split("_")[1] + "/" + status.split("_")[2].remove(".xml");
            cyclesMap_[displayName] = status;

            auto *action = new QAction(displayName, this);
            connect(action, &QAction::triggered, [=]() { changeCycle(displayName); });
            cyclesMenu_->insertAction(cyclesMenu_->actions()[0], action);
        }
        else if (cyclesMap_[ui_.cycleButton->text()] == status) // if current opened cycle changed
        {
            QString url_str = "http://127.0.0.1:5000/updateJournal/" + instName_ + "/" + status + "/" +
                              model_->getJsonObject(model_->index(model_->rowCount() - 1, 0))["run_number"].toString();
            HttpRequestInput input(url_str);
            auto *worker = new HttpRequestWorker(this);
            connect(worker, &HttpRequestWorker::on_execution_finished,
                    [=](HttpRequestWorker *workerProxy) { update(workerProxy); });
            worker->execute(input);
        }
    }
    else
    {
        qDebug() << "no change";
        return;
    }
}

void MainWindow::update(HttpRequestWorker *worker)
{
    for (auto row : worker->jsonArray)
    {
        auto rowObject = row.toObject();
        model_->insertRows(model_->rowCount(), 1);
        auto index = model_->index(model_->rowCount() - 1, 0);
        model_->setData(index, rowObject);
    }
}

void MainWindow::refreshTable()
{
    for (auto i = 0; i < model_->columnCount(); ++i)
    {
        ui_.runDataTable->setColumnHidden(i, true);
    }
    currentInstrumentChanged(instName_);
}
