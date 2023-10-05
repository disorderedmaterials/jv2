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
    auto *rootItem = new SELogTreeItem(QList<QVariant>({"Mr Header"}));

    foreach (const auto &log, worker->jsonArray)
    {
        auto logArray = log.toArray();
        auto name = logArray.first().toString().toUpper();
        name.chop(2);
        auto formattedName = name.append("og");
        // auto *subMenu = new QMenu("Plot from " + formattedName);
        // logArray.removeFirst();
        // if (logArray.size() > 0)
        // contextMenu_->addMenu(subMenu);
        auto *sectionItem = new SELogTreeItem({{formattedName}}, rootItem);
        rootItem->appendChild(sectionItem);

        auto logArrayVar = logArray.toVariantList();
        std::sort(logArrayVar.begin(), logArrayVar.end(),
                  [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

        foreach (const auto &block, logArrayVar)
        {
            sectionItem->appendChild(new SELogTreeItem({{block.toString()}}, sectionItem));
            // seValues[formattedName].push_back(block.toString());
            // Fills contextMenu with all columns
            // QString path = block.toString();
            // auto *action = new QAction(path.right(path.size() - path.lastIndexOf("/") - 1), this);
            // action->setData(path);
            // connect(action, SIGNAL(triggered()), this, SLOT(contextGraph()));
            // subMenu->addAction(action);
        }
    }

    SELogChooserDialog chooserDialog(this);

    chooserDialog.setRootItem(rootItem);
    chooserDialog.exec();
}
