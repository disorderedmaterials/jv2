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

        // Create the instrument (with optional alternative name)
        if (instElement.hasAttribute("altName"))
            instruments_.emplace_back(instrumentName, instElement.attribute("altName"), instrumentType);
        else
            instruments_.emplace_back(instrumentName, std::nullopt, instrumentType);

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

// Find instrument with supplied name
OptionalReferenceWrapper<const Instrument> MainWindow::findInstrument(const QString &name) const
{
    auto instIt =
        std::find_if(instruments_.begin(), instruments_.end(), [&name](const auto &inst) { return inst.name() == name; });

    if (instIt == instruments_.end())
        return {};

    return *instIt;
}

/*
 * UI
 */

// Set current instrument
void MainWindow::on_InstrumentComboBox_currentIndexChanged(int index)
{
    if (controlsUpdating_)
        return;

    // Need a valid journal source
    if (!currentJournalSource_ || !currentJournalSource_->instrumentRequired())
        return;

    // Get the selected instrument
    if (index == -1)
    {
        currentJournalSource_->setCurrentInstrument(std::nullopt);
        return;
    }
    else
        currentJournalSource_->setCurrentInstrument(instruments_[index]);

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournalIndex(currentJournalSource(), [&](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current instrument from active source
OptionalReferenceWrapper<const Instrument> MainWindow::currentInstrument() const
{
    if (!currentJournalSource_)
        return {};

    return currentJournalSource_->currentInstrument();
}
