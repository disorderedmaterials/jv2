// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "backend.h"
#include "httpRequestWorker.h"
#include "instrumentModel.h"
#include "journalModel.h"
#include "journalSource.h"
#include "jsonTableFilterProxy.h"
#include "jsonTableModel.h"
#include "ui_mainWindow.h"
#include <QChart>
#include <QCheckBox>
#include <QDomDocument>
#include <QMainWindow>
#include <QSortFilterProxyModel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
    MainWindow(QCommandLineParser &cliParser);
    ~MainWindow() = default;

    /*
     * UI
     */
    private:
    Ui::MainWindow ui_;
    bool init_;
    // Whether UI controls are currently being updated with new data
    bool controlsUpdating_{false};
    // Main backend class
    Backend backend_;

    private:
    // Update the UI accordingly for the current source, updating its state if required
    void updateForCurrentSource(std::optional<JournalSource::JournalSourceState> newState = {});

    private slots:
    void removeTab(int index);
    // Notification point for backend startup
    void backendStarted(const QString &result);
    // Ping backend to see if it's ready
    void waitForBackend();

    protected:
    void closeEvent(QCloseEvent *event);

    /*
     * Journal Sources
     */
    private:
    // Known journal sources
    std::vector<JournalSource> journalSources_;
    // Currently selected journal source (if any)
    OptionalReferenceWrapper<JournalSource> currentJournalSource_;
    // Model for available journals
    JournalModel journalModel_;

    private:
    // Parse journal source from specified source
    bool parseJournalSources(const QDomDocument &source);
    // Get default journal sources
    void getDefaultJournalSources();
    // Set current journal source
    void setCurrentJournalSource(std::optional<QString> optName);
    // Return current journal source
    JournalSource &currentJournalSource() const;
    // Return selected journal in current source (assuming one is selected)
    Journal &currentJournal() const;

    private slots:
    void on_JournalSourceComboBox_currentIndexChanged(int index);
    void on_JournalComboBox_currentIndexChanged(int index);

    private:
    // Handle get journal updates result
    void handleGetJournalUpdates(HttpRequestWorker *workers);
    // Handle returned journal information for an instrument
    void handleListJournals(HttpRequestWorker *worker);

    /*
     * Instruments
     */
    private:
    // Available instruments
    std::vector<Instrument> instruments_;
    // Model for instruments
    InstrumentModel instrumentModel_;

    private:
    // Parse instruments from specified source
    bool parseInstruments(const QDomDocument &source);
    // Get default instrument complement
    void getDefaultInstruments();

    private slots:
    void on_InstrumentComboBox_currentIndexChanged(int index);
    // Return current instrument from active source
    OptionalReferenceWrapper<const Instrument> currentInstrument() const;

    /*
     * Run Data
     */
    private:
    QJsonArray runData_, groupedRunData_;
    JsonTableModel runDataModel_;
    JsonTableFilterProxy runDataFilterProxy_;
    Instrument::RunDataColumns runDataColumns_, groupedRunDataColumns_;

    private:
    // Clear all run data
    void clearRunData();
    // Get data for specified run number
    std::optional<QJsonObject> dataForRunNumber(int runNumber) const;
    // Generate grouped run data from current run data
    void generateGroupedData();
    // Return the run data model index under the mouse, accounting for the effects of the filter proxys
    const QModelIndex runDataIndexAtPos(const QPoint pos) const;
    // Return integer list of currently-selected run numbers
    std::vector<int> selectedRunNumbers() const;
    // Select and show specified run number in table (if it exists)
    bool highlightRunNumber(int runNumber);

    private slots:
    void on_actionRefresh_triggered();
    void on_actionJumpTo_triggered();

    // Run data context menu requested
    void runDataContextMenuRequested(QPoint pos);

    /*
     * Journal Generation
     */
    private:
    // Handle returned directory list result
    void handleListDataDirectory(const JournalSource &source, HttpRequestWorker *worker);
    // Handle returned journal generation result
    void handleScanResult(const JournalSource &source, HttpRequestWorker *worker);

    /*
     * Network Handling
     */
    private:
    // Perform error check on http result
    bool networkRequestHasError(HttpRequestWorker *worker, const QString &taskDescription);
    // Handle backend ping result
    void handleBackendPingResult(HttpRequestWorker *worker);
    // Handle run data returned for a whole journal
    void handleCompleteJournalRunData(HttpRequestWorker *worker);
    // Handle jump to specified run number
    void handleSelectRunNoInCycle(HttpRequestWorker *worker, int runNumber);

    /*
     * Settings
     */
    private:
    // Save custom column settings
    void saveCustomColumnSettings() const;
    // Retrieve user settings
    void loadSettings();

    /*
     * Find in Current Journal
     */
    private:
    QString searchString_;
    QModelIndexList foundIndices_;
    int currentFoundIndex_;

    private:
    void updateSearch(const QString &arg1);
    void findUp();
    void findDown();
    void selectAllSearches();
    void goToCurrentFoundIndex(QModelIndex index);

    private slots:
    void on_actionFind_triggered();
    void on_actionFindNext_triggered();
    void on_actionFindPrevious_triggered();
    void on_actionSelectAllFound_triggered();

    /*
     * Filtering
     */
    private slots:
    void on_RunFilterEdit_textChanged(const QString &arg1);
    void on_RunFilterCaseSensitivityButton_clicked(bool checked);
    void on_RunFilterClearButton_clicked(bool checked);
    void on_GroupRunsButton_clicked(bool checked);

    /*
     * Search in Source
     */
    private:
    // Cached mass search results
    QList<std::tuple<HttpRequestWorker *, QString>> cachedMassSearch_;

    private:
    // Perform mass search across cycles
    void massSearch(QString name, QString value);

    private slots:
    void on_actionSearchEverywhere_triggered();

    /*
     * Visualisation
     */
    private:
    // Handle extracted SE log values for plotting
    void handlePlotSELogValue(HttpRequestWorker *worker);
    // Handle plotting of SE log data
    void handleCreateSELogPlot(HttpRequestWorker *worker);

    /*
     * Nexus Interaction Stuff To Be Organised
     */
    private slots:
    void toggleAxis(int state);
    void getField();
    void showStatus(qreal x, qreal y, QString title);

    void handleSpectraCharting(HttpRequestWorker *worker);
    void handleMonSpectraCharting(HttpRequestWorker *worker);
    void plotSpectra(HttpRequestWorker *count);
    void plotMonSpectra(HttpRequestWorker *count);
    void getSpectrumCount();
    void getMonitorCount();

    // Normalisation options
    void muAmps(QString runs, bool checked, QString);
    void runDivide(QString currentDetector, QString run, bool checked);
    void monDivide(QString currentRun, QString mon, bool checked);
};
