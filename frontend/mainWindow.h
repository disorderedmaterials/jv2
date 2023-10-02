// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "httpRequestWorker.h"
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
    ~MainWindow();

    /*
     * UI
     */
    private:
    Ui::MainWindow ui_;

    bool init_;
    bool validSource_;
    QPoint pos_;

    /*
     * Main Data
     */
    private:
    // Init
    void fillInstruments(QList<std::tuple<QString, QString, QString>> instruments);
    void setLoadScreen(bool state);
    QString getRunNos();

    void checkForUpdates();

    private slots:
    // Data Selection
    void handle_result_instruments(HttpRequestWorker *worker);
    void handle_result_cycles(HttpRequestWorker *worker);
    void currentInstrumentChanged(const QString &arg1);
    void changeCycle(QString value);
    void recentCycle();
    void changeInst(std::tuple<QString, QString, QString> instrument);

    // Misc Interface Functions
    void removeTab(int index);
    void columnHider(int state);

    void refresh(QString Status);
    void update(HttpRequestWorker *worker);
    void refreshTable();

    protected:
    // Window close event
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    signals:
    void tableFilled();

    private:
    // Table Stuff
    JsonTableModel *model_;
    JSONTableFilterProxy *proxyModel_;
    JsonTableModel::Header header_;
    std::vector<std::pair<QString, QString>> desiredHeader_;
    // Menus
    QMenu *viewMenu_;
    QMenu *findMenu_;
    QMenu *contextMenu_;
    QMenu *instrumentsMenu_;
    QMenu *cyclesMenu_;

    QModelIndexList foundIndices_;
    int currentFoundIndex_;
    // Menu button data
    QString searchString_;
    QString instType_;
    QString instName_;
    QString instDisplayName_;
    QMap<QString, QString> cyclesMap_;
    QMap<QString, QString> headersMap_;

    /*
     * Settings
     */
    private:
    void savePref();
    void clearPref();
    // Get available instruments from config file
    QList<std::tuple<QString, QString, QString>> getInstruments();
    QDomDocument getConfig();
    // Get Fields from config file
    std::vector<std::pair<QString, QString>> getFields(QString instrument, QString instType);

    private slots:
    void on_actionMountPoint_triggered();
    void on_actionClearMountPoint_triggered();
    void on_actionSetLocalSource_triggered();
    void on_actionClearLocalSource_triggered();

    /*
     * Searching
     */
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
    void on_ClearFilterButton_clicked();
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
