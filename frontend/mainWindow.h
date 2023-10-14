// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "backend.h"
#include "dataSource.h"
#include "httpRequestWorker.h"
#include "instrument.h"
#include "journal.h"
#include "jsonTableFilterProxy.h"
#include "jsonTableModel.h"
#include "locator.h"
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
    QMenu *instrumentsMenu_;
    QMenu *journalsMenu_;
    bool init_;
    // Main backend class
    Backend backend_;

    private slots:
    void setLoadScreen(bool state);
    void removeTab(int index);
    // Notification point for backend startup
    void backendStarted(const QString &result);
    // Ping backend to see if it's ready
    void waitForBackend();

    protected:
    void closeEvent(QCloseEvent *event);

    /*
     * Data Sources
     */
    private:
    // Known data sources
    std::vector<DataSource> dataSources_;
    // Currently selected instdata source (if any)
    OptionalReferenceWrapper<DataSource> currentDataSource_;

    private:
    // Parse data source from specified source
    bool parseDataSources(const QDomDocument &source);
    // Get default data sources
    void getDefaultDataSources();
    // Set current data source
    void setCurrentDataSource(QString name);
    // Return current data source
    const DataSource &currentDataSource() const;

    /*
     * Instruments
     */
    private:
    // Available instruments
    std::vector<Instrument> instruments_;
    // Currently selected instrument (if any)
    OptionalReferenceWrapper<Instrument> currentInstrument_;

    private:
    // Parse instruments from specified source
    bool parseInstruments(const QDomDocument &source);
    // Get default instrument complement
    void getDefaultInstruments();
    // Fill instrument list
    void fillInstruments();

    private slots:
    // Set current instrument
    void setCurrentInstrument(QString name);
    // Return current instrument
    const Instrument &currentInstrument() const;

    /*
     * Journals
     */
    private:
    // Available journals
    std::vector<Journal> journals_;
    // Currently selected journal (if any)
    OptionalReferenceWrapper<Journal> currentJournal_;

    private:
    // Add new journal
    Journal &addJournal(const QString &name, const Locator &location);
    // Find named journal
    OptionalReferenceWrapper<Journal> findJournal(const QString &name);
    // Set current journal being displayed
    void setCurrentJournal(QString name);
    void setCurrentJournal(Journal &journal);

    /*
     * Run Data
     */
    private:
    QJsonArray runData_, groupedRunData_;
    JsonTableModel runDataModel_;
    JsonTableFilterProxy runDataFilterProxy_;
    Instrument::RunDataColumns runDataColumns_, groupedRunDataColumns_;

    private:
    // Generate grouped run data from current run data
    void generateGroupedData();
    // Return the run data model index under the mouse, accounting for the effects of the filter proxys
    const QModelIndex runDataIndexAtPos(const QPoint pos) const;
    // Get selected run / cycle information [LEGACY, TO FIX]
    std::pair<QString, QString> selectedRunNumbersAndCycles() const;
    // Select and show specified run number in table (if it exists)
    bool highlightRunNumber(int runNumber);

    private slots:
    void on_actionRefresh_triggered();
    void on_actionJumpTo_triggered();

    // Run data context menu requested
    void runDataContextMenuRequested(QPoint pos);

    /*
     * Network Handling
     */
    private:
    // Perform error check on http result
    bool networkRequestHasError(HttpRequestWorker *worker, const QString &taskDescription);
    // Handle backend ping result
    void handleBackendPingResult(HttpRequestWorker *worker);
    // Handle get journal updates result
    void handleGetJournalUpdates(HttpRequestWorker *workers);
    // Handle returned journal information for an instrument
    void handleListJournals(HttpRequestWorker *worker);
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
     * Searching
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
     * Mass Search
     */
    private:
    // Cached mass search results
    QList<std::tuple<HttpRequestWorker *, QString>> cachedMassSearch_;

    private:
    // Perform mass search across cycles
    void massSearch(QString name, QString value);

    private slots:
    void on_actionMassSearchRBNo_triggered();
    void on_actionMassSearchTitle_triggered();
    void on_actionMassSearchUser_triggered();
    void on_actionMassSearchRunRange_triggered();
    void on_actionMassSearchDateRange_triggered();
    void on_actionClearCachedSearches_triggered();

    /*
     * Visualisation
     */
    private slots:
    // Handle extracted SE log values for plotting
    void handlePlotSELogValue(HttpRequestWorker *worker);

    /*
     * Nexus Interaction Stuff To Be Organised
     */
    private slots:
    void handle_result_contextGraph(HttpRequestWorker *worker);
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
