// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

// Handle returned directory list result
void MainWindow::handleListDataDirectory(const JournalSource &source, HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "trying to list data directory"))
        return;

    // Result contains the number of NeXuS files found and the target dir
    const auto receivedData = worker->jsonResponse.object();
    auto nFilesFound = receivedData["num_files"].toInt();
    auto dataDirectory = receivedData["data_directory"].toString();

    // Check for zero files
    if (nFilesFound == 0)
    {
        statusBar()->showMessage("No NeXuS files found.");
        QMessageBox::warning(
            this, "File Error",
            QString("No NeXuS files were found in '%1'.\nCheck the location, network mounts etc.").arg(dataDirectory));

        updateForCurrentSource(JournalSource::JournalSourceState::RunDataScanNoFilesError);

        return;
    }

    // Begin the journal generation
    backend_.generateJournals(source);
}
