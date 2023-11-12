// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QMessageBox>

// Handle returned directory list result
void MainWindow::handleListDataDirectory(JournalSource &source, HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "trying to list data directory"))
        return;

    // Result contains the number of NeXuS files found and the target dir
    const auto receivedData = worker->jsonResponse().object();
    auto nFilesFound = receivedData["num_files"].toInt();
    auto dataDirectory = receivedData["data_directory"].toString();

    // Check for zero files
    if (nFilesFound == 0)
    {
        statusBar()->showMessage("No NeXuS files found.");
        setErrorPage("Data Error",
                     QString("No NeXuS files were found in '%1'.\nCheck the location, network mounts etc.").arg(dataDirectory));

        updateForCurrentSource(JournalSource::JournalSourceState::Error);

        return;
    }

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    // Begin the journal generation
    backend_.generateJournals(source, [&](HttpRequestWorker *scanWorker) { handleScanResult(source, scanWorker); });
}

// Handle returned journal generation result
void MainWindow::handleScanResult(JournalSource &source, HttpRequestWorker *worker)
{
    // Check network reply
    if (networkRequestHasError(worker, "trying to generate journals for directory"))
        return;

    // Success?
    if (!worker->response().startsWith("\"SUCCESS"))
    {
        setErrorPage("Journal Generation Failed", "Something happened.");
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return;
    }

    // Generation was a success, so try to load in the file we just generated
    setCurrentJournalSource(source);
}
