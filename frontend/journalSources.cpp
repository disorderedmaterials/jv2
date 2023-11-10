// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QMessageBox>
#include <QSettings>

/*
 * Private Functions
 */

// Set up standard journal sources
void MainWindow::setUpStandardJournalSources()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    // The main ISIS Archive
    auto &isisArchive = journalSources_.emplace_back("ISIS Archive", JournalSource::IndexingType::Network);
    isisArchive.setJournalOrganisationByInstrument(Instrument::InstrumentPathType::AltNDXName);
    isisArchive.setRunDataOrganisationByInstrument(Instrument::InstrumentPathType::NDXName);
    isisArchive.setJournalData("http://data.isis.rl.ac.uk/journals", "journal_main.xml");
    isisArchive.setRunDataLocation(settings.value("ISISArchiveDataUrl", "/archive").toString(),
                                   JournalSource::DataOrganisationType::Directory);

    // IDAaaS RB Directories
    auto &idaaasRB = journalSources_.emplace_back("IDAaaS", JournalSource::IndexingType::Generated);
    idaaasRB.setRunDataOrganisationByInstrument(Instrument::InstrumentPathType::NDXName);
    idaaasRB.setRunDataLocation("/mnt/ceph/instrument_data_cache", JournalSource::DataOrganisationType::RBNumber);
}

// Find the specified journal source
OptionalReferenceWrapper<JournalSource> MainWindow::findJournalSource(const QString &name)
{
    auto sourceIt = std::find_if(journalSources_.begin(), journalSources_.end(),
                                 [name](const auto &source) { return source.name() == name; });
    if (sourceIt != journalSources_.end())
        return *sourceIt;

    return {};
}

// Set current journal source
void MainWindow::setCurrentJournalSource(OptionalReferenceWrapper<JournalSource> optSource, std::optional<QString> goToJournal)
{
    Locker updateLock(controlsUpdating_);

    // Clear any existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    currentJournalSource_ = optSource;

    // If no source was specified, we're done
    if (!currentJournalSource_)
    {
        updateForCurrentSource();

        return;
    }

    // Make sure we have an instrument set if one is required
    if (currentJournalSource().instrumentRequired() && !currentJournalSource().currentInstrument())
        currentJournalSource().setCurrentInstrument(instruments_.front());

    // Reset the state of the source since we can't assume the result of the index request
    currentJournalSource().setState(JournalSource::JournalSourceState::Loading);

    updateForCurrentSource();

    backend_.getJournalIndex(currentJournalSource(), [&](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current journal source
JournalSource &MainWindow::currentJournalSource() const
{
    if (currentJournalSource_)
        return currentJournalSource_->get();

    throw(std::runtime_error("No current journal source defined.\n"));
}

// Return selected journal in current source (assuming one is selected)
Journal &MainWindow::currentJournal() const
{
    if (currentJournalSource_ && currentJournalSource_->get().currentJournal())
        return currentJournalSource_->get().currentJournal()->get();

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

    currentJournalSource().setCurrentJournal(index);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

void MainWindow::on_JournalComboBackToJournalsButton_clicked(bool checked)
{
    currentJournalSource().stopShowingSearchedData();

    ui_.JournalComboStack->setCurrentIndex(0);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

/*
 * Network Handling
 */

// Handle returned journal information for an instrument
void MainWindow::handleListJournals(HttpRequestWorker *worker, std::optional<QString> journalToLoad)
{
    Locker updateLocker(controlsUpdating_);

    // Clear existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    // Check network reply
    if (networkRequestHasError(worker, "trying to list journals"))
    {
        ui_.NetworkErrorInfoLabel->setText(worker->response());
        updateForCurrentSource(JournalSource::JournalSourceState::NetworkError);
        return;
    }

    auto &journalSource = currentJournalSource();
    journalSource.clearJournals();

    // Special case - for cache or disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response().startsWith("\"Index File Not Found\""))
    {
        updateForCurrentSource(JournalSource::JournalSourceState::NoIndexFileError);

        auto sourceID = journalSource.instrumentRequired()
                            ? QString("%1/%2").arg(journalSource.name(), journalSource.currentInstrument()->get().name())
                            : journalSource.name();

        if (QMessageBox::question(
                this, "Index File Doesn't Exist",
                QString("No index file currently exists in '%1'.\nWould you like to generate it now?").arg(sourceID)) ==
            QMessageBox::StandardButton::Yes)
            backend_.listDataDirectory(currentJournalSource(),
                                       [&](HttpRequestWorker *worker) { handleListDataDirectory(journalSource, worker); });

        return;
    }

    // Add returned journals
    journalSource.setJournals(worker->jsonResponse().array());

    // Set a named journal as the current one (optional)
    if (journalToLoad)
    {
        if (journalSource.findJournal(*journalToLoad))
            journalSource.setCurrentJournal(*journalToLoad);
    }

    journalModel_.setData(journalSource.journals());

    updateForCurrentSource();

    // Now have a new current journal, so retrieve it
    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
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
