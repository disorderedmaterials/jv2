// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QFile>

/*
 * Private Functions
 */

// Parse instruments from specified source
bool MainWindow::parseInstruments(const QDomDocument &source)
{
    auto docRoot = source.documentElement();
    auto instrumentNodes = docRoot.elementsByTagName("inst");

    // Loop over instruments
    for (auto i = 0; i < instrumentNodes.count(); ++i)
    {
        auto instElement = instrumentNodes.item(i).toElement();

        // Get instrument name
        auto instrumentName = instElement.attribute("name");

        // Get instrument type
        auto instrumentType = Instrument::instrumentType(instElement.attribute("type", "Neutron"));

        auto &inst = instruments_.emplace_back(instrumentName, instrumentType);

        // Data locations
        inst.setJournalDirectory(instElement.attribute("journalDirectory"));
        inst.setDataDirectory(instElement.attribute("runDataRootUrl"));

        // If display columns are defined parse them now, otherwise assign defaults based on instrument
        auto columns = instElement.elementsByTagName("columns");
        // TODO
    }

    return true;
}

// Get default instrument complement
void MainWindow::getDefaultInstruments()
{
    QFile file(":/data/instruments.xml");
    if (!file.exists())
        throw(std::runtime_error("Internal instrument data not found.\n"));

    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    if (!parseInstruments(dom))
        throw(std::runtime_error("Couldn't parse internal instrument data.\n"));
}

/*
 * UI
 */

// Set current instrument
void MainWindow::on_InstrumentComboBox_currentIndexChanged(int index)
{
    // Need a valid journal source
    if (!currentJournalSource_ || !currentJournalSource().instrumentSubdirectories())
        return;
    auto &source = currentJournalSource();

    // Get the selected instrument
    if (index == -1)
    {
        source.setCurrentInstrument(std::nullopt);
        return;
    }
    else
        source.setCurrentInstrument(instruments_[index]);

    // Clear any mass search results since they're instrument-specific
    cachedMassSearch_.clear();

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.listJournals(currentJournalSource(), [=](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current instrument from active source
OptionalReferenceWrapper<const Instrument> MainWindow::currentInstrument() const
{
    if (!currentJournalSource_)
        return {};

    return currentJournalSource().currentInstrument();
}
