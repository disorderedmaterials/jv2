// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "ui_mainWindow.h"
#include "version.h"
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QCommandLineParser &cliParser) : QMainWindow(), backend_(cliParser), runDataFilterProxy_(runDataModel_)
{
    ui_.setupUi(this);

    Locker updateLocker(controlsUpdating_);

    // Set the window title
    setWindowTitle(QString("JournalViewer 2 (v%1)").arg(JV2VERSION));

    // Set up standard journal sources
    setUpStandardJournalSources();
    journalSourceModel_.setData(journalSources_);

    // Get default instrument run data columns
    Instrument::getDefaultColumns();

    // Get available instrument data
    getDefaultInstruments();
    instrumentModel_.setData(instruments_);

    // Define initial variable state
    searchString_ = "";
    groupedRunDataColumns_.emplace_back("Run Numbers", "run_number");
    groupedRunDataColumns_.emplace_back("Title", "title");
    groupedRunDataColumns_.emplace_back("Total Duration", "duration");

    // Connect models
    ui_.JournalSourceComboBox->setModel(&journalSourceModel_);
    ui_.InstrumentComboBox->setModel(&instrumentModel_);
    ui_.JournalComboBox->setModel(&journalModel_);

    // Set up the main data table
    ui_.RunDataTable->setModel(&runDataFilterProxy_);
    // -- Allow re-arranging of table columns
    ui_.RunDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_.RunDataTable->horizontalHeader()->setDragEnabled(true);
    ui_.RunDataTable->setAlternatingRowColors(true);
    ui_.RunDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");
    ui_.RunDataTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    // -- Context menu
    ui_.RunDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.RunDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(runDataContextMenuRequested(QPoint)));

    // Disables closing data tab + handles tab closing
    ui_.MainTabs->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_.MainTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Set up and connect the journal update timer and make sure it is currently stopped
    journalAutoUpdateTimer_.setInterval(30000);
    journalAutoUpdateTimer_.stop();
    connect(&journalAutoUpdateTimer_, &QTimer::timeout, [=]() { on_actionRefresh_triggered(); });

    // Connect exit action
    connect(ui_.actionQuit, SIGNAL(triggered()), this, SLOT(close()));

    // Start the backend - this will notify backendStarted when complete, but we still need to wait for the server to come up
    connect(&backend_, SIGNAL(started(const QString &)), this, SLOT(backendStarted(const QString &)));
    backend_.start();
}

/*
 * UI
 */

// Update the UI accordingly for the current source, updating its state if required
void MainWindow::updateForCurrentSource(std::optional<JournalSource::JournalSourceState> newState)
{
    Locker updateLocker(controlsUpdating_);

    // Do we actually have a current source?
    if (!currentJournalSource_)
    {
        ui_.InstrumentComboBox->setEnabled(false);
        ui_.JournalComboBox->setEnabled(false);
        journalModel_.setData(std::nullopt);

        setErrorPage("No Journal Source", "There is no current journal source set, so nothing to display.");
        ui_.MainStack->setCurrentIndex(JournalSource::JournalSourceState::Error);

        journalAutoUpdateTimer_.stop();

        return;
    }

    auto &source = currentJournalSource_->get();
    if (newState)
        source.setState(*newState);

    ui_.JournalSourceComboBox->setCurrentText(source.name());

    // Set the current instrument
    if (source.instrumentRequired() && source.currentInstrument())
        ui_.InstrumentComboBox->setCurrentText(source.currentInstrument()->get().name());
    else
        ui_.InstrumentComboBox->setCurrentIndex(-1);

    // Set relevant controls
    ui_.InstrumentComboBox->setEnabled(source.instrumentRequired());
    if (source.currentJournal())
        ui_.JournalComboBox->setCurrentText(source.currentJournal()->get().name());
    else
        ui_.JournalComboBox->setCurrentIndex(-1);

    // Set the journal combo stack page according to the current source search state
    ui_.JournalComboStack->setCurrentIndex(source.showingSearchedData() ? 1 : 0);

    // Set the main stack page to correspond to the state enum
    ui_.MainStack->setCurrentIndex(source.state());

    // If the source state is viewing normal run data and it is a Network source, enable the auto-update timer
    if (source.state() == JournalSource::JournalSourceState::OK && !source.showingSearchedData() &&
        source.type() == JournalSource::IndexingType::Network)
        journalAutoUpdateTimer_.start();
    else
        journalAutoUpdateTimer_.stop();
}

// Update the error page
void MainWindow::setErrorPage(const QString &errorTitle, const QString &errorText)
{
    ui_.ErrorLabel->setText(errorTitle);
    ui_.ErrorInfoLabel->setText(errorText);
    ui_.ErrorRetryButton->setEnabled(false);
}

void MainWindow::removeTab(int index) { delete ui_.MainTabs->widget(index); }

/*
 * Window
 */

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update recent journal settings
    storeRecentJournalSettings();

    // Shut down backend
    backend_.stop();

    event->accept();
}

// Notification point for backend startup having completed
void MainWindow::backendStarted(const QString &result)
{
    if (result == "OK")
        waitForBackend();
    else
        QMessageBox::warning(this, "Error Starting Backend",
                             "The backend failed to start.\nThe error message received was: " + result);
}

// Ping backend to see if it's ready
void MainWindow::waitForBackend()
{
    // Set max number of pings to attempt
    static int pingsRemaining = 5;

    --pingsRemaining;

    if (pingsRemaining < 0)
    {
        QMessageBox::warning(this, "Backend Error", "Can't connect to the backend - giving up!");
        return;
    }

    // Create a timer to ping the backend after 1000 ms
    auto *pingTimer = new QTimer;
    pingTimer->setSingleShot(true);
    pingTimer->setInterval(1000);
    connect(pingTimer, &QTimer::timeout,
            [=]()
            {
                backend_.ping(
                    [this](HttpRequestWorker *worker)
                    {
                        if (worker->response().contains("READY"))
                            prepare();
                        else
                            waitForBackend();
                    });
            });
    pingTimer->start();
}

// Prepare initial state once the backend is ready
void MainWindow::prepare()
{
    // Get recent journal settings - this will set directly the relevant data but not call the backend
    auto requestedJournal = getRecentJournalSettings();

    setCurrentJournalSource(currentJournalSource_, requestedJournal);
}
