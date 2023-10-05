// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "seLogChooserDialog.h"
#include <QMessageBox>

// Handle extracted SE log values for plotting
void MainWindow::handlePlotSELogValue(HttpRequestWorker *worker)
{
    // Network error?
    if (worker->errorType != QNetworkReply::NoError)
    {
        QMessageBox::information(this, "Network Error",
                                 "A network error occurred while retrieving the run information.\nThe reported error was: " +
                                     worker->errorString);
        return;
    }

    // Other error?
    auto response = worker->response;
    if (response.contains("Error"))
    {
        statusBar()->showMessage("Network error!");
        QMessageBox::warning(this, "An Error Occurred", response);
        return;
    }

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

    auto result = chooserDialog.getValue();
    qDebug() << result;
    if (result.isEmpty())
        return;

    auto &&[runNos, cycles] = selectedRunNumbersAndCycles();
    if (runNos.size() == 0)
        return;

    // Request the log value data - need to reformat it and replace '/' with ':' so it doesn't interfere with our route
    auto *dataWorker = new HttpRequestWorker(this);
    connect(dataWorker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextGraph(HttpRequestWorker *)));
    setLoadScreen(true);
    dataWorker->execute("http://127.0.0.1:5000/getNexusData/" + currentInstrument().dataDirectory() + "/" + cycles + "/" +
                        runNos + "/" + result.replace("/", ":"));
}
