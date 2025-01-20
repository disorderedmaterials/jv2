// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "plotLogDataWidget.h"
#include <QSplitter>

namespace JV2
{
PlotLogDataWidget::PlotLogDataWidget(MainWindow *parent, Backend &backend, const JournalSource *source,
                                     const std::vector<int> &runNumbers)
    : QWidget(parent), mainWindow_(parent), backend_(backend), source_(source), runNumbers_(runNumbers),
      logValueFilterProxy_(logValueModel_)
{
    ui_.setupUi(this);

    ui_.PropertyList->setModel(&logValueFilterProxy_);
    ui_.PropertyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(&logValueModel_, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)), this,
            SLOT(logValuesChanged(const QModelIndex &, const QModelIndex &, const QList<int> &)));

    // Acquire the available log data
    backend_.getNeXuSLogValues(source_, runNumbers_, [=](HttpRequestWorker *worker) { handleRetrieveSELogProperties(worker); });
}

PlotLogDataWidget::~PlotLogDataWidget() {}

void PlotLogDataWidget::handleRetrieveSELogProperties(HttpRequestWorker *worker)
{
    // Check for errors
    if (mainWindow_->handleRequestError(worker, "retrieving log values from run") != Backend::NoError)
        return;

    // Iterate over log values extracted from the target run data and create a vector of all those available
    logValues_.reserve(1024);
    logValues_.clear();
    foreach (const auto &log, worker->jsonResponse().array())
    {
        auto logArray = log.toArray();
        if (logArray.size() < 2)
            continue;

        // Remove the name item and proceed to iterate over log values
        logArray.removeFirst();

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
            logValues_.emplace_back(block.toString().split("/").last(), block.toString());
    }

    logValueModel_.setData(logValues_);
}

// Log value selection changed
void PlotLogDataWidget::logValuesChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    auto optData = logValueModel_.getData(topLeft);
    auto &data = optData->get();
    qDebug() << "Toggled data was " + data.name();
}
} // namespace JV2
