// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "seLogChooserDialog.h"
#include <QMessageBox>

// Handle extracted SE log values for plotting
void MainWindow::handlePlotSELogValue(HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "retrieving log values from run"))
        return;

    // Iterate over logs extracted from the target run data and construct our mapped values
    auto *rootItem = new GenericTreeItem({"Log Value", "Full Path"});
    foreach (const auto &log, worker->jsonArray)
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

    // Create the dialog
    SELogChooserDialog chooserDialog(this, rootItem);

    auto logValue = chooserDialog.getValue();
    qDebug() << logValue;
    if (logValue.isEmpty())
        return;

    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();
    if (runNos.size() == 0)
        return;

    // Request the log value data
    backend_.getNexusLogValueData(currentJournal().location(), selectedRunNumbers(), logValue,
                                  [=](HttpRequestWorker *worker) { handle_result_contextGraph(worker); });

    setLoadScreen(true);
}
