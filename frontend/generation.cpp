// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QMessageBox>

// Handle returned directory list result
void MainWindow::handleGenerateList(JournalSource &source, HttpRequestWorker *worker)
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

    // Update the GUI
    ui_.GeneratingPageLabel->setText(
        QString("Generating Journals for Source '%1'...\nSource Data Directory is '%2', organised by '%3'")
            .arg(source.name(), source.runDataRootUrl(), JournalSource::dataOrganisationType(source.runDataOrganisation())));
    ui_.GeneratingProgressBar->setMaximum(nFilesFound);
    updateGenerationPage(0, "<No Files Scanned>");
    sourceBeingGenerated_ = source;

    // Make a file tree
    auto *rootItem = new GenericTreeItem({"Journal", "Filename / Path"});
    auto files = receivedData["files"].toObject();
    for (const auto &key : files.keys())
    {
        auto *sectionItem = rootItem->appendChild({key, ""});

        auto dataFiles = files[key].toArray();
        for (const auto &file : dataFiles)
            sectionItem->appendChild({"", file.toString()});
    }
    ui_.GeneratingTreeView->setModel(nullptr);
    generatorScannedFilesModel_.setRootItem(rootItem);
    ui_.GeneratingTreeView->setModel(&generatorScannedFilesModel_);
    ui_.GeneratingTreeView->expandAll();
    ui_.GeneratingTreeView->resizeColumnToContents(0);
    ui_.GeneratingTreeView->resizeColumnToContents(1);

    updateForCurrentSource(JournalSource::JournalSourceState::Generating);

    // Begin the background file scan
    backend_.generateBackgroundScan(source, [&](HttpRequestWorker *scanWorker) { handleGenerateBackgroundScan(); });
}

// Handle / monitor the generation background scan
void MainWindow::handleGenerateBackgroundScan()
{
    if (!sourceBeingGenerated_)
        throw(std::runtime_error("No target source for generation is set.\n"));
    auto &source = sourceBeingGenerated_->get();

    // Create a timer to ping the backend for a scan update after 1000 ms
    auto *pingTimer = new QTimer;
    pingTimer->setSingleShot(true);
    pingTimer->setInterval(1000);
    connect(pingTimer, &QTimer::timeout,
            [&]()
            {
                backend_.generateBackgroundScanUpdate(
                    [this](HttpRequestWorker *worker)
                    {
                        if (worker->response().startsWith("\"NOT_RUNNING"))
                        {
                            // If we are currently displaying the target source for generation, indicate an error
                            if (currentJournalSource_ && sourceBeingGenerated_)
                            {
                                if (&sourceBeingGenerated_->get() == &currentJournalSource_->get())
                                {
                                    setErrorPage("Journal Scan Failed", "Best complain to somebody about it...");
                                    updateForCurrentSource(JournalSource::JournalSourceState::Error);
                                }
                            }
                            return;
                        }
                        else
                        {
                            // Update the generator page of the stack
                            auto nCompleted = worker->jsonResponse()["num_completed"].toInt();
                            auto lastFilename = worker->jsonResponse()["last_filename"].toString();
                            updateGenerationPage(nCompleted, lastFilename);

                            // Complete?
                            if (worker->jsonResponse()["complete"].toBool())
                                backend_.generateFinalise(sourceBeingGenerated_->get(), [=](HttpRequestWorker *worker)
                                                          { handleGenerateFinalise(sourceBeingGenerated_->get(), worker); });
                            else
                                handleGenerateBackgroundScan();
                        }
                    });
            });
    pingTimer->start();
}

// Handle journal generation finalisation
void MainWindow::handleGenerateFinalise(JournalSource &source, HttpRequestWorker *worker)
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

// Update journal generation page for specified source
void MainWindow::updateGenerationPage(int nCompleted, QString currentFile)
{
    ui_.GeneratingProgressBar->setValue(nCompleted);

    //    generatorScannedFilesModel_;
}
