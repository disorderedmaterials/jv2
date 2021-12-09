// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QWidgetAction>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    initialiseElements();
}

MainWindow::~MainWindow() { delete ui_; }

// Configure initial application state
void MainWindow::initialiseElements()
{
    fillInstruments();

    // First Iteration variable for set-up commands
    init_ = true;

    // View menu for column toggles
    viewMenu_ = ui_->menubar->addMenu("View");

    // Temporary nexus data menu
    nexusMenu_ = ui_->menubar->addMenu("Nexus");
    runsMenu_ = ui_->menubar->addMenu("Runs");

    // Allows re-arranging of table columns
    ui_->runDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_->runDataTable->horizontalHeader()->setDragEnabled(true);
    ui_->runDataTable->setAlternatingRowColors(true);
    ui_->runDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");

    // Default heading stuff
    neutronHeader_.append({"run_number", "title", "start_time", "duration", "proton_charge", "user_name"});
    muonHeader_.append({"run_number", "title", "start_time", "duration", "total_mevents", "user_name"});

    // Sets instrument to last used
    QSettings settings;
    QString recentInstrument = settings.value("recentInstrument").toString();
    auto instrumentIndex = ui_->instrumentsBox->findText(recentInstrument);
    if (instrumentIndex != -1)
        ui_->instrumentsBox->setCurrentIndex(instrumentIndex);
    else
        ui_->instrumentsBox->setCurrentIndex(ui_->instrumentsBox->count() - 1);

    // Creates Chart
    chart_ = new QChart;
    ui_->chartView->setChart(chart_);

    // Creates graph toggle
    ui_->togglePage->addItem(tr("Data table"));
    ui_->togglePage->addItem(tr("Data graph"));
    connect(ui_->togglePage, QOverload<int>::of(&QComboBox::activated), ui_->stackedWidget, &QStackedWidget::setCurrentIndex);

    // Disables closing data tab + handles tab closing
    ui_->tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_->runDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->runDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (ui_->cyclesBox->count() == 0)
    {
        ui_->instrumentsBox->clear();
        QWidget::setEnabled(false);
    }
    QSettings settings;
    QString recentCycle = settings.value("recentCycle").toString();
    auto cycleIndex = ui_->cyclesBox->findText(recentCycle);

    // Sets cycle to last used/ most recent if unavailable
    if (ui_->instrumentsBox->currentText() != "")
    {
        if (cycleIndex != -1)
            ui_->cyclesBox->setCurrentIndex(cycleIndex);
        else if (ui_->cyclesBox->currentText() != "")
            ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
    }
    else
        ui_->cyclesBox->setEnabled(false);
    if (cycleIndex != -1)
        ui_->cyclesBox->setCurrentIndex(cycleIndex);
    else
        ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
}

// Fill instrument list
void MainWindow::fillInstruments()
{
    QList<QString> instruments = {"merlin neutron", "nimrod neutron", "sandals neutron", "iris neutron", "emu muon"};

    // Only allow calls after initial population
    ui_->instrumentsBox->blockSignals(true);
    ui_->instrumentsBox->clear();
    foreach (const QString instrument, instruments)
    {
        ui_->instrumentsBox->addItem(instrument.split(" ")[0], instrument.split(" ")[1]);
    }
    ui_->instrumentsBox->blockSignals(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings;
    settings.setValue("recentInstrument", ui_->instrumentsBox->currentText());
    settings.setValue("recentCycle", ui_->cyclesBox->currentText());

    // Close server
    QString url_str = "http://127.0.0.1:5000/shutdown";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);

    event->accept();
}

void MainWindow::on_massSearchButton_clicked()
{
    QString url_str = "http://127.0.0.1:5000/getAllJournals/"+ui_->instrumentsBox->currentText()+"/"+ui_->massSearchBox->text();
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handle_result_massSearch(HttpRequestWorker *)));
    worker->execute(input);
}

void MainWindow::handle_result_massSearch(HttpRequestWorker *worker)
{
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        // Get keys from json data
        auto jsonArray = worker->json_array;
        auto jsonObject = jsonArray.at(0).toObject();
        header_.clear();
        foreach (const QString &key, jsonObject.keys())
        {
            header_.push_back(JsonTableModel::Heading({{"title", key}, {"index", key}}));
        }

        // Sets and fills table data
        model_ = new JsonTableModel(header_, this);
        proxyModel_ = new QSortFilterProxyModel;
        proxyModel_->setSourceModel(model_);
        ui_->runDataTable->setModel(proxyModel_);
        model_->setJson(jsonArray);
        ui_->runDataTable->show();

        // Fills viewMenu_ with all columns
        viewMenu_->clear();
        foreach (const QString &key, jsonObject.keys())
        {

            QCheckBox *checkBox = new QCheckBox(viewMenu_);
            QWidgetAction *checkableAction = new QWidgetAction(viewMenu_);
            checkableAction->setDefaultWidget(checkBox);
            checkBox->setText(key);
            checkBox->setCheckState(Qt::PartiallyChecked);
            viewMenu_->addAction(checkableAction);
            connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(columnHider(int)));

            // Filter table based on desired headers
            if (!desiredHeader_.contains(key))
                checkBox->setCheckState(Qt::Unchecked);
            else
                checkBox->setCheckState(Qt::Checked);
        }
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}