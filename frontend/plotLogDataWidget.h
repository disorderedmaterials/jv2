// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "backend.h"
#include "logValue.h"
#include "logValueFilterProxy.h"
#include "logValueGroup.h"
#include "logValueGroupModel.h"
#include "logValueModel.h"
#include "mainWindow.h"
#include "ui_plotLogDataWidget.h"
#include <QWidget>

namespace JV2
{
// Forward Declarations
class JournalSource;

class PlotLogDataWidget : public QWidget
{
    Q_OBJECT

    public:
    PlotLogDataWidget(MainWindow *parent, Backend &backend, const JournalSource *journalSource,
                      const std::vector<int> &runNumbers);
    ~PlotLogDataWidget() = default;

    private:
    // User interface object
    Ui::PlotLogDataWidget ui_;
    // Model and proxies for log values
    LogValueModel logValueModel_;
    LogValueFilterProxy availableLogValueFilterProxy_;
    LogValueFilterProxy shownLogValueFilterProxy_;
    // Model and proxy for log value groups
    LogValueGroupModel logValueGroupModel_;
    // Main Window parent
    MainWindow *mainWindow_{nullptr};
    // Main backend
    Backend &backend_;
    // Journal source from which the run numbers came
    const JournalSource *journalSource_{nullptr};
    // Log values available for plotting
    std::vector<LogValue> logValues_;
    // Run numbers to display on the plot
    std::vector<int> runNumbers_;
    // Log value groups (per run-number)
    std::vector<LogValueGroup> logValueGroups_;

    private:
    // Handle retrieved log values
    void handleRetrieveSELogValues(HttpRequestWorker *worker);
    // Handle retrieved log value data
    void handleRetrieveSELogValueData(HttpRequestWorker *worker);
    // Show data from the supplied LogValue on the plot
    void showData(const LogValue &logValue);
    // Hide data from the supplied LogValue from the plot
    void hideData(const LogValue &logValue);

    private slots:
    // Log value selection changed
    void logValueChanged(const QModelIndex &, const QModelIndex &, const QList<int> &);
    // Log value group selection changed
    void logValueGroupChanged(const QModelIndex &, const QModelIndex &, const QList<int> &);

    public:
    // Create summary text for the plot
    QString summaryText(const QString &lastProperty = {}) const;

    signals:
    // Summary text updated
    void summaryTextChanged(QString);
};
} // namespace JV2
