// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
#include "instrument.h"
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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

    /*
     * UI
     */
    private:
    Ui::MainWindow ui_;
    QMenu *viewMenu_;
    QMenu *findMenu_;
    QMenu *contextMenu_;
    QMenu *instrumentsMenu_;
    QMenu *cyclesMenu_;
    bool init_;
    bool validSource_;
    QPoint pos_;

    private slots:
    void setLoadScreen(bool state);
    void removeTab(int index);
    void columnHider(int state);

    protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    /*
     * Instrument Sources
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
     * Main Data
     */
    private:
    QJsonArray runData_, groupedRunData_;
    QMap<QString, QString> cyclesMap_;
    QMap<QString, QString> headersMap_;
    JsonTableModel runDataModel_;
    JsonTableFilterProxy runDataFilterProxy_;
    JsonTableModel::Header header_, groupedTableHeaders_;
    std::vector<std::pair<QString, QString>> desiredHeader_;

    private:
    // Generate grouped run data from current run data
    void generateGroupedData();
    QString getRunNos();
    void checkForUpdates();

    private slots:
    // Handle cycle update result
    void handleCycleUpdate(QString response);
    // Handle JSON run data returned from workers
    void handleRunData(HttpRequestWorker *worker);
    // Handle returned cycle information for an instrument
    void handleGetCycles(HttpRequestWorker *worker);
    // Handle run data returned for a whole cycle
    void handleCycleRunData(HttpRequestWorker *worker);

    private slots:
    // Set current cycle being displayed
    void setCurrentCycle(QString cycleName);
    void recentCycle();

    signals:
    void tableFilled();

    /*
     * Settings
     */
    private:
    // Load
    QDomDocument getConfig();
    // Get Fields from config file
    std::vector<std::pair<QString, QString>> getFields(QString instrument, QString instType);

    private slots:
    void savePref();
    void clearPref();
    void on_actionMountPoint_triggered();
    void on_actionClearMountPoint_triggered();
    void on_actionSetLocalSource_triggered();
    void on_actionClearLocalSource_triggered();

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
    void selectIndex(QString runNumber);
    void selectSimilar();
    void goToCurrentFoundIndex(QModelIndex index);

    private slots:
    void on_actionSearch_triggered();
    void on_actionSelectNext_triggered();
    void on_actionSelectPrevious_triggered();
    void on_actionSelectAll_triggered();

    /*
     * Filtering
     */
    private:
    void goTo(HttpRequestWorker *worker, QString runNumber);

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
    void on_actionMassSearchRB_No_triggered();
    void on_actionMassSearchTitle_triggered();
    void on_actionMassSearchUser_triggered();
    void on_actionMassSearchRunRange_triggered();
    void on_actionMassSearchDateRange_triggered();
    void on_actionClear_cached_searches_triggered();
    void on_actionRun_Number_triggered();

    /*
     * Visualisation
     */
    private slots:
    void customMenuRequested(QPoint pos);
    void handle_result_contextGraph(HttpRequestWorker *worker);
    void contextGraph();
    void handle_result_contextMenu(HttpRequestWorker *worker);
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
