// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "backend.h"
#include "genericTreeModel.h"
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
    PlotLogDataWidget(MainWindow *parent, Backend &backend, const JournalSource *source, const std::vector<int> &runNumbers);
    ~PlotLogDataWidget();

    private:
    // User interface object
    Ui::PlotLogDataWidget ui_;
    // Main Window parent
    MainWindow *mainWindow_{nullptr};
    // Tree model for properties
    GenericTreeModel propertyModel_;
    // Main backend
    Backend &backend_;
    // Journal source from which the run numbers came
    const JournalSource *source_{nullptr};
    // Run numbers to display on the plot
    std::vector<int> runNumbers_;

    private:
    // Handle retrieved log properties data
    void handleRetrieveSELogProperties(HttpRequestWorker *worker);

    private slots:

    signals:
};
} // namespace JV2
