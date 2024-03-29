// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "journalSourcesDialog.h"
#include "mainWindow.h"
#include <QCommandLineParser>
#include <QDomDocument>
#include <QMessageBox>
#include <QSettings>

/*
 * Private Functions
 */

// Find the specified journal source
JournalSource *MainWindow::findJournalSource(const QString &name)
{
    auto sourceIt = std::find_if(journalSources_.begin(), journalSources_.end(),
                                 [name](const auto &source) { return source->name() == name; });
    if (sourceIt != journalSources_.end())
        return sourceIt->get();

    return nullptr;
}

// Set current journal source
void MainWindow::setCurrentJournalSource(JournalSource *source, std::optional<QString> goToJournal)
{
    Locker updateLock(controlsUpdating_);

    // Clear any existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    currentJournalSource_ = source;

    // If no source was specified, we're done
    if (!currentJournalSource_)
    {
        updateForCurrentSource();

        return;
    }

    // If this source is generating, move to the generator page and stop there
    if (currentJournalSource_->state() == JournalSource::JournalSourceState::Generating)
    {
        updateForCurrentSource();
        return;
    }

    // Make sure we have an instrument set if one is required
    if (currentJournalSource_->instrumentRequired() && !currentJournalSource_->currentInstrument())
        currentJournalSource_->setCurrentInstrument(instruments_.front());

    // Reset the state of the source since we can't assume the result of the index request
    currentJournalSource_->setState(JournalSource::JournalSourceState::Loading);

    updateForCurrentSource();

    backend_.getJournalIndex(currentJournalSource(),
                             [&](HttpRequestWorker *worker) { handleListJournals(worker, goToJournal); });
}

// Return current journal source
JournalSource *MainWindow::currentJournalSource() const
{
    if (currentJournalSource_)
        return currentJournalSource_;

    throw(std::runtime_error("No current journal source defined.\n"));
}

// Return selected journal in current source (assuming one is selected)
Journal &MainWindow::currentJournal() const
{
    if (currentJournalSource_ && currentJournalSource_->currentJournal())
        return currentJournalSource_->currentJournal()->get();

    throw(std::runtime_error("No current journal can be assumed (either the source or the selected journal is not defined.\n"));
}

/*
 * UI
 */

void MainWindow::on_JournalSourceComboBox_currentIndexChanged(int index)
{
    if (controlsUpdating_)
        return;

    if (index == -1)
        setCurrentJournalSource({});
    else
    {
        auto optSource = findJournalSource(ui_.JournalSourceComboBox->currentText());
        if (!optSource)
            throw(std::runtime_error("Selected journal source does not exist!\n"));
        setCurrentJournalSource(optSource);
    }
}

void MainWindow::on_JournalComboBox_currentIndexChanged(int index)
{
    if (!currentJournalSource_ || controlsUpdating_)
        return;

    currentJournalSource_->setCurrentJournal(index);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

void MainWindow::on_JournalComboBackToJournalsButton_clicked(bool checked)
{
    if (!currentJournalSource_)
        return;

    currentJournalSource_->stopShowingSearchedData();

    ui_.JournalComboStack->setCurrentIndex(0);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

void MainWindow::on_actionEditSources_triggered()
{
    JournalSourcesDialog sourcesDialog(this);

    sourcesDialog.go(journalSources_);

    storeJournalSourcesToSettings();

    Locker updateLocker(controlsUpdating_);
    journalSourceModel_.setData(journalSources_);
    updateLocker.unlock();

    updateForCurrentSource();
}

void MainWindow::on_actionRegenerateSource_triggered()
{
    // Check if another source is being generated...
    if (sourceBeingGenerated_)
    {
        QMessageBox::warning(this, "Error",
                             QString("Can't generate indices for '%1' because another generation "
                                     "process is currently active (for '%2').")
                                 .arg(currentJournalSource_->sourceID(), sourceBeingGenerated_->sourceID()));
    }
    else if (QMessageBox::question(
                 this, "Regenerate Source Indices",
                 QString("This will completely regenerate all index files for '%1'.\nAre you sure you want to proceed?")
                     .arg(currentJournalSource_->sourceID())) == QMessageBox::StandardButton::Yes)
    {
        sourceBeingGenerated_ = currentJournalSource_;
        backend_.generateList(currentJournalSource(), [&](HttpRequestWorker *worker)
                              { handleGenerateList(worker, Backend::JournalGenerationStyle::Full); });
    }
}

/*
 * Network Handling
 */

// Handle returned journal information for an instrument
void MainWindow::handleListJournals(HttpRequestWorker *worker, std::optional<QString> journalToLoad)
{
    if (!currentJournalSource_)
        return;

    Locker updateLocker(controlsUpdating_);

    // Clear existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    // Check network reply
    if (handleRequestError(worker, "trying to list journals") != NoError)
        return;

    // Special case - for cache or disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response().startsWith("\"Index File Not Found\""))
    {
        setErrorPage("No Index File Found", "An index file could not be found.");
        updateForCurrentSource(JournalSource::JournalSourceState::Error);

        // Check if another source is being generated...
        if (sourceBeingGenerated_)
        {
            QMessageBox::warning(this, "Index File Doesn't Exist",
                                 QString("No index file currently exists in '%1'.\nIt can be created but another generation "
                                         "process is currently active (for '%2').")
                                     .arg(currentJournalSource_->sourceID(), sourceBeingGenerated_->sourceID()));
        }
        else if (QMessageBox::question(this, "Index File Doesn't Exist",
                                       QString("No index file currently exists in '%1'.\nWould you like to generate it now?")
                                           .arg(currentJournalSource_->sourceID())) == QMessageBox::StandardButton::Yes)
        {
            sourceBeingGenerated_ = currentJournalSource_;
            backend_.generateList(currentJournalSource(), [&](HttpRequestWorker *worker)
                                  { handleGenerateList(worker, Backend::JournalGenerationStyle::Full); });
        }

        return;
    }

    // Add returned journals
    currentJournalSource_->setJournals(worker->jsonResponse().array());

    // Set a named journal as the current one (optional)
    if (journalToLoad)
    {
        if (currentJournalSource_->findJournal(*journalToLoad))
            currentJournalSource_->setCurrentJournal(*journalToLoad);
    }

    journalModel_.setData(currentJournalSource_->journals());

    updateForCurrentSource();

    // Now have a new current journal, so retrieve it
    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

// Handle run data returned for a whole journal
void MainWindow::handleCompleteJournalRunData(HttpRequestWorker *worker, std::optional<int> runNumberToHighlight)
{
    runData_ = QJsonArray();
    runDataModel_.setData(runData_);

    // Check network reply
    if (handleRequestError(worker, "trying to retrieve run data for the journal") != NoError)
        return;

    // Turn off grouping
    if (ui_.GroupRunsButton->isChecked())
        ui_.GroupRunsButton->setChecked(false);

    // Get desired fields and titles from config files
    runDataColumns_ = currentInstrument() ? currentInstrument()->get().runDataColumns()
                                          : Instrument::runDataColumns(Instrument::InstrumentType::Neutron);
    runData_ = worker->jsonResponse().array();

    // Set table data
    runDataModel_.setHorizontalHeaders(runDataColumns_);
    runDataModel_.setData(runData_);

    ui_.RunDataTable->resizeColumnsToContents();
    updateSearch(searchString_);
    ui_.RunFilterEdit->clear();

    updateForCurrentSource(JournalSource::JournalSourceState::OK);

    // Highlight / go to specific run number if requested
    if (runNumberToHighlight)
        highlightRunNumber(*runNumberToHighlight);
}

// Handle get journal updates result
void MainWindow::handleGetJournalUpdates(HttpRequestWorker *worker)
{
    // A null response indicates no change
    if (worker->response().startsWith("null"))
        return;

    // The main body of the request contains any run numbers we don't currently have.
    // If we are currently displaying grouped data we append the new data directly then refresh the grouping
    if (ui_.GroupRunsButton->isChecked())
    {
        foreach (const auto &item, worker->jsonResponse().array())
            runData_.append(item);

        generateGroupedData();

        runDataModel_.setData(groupedRunData_);
        runDataModel_.setHorizontalHeaders(groupedRunDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        // Update via the model
        runDataModel_.appendData(worker->jsonResponse().array());
    }
}

// Handle jump to journal
void MainWindow::handleJumpToJournal(HttpRequestWorker *worker)
{
    // Check network reply
    if (handleRequestError(worker, "trying to select run number within journal") != NoError)
        return;

    // Get data from the response
    auto journalName = worker->jsonResponse()["journal_display_name"].toString();
    auto runNumber = worker->jsonResponse()["run_number"].toInt();

    // If the journal name is empty then no containing journal could be found
    if (journalName.isEmpty())
    {
        QMessageBox::information(this, "Not Found", QString("Run number %1 could not be found in any journal").arg(runNumber));
        return;
    }

    // Find the named journal in the current source
    auto optJournal = currentJournalSource()->findJournal(journalName);
    if (!optJournal)
        return;

    // If this is the current journal just jump to the run number if provided
    if (&optJournal->get() == &currentJournal())
    {
        if (runNumber > 0)
            highlightRunNumber(runNumber);
        return;
    }

    // Otherwise set the new journal, load it, and highlight the run number if provided
    currentJournalSource()->setCurrentJournal(journalName);
    updateForCurrentSource();

    // Now have a new current journal, so retrieve it
    backend_.getJournal(currentJournalSource(),
                        [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker, runNumber); });
}
