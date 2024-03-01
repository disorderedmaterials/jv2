// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QMessageBox>

/*
 * UI
 */

// Update journal generation page for specified source
void MainWindow::updateGenerationPage(int nCompleted, const QString &lastFileProcessed)
{
    ui_.GeneratingProgressBar->setValue(nCompleted);
    ui_.GeneratingInfoLabel->setText(QString("Last file processed was '%1')").arg(lastFileProcessed));
}

void MainWindow::on_GeneratingCancelButton_clicked(bool checked)
{
    if (!sourceBeingGenerated_)
        return;

    if (QMessageBox::question(
            this, "Stop Journal Generation?",
            QString("Are you sure you want to cancel journal generation for '%1'?\nAll progress to date will be lost.")
                .arg(sourceBeingGenerated_->sourceID())) == QMessageBox::StandardButton::Yes)
        backend_.generateBackgroundScanStop([&](HttpRequestWorker *worker) { handleGenerateBackgroundScanStop(worker); });
}

/*
 * Network Handling
 */

// Handle returned directory list result
void MainWindow::handleGenerateList(HttpRequestWorker *worker, bool updateCurrentCollection)
{
    // Check network reply
    if (handleRequestError(worker, "trying to list data directory") != NoError)
    {
        sourceBeingGenerated_ = nullptr;
        return;
    }

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

        sourceBeingGenerated_ = nullptr;

        return;
    }

    // Update the GUI
    ui_.GeneratingPageLabel->setText(
        QString("Generating Journals for Source '%1'...\nSource Data Directory is '%2', organised by '%3'")
            .arg(sourceBeingGenerated_->name(), sourceBeingGenerated_->runDataRootUrl(),
                 JournalSource::dataOrganisationType(sourceBeingGenerated_->dataOrganisation())));
    ui_.GeneratingProgressBar->setMaximum(nFilesFound);
    updateGenerationPage(0, "<No Files Scanned>");

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
    if (updateCurrentCollection)
        backend_.generateBackgroundUpdate(sourceBeingGenerated_,
                                          [&](HttpRequestWorker *scanWorker) { handleGenerateBackgroundScan(scanWorker); });
    else
        backend_.generateBackgroundScan(sourceBeingGenerated_,
                                        [&](HttpRequestWorker *scanWorker) { handleGenerateBackgroundScan(scanWorker); });
}

// Handle / monitor the generation background scan
void MainWindow::handleGenerateBackgroundScan(HttpRequestWorker *worker)
{
    // Check network reply
    if (handleRequestError(worker, "trying to perform background scan") != NoError)
        return;

    if (!sourceBeingGenerated_)
        throw(std::runtime_error("No target source for generation is set.\n"));

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
                                if (sourceBeingGenerated_ == currentJournalSource_)
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
                                backend_.generateFinalise(sourceBeingGenerated_,
                                                          [=](HttpRequestWorker *worker) { handleGenerateFinalise(worker); });
                            else
                                handleGenerateBackgroundScan(worker);
                        }
                    });
            });
    pingTimer->start();
}

// Handle journal generation finalisation
void MainWindow::handleGenerateFinalise(HttpRequestWorker *worker)
{
    // Check network reply
    if (handleRequestError(worker, "trying to generate journals for directory") != NoError)
        return;

    // Success?
    if (!worker->response().startsWith("\"SUCCESS"))
    {
        setErrorPage("Journal Generation Failed", "Something happened.");
        updateForCurrentSource(JournalSource::JournalSourceState::Error);
        return;
    }

    // Generation was a success, so clean up
    sourceBeingGenerated_->setState(JournalSource::JournalSourceState::Loading);

    statusBar()->showMessage(QString("Journal generation completed for source '%1'.\n").arg(sourceBeingGenerated_->name()));

    // Show the new journals _if_ the current journal source being displayed is the one just generated
    if (sourceBeingGenerated_ == currentJournalSource_)
        setCurrentJournalSource(sourceBeingGenerated_);

    sourceBeingGenerated_ = nullptr;
}

void MainWindow::handleGenerateBackgroundScanStop(HttpRequestWorker *worker)
{
    // Check network reply
    if (handleRequestError(worker, "trying to stop run data scan for directory") != NoError)
        return;
}

// Handle get generated journal updates result
void MainWindow::handleGetGeneratedJournalUpdates(HttpRequestWorker *worker)
{
    printf("HERE WE ARE.\n");

    // Check network reply
    if (handleRequestError(worker, "trying to list data directory") != NoError)
    {
        sourceBeingGenerated_ = nullptr;
        return;
    }

    qDebug() << worker->response();

    // Result contains the number of NeXuS files found and the target dir
    const auto receivedData = worker->jsonResponse().object();
    auto nFilesFound = receivedData["num_files"].toInt();
    auto dataDirectory = receivedData["data_directory"].toString();
    qDebug() << nFilesFound;
    qDebug() << dataDirectory;

    // Begin the background file scan
    //    backend_.generateUpdate(sourceBeingGenerated_, [&](HttpRequestWorker *scanWorker) { handleGenerateBackgroundScan();
    //    });
}
