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
        auto sourceType = JournalSource::indexingType(sourceElement.attribute("type", "Generated"));

        // Create the source
        auto &journalSource = journalSources_.emplace_back(sourceName, sourceType);

        // Set whether the journals / data are organised by known instrument
        auto organisedByInstrument = sourceElement.attribute("instrumentSubdirs", "false").toLower() == "true";

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
    // Clear any existing data
    clearRunData();
    journalsMenu_->clear();

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

    currentJournalSource_ = *sourceIt;

    // Clear any mass search results since they're source-specific
    cachedMassSearch_.clear();

    // Reset the state of the source since we can't assume the result of the index request
    currentJournalSource_->get().setState(JournalSource::JournalSourceState::Loading);

    bool orgByInst = currentJournalSource_->get().instrumentSubdirectories();
    backend_.listJournals(currentJournalSource(), orgByInst ? currentInstrument().journalDirectory() : "",
                          [=](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current journal source
JournalSource &MainWindow::currentJournalSource() const
{
    if (currentJournalSource_)
        return currentJournalSource_->get();

    throw(std::runtime_error("No current journal source defined.\n"));
}

// Set current journal in the current journal source
void MainWindow::setCurrentJournal(const QString &name)
{
    currentJournalSource().setCurrentJournal(name);

    ui_.journalButton->setText(name);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(currentJournal().location(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
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

/*
 * Network Handling
 */

// Handle returned journal information for an instrument
void MainWindow::handleListJournals(HttpRequestWorker *worker)
{
    // Clear existing data
    clearRunData();
    journalsMenu_->clear();

    // Check network reply
    if (networkRequestHasError(worker, "trying to list journals"))
    {
        ui_.NetworkErrorInfoLabel->setText(worker->response);
        updateForCurrentSource(JournalSource::JournalSourceState::NetworkError);
        return;
    }

    // Special case - for disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response.startsWith("\"Index File Not Found\""))
    {
        auto &journalSource = currentJournalSource();

        updateForCurrentSource(JournalSource::JournalSourceState::NoIndexFileError);

        bool orgByInst = journalSource.instrumentSubdirectories();
        auto rootUrl = orgByInst ? QString("%1/%2").arg(currentJournalSource().journalRootUrl(), currentInstrument().name())
                                 : currentJournalSource().journalRootUrl();

        if (QMessageBox::question(this, "Index File Doesn't Exist",
                                  QString("No index file %1/%2 currently exists.\nWould you like to generate it now?")
                                      .arg(rootUrl, currentJournalSource().journalIndexFilename())) ==
            QMessageBox::StandardButton::Yes)
        {
            bool orgByInst = currentJournalSource_->get().instrumentSubdirectories();

            backend_.listDataDirectory(currentJournalSource(), orgByInst ? currentInstrument().journalDirectory() : "",
                                       [=](HttpRequestWorker *worker) { handleListDataDirectory(journalSource, worker); });

            return;
        }
    }

    // Add returned journals
    for (auto i = worker->jsonArray.count() - 1; i >= 0; i--)
    {
        auto value = worker->jsonArray[i].toObject();

        currentJournalSource().addJournal(
            value["display_name"].toString(),
            {value["server_root"].toString(), value["directory"].toString(), value["filename"].toString()});

        auto *action = new QAction(value["display_name"].toString(), this);
        connect(action, &QAction::triggered, [=]() { setCurrentJournal(value["display_name"].toString()); });
        journalsMenu_->addAction(action);
    }

    // If there is no current journal, set one
    if (!currentJournalSource().currentJournal() && !currentJournalSource().journals().empty())
        setCurrentJournal(currentJournalSource().journals().front().name());
    else
        updateForCurrentSource(JournalSource::JournalSourceState::OK);
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
