// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

// Gathers all fields based on selected runs
void MainWindow::on_graph_clicked()
{
    // Clear related fields
    chart_->removeAllSeries();
    runsMenu_->clear();
    nexusMenu_->clear();

    QModelIndexList selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColum;
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;

    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");

        // Fills runsMenu_ with all runNo's
        QAction *action = new QAction(runNo, this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()), this, SLOT(runToggled()));
        runsMenu_->addAction(action);
    }
    // Removes final ";"
    runNos.chop(1);

    QString url_str = "http://127.0.0.1:5000/getNexusFields/";
    QString cycle = ui_->cyclesBox->currentText().replace("journal", "cycle").replace(".xml", "");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos;

    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_fieldQuery(HttpRequestWorker *)));
    worker->execute(input);
}

// Fills field menu
void MainWindow::handle_result_fieldQuery(HttpRequestWorker *worker)
{
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        nexusMenu_->clear();
        foreach (const QJsonValue &value, worker->json_array)
        {
            // Fills nexusMenu_ with all columns
            QAction *action = new QAction(value.toString(), this);
            action->setCheckable(true);
            connect(action, SIGNAL(triggered()), this, SLOT(fieldToggled()));
            nexusMenu_->addAction(action);
        }
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

// Handles field selection
void MainWindow::fieldToggled()
{
    // Gets signal object
    auto *fieldAction = qobject_cast<QAction *>(sender());

    // Gathers all selected runs
    QString runNos = "";
    for (QAction *runAction : runsMenu_->actions())
    {
        if (runAction->isChecked() == true)
            runNos.append(runAction->text() + ";");
    }
    runNos.chop(1);

    if (runNos.size() == 0)
        return;

    if (fieldAction->isChecked() == false)
    {
        for (auto series : chart_->series())
        {
            if (series->name().contains(fieldAction->text().replace("/", ":")))
            {
                chart_->removeSeries(series);
            }
        }
        chart_->createDefaultAxes();
    }
    else
    {
        QString url_str = "http://127.0.0.1:5000/getNexusData/";
        QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");
        QString field = fieldAction->text().replace("/", ":");
        url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos + "/" + field;

        HttpRequestInput input(url_str);
        HttpRequestWorker *worker = new HttpRequestWorker(this);
        connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handle_result_logData(HttpRequestWorker *)));
        worker->execute(input);
    }
}

// Handles run selection
void MainWindow::runToggled()
{
    auto *runAction = qobject_cast<QAction *>(sender());

    // Gathers selected fields
    QString fields = "";
    for (QAction *fieldAction : nexusMenu_->actions())
    {
        if (fieldAction->isChecked() == true)
            fields.append(fieldAction->text().replace("/", ":") + ";");
    }
    // removes end ";"
    fields.chop(1);

    if (fields.size() == 0)
        return;

    if (runAction->isChecked() == false)
    {
        for (auto series : chart_->series())
        {
            if (series->name().contains(runAction->text()))
            {
                chart_->removeSeries(series);
            }
        }
        chart_->createDefaultAxes();
    }
    else
    {
        QString url_str = "http://127.0.0.1:5000/getNexusData/";
        QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");

        url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runAction->text() + "/" + fields;
        HttpRequestInput input(url_str);
        HttpRequestWorker *worker = new HttpRequestWorker(this);
        connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handle_result_logData(HttpRequestWorker *)));
        worker->execute(input);
    }
}

// Handles log data
void MainWindow::handle_result_logData(HttpRequestWorker *worker)
{
    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        // For each Run
        foreach (const QJsonValue &runFields, worker->json_array)
        {
            QJsonArray runFieldsArray = runFields.toArray();

            // For each field
            foreach (const QJsonValue &fieldData, runFieldsArray)
            {
                QJsonArray fieldDataArray = fieldData.toArray();

                // For each plot point
                QLineSeries *series = new QLineSeries();

                // Set series ID
                QString name = fieldDataArray.first()[0].toString() + " " + fieldDataArray.first()[1].toString();
                series->setName(name);
                fieldDataArray.removeFirst();
                foreach (const QJsonValue &dataPair, fieldDataArray)
                {
                    QJsonArray dataPairArray = dataPair.toArray();
                    series->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
                }
                chart_->addSeries(series);
            }
        }
        // Resize chart
        chart_->createDefaultAxes();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::customMenuRequested(QPoint pos)
{
    QMessageBox::information(this, "", "Requested");

    QModelIndex index = ui_->runDataTable->indexAt(pos);
    QModelIndexList selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColum;
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;

    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");
    }
    // Removes final ";"
    runNos.chop(1);

    QMessageBox::information(this, "", "Clear menu");
    contextMenu_->clear();
    QMessageBox::information(this, "", "Add default action");
    QAction *action = new QAction("why", this);
    contextMenu_->addAction(action);
    QMessageBox::information(this, "", "Show Menu");
    contextMenu_->exec(pos);

    QString url_str = "http://127.0.0.1:5000/getNexusFields/";
    QString cycle = ui_->cyclesBox->currentText().replace("journal", "cycle").replace(".xml", "");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos;

    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextMenu(HttpRequestWorker *)));
    worker->execute(input);
}

// Fills field menu
void MainWindow::handle_result_contextMenu(HttpRequestWorker *worker)
{
    QMessageBox::information(this, "", "Handle result");

    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        QMessageBox::information(this, "", "Fill menu");
        foreach (const QJsonValue &value, worker->json_array)
        {
            // Fills contextMenu with all columns
            QAction *action = new QAction(value.toString(), this);
            connect(action, SIGNAL(triggered()), this, SLOT(contextGraph()));
            contextMenu_->addAction(action);
        }
        QMessageBox::information(this, "", "menu Filled");
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::contextGraph()
{
    QMessageBox::information(this, "", "Graph");

    // Gets signal object
    auto *contextAction = qobject_cast<QAction *>(sender());
    QMessageBox::information(this, "", "Action got");

    // Gathers all selected runs
    QModelIndexList selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();
    // Finds run number location in table
    int runNoColum;
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }
    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;
    // Concats runs
    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");
    }
    // Removes final ";"
    runNos.chop(1);
    // Error handling
    if (runNos.size() == 0)
        return;

    QMessageBox::information(this, "", "Get log data");
    QString url_str = "http://127.0.0.1:5000/getNexusData/";
    QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");
    QString field = contextAction->text().replace("/", ":");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos + "/" + field;

    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextGraph(HttpRequestWorker *)));
    worker->execute(input);
}

// Handles log data
void MainWindow::handle_result_contextGraph(HttpRequestWorker *worker)
{
    QMessageBox::information(this, "", "Data got");

    QChartView *contextChartView = new QChartView();
    ui_->tabWidget->addTab(contextChartView, "graph");
    QChart *contextChart = new QChart();
    contextChartView->setChart(contextChart);
    QMessageBox::information(this, "", "Tab created");

    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        // For each Run
        foreach (const QJsonValue &runFields, worker->json_array)
        {
            QJsonArray runFieldsArray = runFields.toArray();

            // For each field
            foreach (const QJsonValue &fieldData, runFieldsArray)
            {
                QJsonArray fieldDataArray = fieldData.toArray();

                // For each plot point
                QLineSeries *series = new QLineSeries();

                // Set series ID
                QString name = fieldDataArray.first()[0].toString() + " " + fieldDataArray.first()[1].toString();
                series->setName(name);
                fieldDataArray.removeFirst();
                foreach (const QJsonValue &dataPair, fieldDataArray)
                {
                    QJsonArray dataPairArray = dataPair.toArray();
                    series->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
                }
                contextChart->addSeries(series);
            }
        }
        QMessageBox::information(this, "", "Chart filled");
        // Resize chart
        contextChart->createDefaultAxes();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}