// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "plotLogDataWidget.h"
#include <QSplitter>

PlotLogDataWidget::PlotLogDataWidget(QWidget *parent, Backend &backend, const JournalSource* source, const std::vector<int> &runNumbers) : QWidget(parent), backend_(backend), source_(source), runNumbers_(runNumbers)
{
    ui_.setupUi(this);

//    propertyModel_.setRootItem(rootItem);
    ui_.PropertyTree->setModel(&propertyModel_);
    ui_.PropertyTree->expandAll();
    ui_.PropertyTree->resizeColumnToContents(0);
    ui_.PropertyTree->resizeColumnToContents(1);
    ui_.PropertyTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    connect(ui_.PropertyTree->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this,
            SLOT(onTreeSelectionChanged(const QItemSelection &, const QItemSelection &)));

    // Acquire the available log data
    backend_.getNexusFields(source_, runNumbers_,
                                    [=](HttpRequestWorker *worker) { handleRetrieveSELogProperties(worker); });
}

void PlotLogDataWidget::handleRetrieveSELogProperties(HttpRequestWorker* worker)
{    // Check for errors
    if (handleRequestError(worker, "retrieving log values from run") != NoError)
        return;

    // Iterate over logs extracted from the target run data and construct our mapped values

    auto *rootItem = new GenericTreeItem({"Log Value", "Full Path"});
    foreach (const auto &log, worker->jsonResponse().array())
    {
        auto logArray = log.toArray();
        if (logArray.size() < 2)
            continue;

        // First item in the array is the name of the log value set / section
        auto *sectionItem = rootItem->appendChild({logArray.first().toString(), ""});

        // Remove the name item and proceed to iterate over log values
        logArray.removeFirst();

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
            sectionItem->appendChild({block.toString().split("/").last(), block.toString()});
    }
}

PlotLogDataWidget::~PlotLogDataWidget() {}
