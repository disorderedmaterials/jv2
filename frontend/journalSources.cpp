// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QFile>

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
        auto sourceType = JournalSource::journalSourceType(sourceElement.attribute("type", "DiskByDirectory"));

        // Get source root URL
        auto sourceRootURL = sourceElement.attribute("rootUrl");

        // Data directory and main index file name
        auto sourceDataDirectory = sourceElement.attribute("dataDirectory");
        auto sourceIndexFile = sourceElement.attribute("indexFile");

        // Whether the journals / data are organised by known instrument
        auto organisedByInstrument = sourceElement.attribute("byInstrument", "false").toLower() == "true";

        // Create the source
        auto &journalSource = journalSources_.emplace_back(sourceName, sourceType, sourceRootURL, sourceDataDirectory,
                                                           sourceIndexFile, organisedByInstrument);
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
    // Clear any existing journal (and run) data
    clearJournals();

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

    bool orgByInst = currentJournalSource_->get().organisedByInstrument();
    backend_.listJournals(currentJournalSource(), orgByInst ? currentInstrument().journalDirectory() : "",
                          [=](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current journal source
const JournalSource &MainWindow::currentJournalSource() const
{
    if (currentJournalSource_)
        return currentJournalSource_->get();

    throw(std::runtime_error("No current journal source defined.\n"));
}

/*
 * Private SLots
 */

void MainWindow::on_JournalSourceComboBox_currentIndexChanged(int index)
{
    if (index == -1)
        setCurrentJournalSource({});
    else
        setCurrentJournalSource(ui_.JournalSourceComboBox->currentText());
}