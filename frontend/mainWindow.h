// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "backend.h"
#include "genericTreeModel.h"
#include "httpRequestWorker.h"
#include "instrumentModel.h"
#include "journalModel.h"
#include "journalSource.h"
#include "journalSourceModel.h"
#include "lock.h"
#include "runDataFilterProxy.h"
#include "runDataModel.h"
#include "ui_mainWindow.h"
#include <QChart>
#include <QCheckBox>
#include <QDomDocument>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QTimer>

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
    // Whether UI controls are currently being updated with new data
    Lock controlsUpdating_;
    // Main backend class
    Backend backend_;
    // Journal auto update timer
    QTimer journalAutoUpdateTimer_;

    private:
    // Update the UI accordingly for the current source, updating its state if required
    void updateForCurrentSource(std::optional<JournalSource::JournalSourceState> newState = {});
    // Update the error page
    void setErrorPage(const QString &errorTitle, const QString &errorText);

    private slots:
    void removeTab(int index);
    // Notification point for backend startup
    void backendStarted(const QString &result);
    // Ping backend to see if it's ready
    void waitForBackend();
    // Prepare initial state once the backend is ready
    void prepare();

    protected:
    void closeEvent(QCloseEvent *event) override;

    /*
     * Journal Sources
     */
    private:
    // Known journal sources
    std::vector<std::unique_ptr<JournalSource>> journalSources_;
    // Currently selected journal source (if any)
    JournalSource *currentJournalSource_;
    // Model for available journal sources
    JournalSourceModel journalSourceModel_;
    // Model for available journals
    JournalModel journalModel_;

    private:
    // Set up standard journal sources
    void setUpStandardJournalSources(QCommandLineParser &cliParser);
    // Find the specified journal source
    JournalSource *findJournalSource(const QString &name);
    // Set current journal source
    void setCurrentJournalSource(JournalSource *source, std::optional<QString> goToJournal = {});
    // Return current journal source
    JournalSource *currentJournalSource() const;
    // Return selected journal in current source (assuming one is selected)
    Journal &currentJournal() const;

    private slots:
    void on_JournalSourceComboBox_currentIndexChanged(int index);
    void on_JournalComboBox_currentIndexChanged(int index);
    void on_JournalComboBackToJournalsButton_clicked(bool checked);
    void on_actionEditSources_triggered();

    private:
    // Handle returned journal information for an instrument
    void handleListJournals(HttpRequestWorker *worker, std::optional<QString> journalToLoad = {});
    // Handle get journal updates result
    void handleGetJournalUpdates(HttpRequestWorker *workers);
    // Handle jump to journal
    void handleJumpToJournal(HttpRequestWorker *worker);

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
    // Find instrument with supplied name
    OptionalReferenceWrapper<const Instrument> findInstrument(const QString &name) const;

    private slots:
    void on_InstrumentComboBox_currentIndexChanged(int index);
    // Return current instrument from active source
    OptionalReferenceWrapper<const Instrument> currentInstrument() const;

    /*
     * Run Data
     */
    private:
    QJsonArray runData_, groupedRunData_;
    RunDataModel runDataModel_;
    RunDataFilterProxy runDataFilterProxy_;
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
    void highlightRunNumber(int runNumber);

    private slots:
    void on_actionRefreshJournal_triggered();
    void on_actionJumpTo_triggered();
    // Run data context menu requested
    void runDataContextMenuRequested(QPoint pos);

    private:
    // Handle run data returned for a whole journal
    void handleCompleteJournalRunData(HttpRequestWorker *worker, std::optional<int> runNumberToHighlight = {});

    /*
     * Journal Generation
     */
    private:
    // Current source being generated (if any)
    JournalSource *sourceBeingGenerated_{nullptr};
    // Map of sort keys to run data files
    std::map<QString, std::vector<QString>> scannedFiles_;

    private:
    // Model for scanned journal files
    GenericTreeModel generatorScannedFilesModel_;
    // Update journal generation page for specified source
    void updateGenerationPage(int nCompleted, const QString &lastFileProcessed);

    private slots:
    void on_GeneratingCancelButton_clicked(bool checked);

    private:
    // Handle returned directory list result
    void handleGenerateList(HttpRequestWorker *worker);
    // Handle / monitor the generation background scan
    void handleGenerateBackgroundScan();
    // Handle journal generation finalisation
    void handleGenerateFinalise(HttpRequestWorker *worker);
    // Handle journal generation background scan termination
    void handleGenerateBackgroundScanStop(HttpRequestWorker *worker);

    /*
     * Network Handling
     */
    private:
    // Perform error check on http result
    bool networkRequestHasError(HttpRequestWorker *worker, const QString &taskDescription);

    /*
     * Settings
     */
    private:
    // Save custom column settings
    void saveCustomColumnSettings() const;
    // Store recent journal settings
    void storeRecentJournalSettings() const;
    // Get recent journal settings
    std::optional<QString> getRecentJournalSettings();
    // Store user-defined journal sources
    void storeUserJournalSources() const;
    // Get user-defined journal sources
    void getUserJournalSources();

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
     * Search Everywhere
     */
    private slots:
    void on_actionSearchEverywhere_triggered();

    private:
    // Handle search result
    void handleSearchResult(HttpRequestWorker *worker);

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

    // Normalisation options
    void muAmps(QString runs, bool checked, QString);
    void runDivide(QString currentDetector, QString run, bool checked);
    void monDivide(QString currentRun, QString mon, bool checked);
};
