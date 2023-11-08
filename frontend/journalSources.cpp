// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QFile>
#include <QMessageBox>

/*
 * Private Functions
 */

// Parse journal sources from specified source
bool MainWindow::parseJournalSources(const QDomDocument &source)
{
    // Clear old sources
    journalSources_.clear();
    ui_.JournalSourceComboBox->clear();

    auto docRoot = source.documentElement();
    auto sourceNodes = docRoot.elementsByTagName("source");

    // Loop over sources
    for (auto i = 0; i < sourceNodes.count(); ++i)
    {
        auto sourceElement = sourceNodes.item(i).toElement();

        // Get source name
        auto sourceName = sourceElement.attribute("name");

        // Get source type
        auto sourceType = JournalSource::indexingType(sourceElement.attribute("journalType", "Generated"));

        // Create the source
        auto &journalSource = journalSources_.emplace_back(sourceName, sourceType);

        // Set whether the journals / data are organised by known instrument
        journalSource.setInstrumentSubdirectories(sourceElement.attribute("instrumentSubdirs", "false").toLower() == "true");

        // Set journal data
        journalSource.setJournalData(sourceElement.attribute("journalRootUrl"),
                                     sourceElement.attribute("journalIndexFilename"));

        // Run data
        journalSource.setRunDataLocation(
            sourceElement.attribute("runDataRootUrl"),
            JournalSource::dataOrganisationType(sourceElement.attribute("dataOrganisation", "Directory")));
    }

    // Populate the combo box with options
    for (const auto &source : journalSources_)
        ui_.JournalSourceComboBox->addItem(source.name());

    return true;
}

// Get default journal sources complement
void MainWindow::getDefaultJournalSources()
{
    QFile file(":/data/sources.xml");
    if (!file.exists())
        throw(std::runtime_error("Internal journal sources not found.\n"));

    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    if (!parseJournalSources(dom))
        throw(std::runtime_error("Couldn't parse internal journal sources.\n"));
}

/*
 * UI
 */

// Set current journal source
void MainWindow::setCurrentJournalSource(std::optional<QString> optName)
{
    if (controlsUpdating_)
        return;

    // Clear any existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    // If no source is specified, we're done
    if (!optName)
    {
        currentJournalSource_ = std::nullopt;

        updateForCurrentSource();

        return;
    }

    // Find the source specified
    auto name = *optName;
    auto sourceIt = std::find_if(journalSources_.begin(), journalSources_.end(),
                                 [name](const auto &source) { return source.name() == name; });
    if (sourceIt == journalSources_.end())
        throw(std::runtime_error("Selected journal source does not exist!\n"));

    auto &source = *sourceIt;
    currentJournalSource_ = source;

    // Make sure we have a default instrument set if one is required
    if (source.instrumentSubdirectories() && !source.currentInstrument())
        source.setCurrentInstrument(instruments_.front());

    // Reset the state of the source since we can't assume the result of the index request
    source.setState(JournalSource::JournalSourceState::Loading);

    backend_.getJournalIndex(source, [=](HttpRequestWorker *worker) { handleListJournals(worker); });
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
    if (index == -1)
        setCurrentJournalSource({});
    else
        setCurrentJournalSource(ui_.JournalSourceComboBox->currentText());
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
void MainWindow::handleListJournals(HttpRequestWorker *worker)
{
    controlsUpdating_ = true;

    // Clear existing data
    clearRunData();
    journalModel_.setData(std::nullopt);

    // Check network reply
    if (networkRequestHasError(worker, "trying to list journals"))
    {
        ui_.NetworkErrorInfoLabel->setText(worker->response);
        updateForCurrentSource(JournalSource::JournalSourceState::NetworkError);
        controlsUpdating_ = false;
        return;
    }

    auto &journalSource = currentJournalSource();
    journalSource.clearJournals();

    // Special case - for cache or disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response.startsWith("\"Index File Not Found\""))
    {
        updateForCurrentSource(JournalSource::JournalSourceState::NoIndexFileError);

        bool orgByInst = journalSource.instrumentSubdirectories() && journalSource.currentInstrument();
        auto rootUrl =
            orgByInst ? QString("%1/%2").arg(journalSource.journalRootUrl(), journalSource.currentInstrument()->get().name())
                      : journalSource.journalRootUrl();

        if (QMessageBox::question(
                this, "Index File Doesn't Exist",
                QString("No index file '%1/%2' currently exists in the source '%3'.\nWould you like to generate it now?")
                    .arg(rootUrl, journalSource.journalIndexFilename(), journalSource.name())) ==
            QMessageBox::StandardButton::Yes)
        {
            backend_.listDataDirectory(currentJournalSource(),
                                       [=](HttpRequestWorker *worker) { handleListDataDirectory(journalSource, worker); });
            controlsUpdating_ = false;
            return;
        }
    }

    // Add returned journals
    journalSource.setJournals(worker->jsonArray);

    journalModel_.setData(journalSource.journals());

    updateForCurrentSource();

    controlsUpdating_ = false;

    // Now have a new current journal, so retrieve it
    backend_.getJournal(currentJournalSource(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

// Handle get journal updates result
void MainWindow::handleGetJournalUpdates(HttpRequestWorker *worker)
{
    // A null response indicates no change
    if (worker->response.startsWith("null"))
        return;

    // The main body of the request contains any run numbers we don't currently have.
    // If we are currently displaying grouped data we append the new data directly then refresh the grouping
    if (ui_.GroupRunsButton->isChecked())
    {
        foreach (const auto &item, worker->jsonArray)
            runData_.append(item);

        generateGroupedData();

        runDataModel_.setData(groupedRunData_);
        runDataModel_.setHorizontalHeaders(groupedRunDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        // Update via the model
        runDataModel_.appendData(worker->jsonArray);
    }
}
