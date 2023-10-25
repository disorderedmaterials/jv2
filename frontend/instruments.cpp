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

// Fill instrument list
void MainWindow::fillInstruments()
{
    // Only allow calls after initial population
    instrumentsMenu_ = new QMenu("instrumentsMenu");
    journalsMenu_ = new QMenu("cyclesMenu");

    connect(ui_.instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_.instrumentButton->mapToGlobal(QPoint(0, ui_.instrumentButton->height()))); });
    connect(ui_.journalButton, &QPushButton::clicked,
            [=]() { journalsMenu_->exec(ui_.journalButton->mapToGlobal(QPoint(0, ui_.journalButton->height()))); });
    for (auto &inst : instruments_)
    {
        auto *action = new QAction(inst.name(), this);
        connect(action, &QAction::triggered, [=]() { setCurrentInstrument(inst.name()); });
        instrumentsMenu_->addAction(action);
    }
}

/*
 * UI
 */

// Set current instrument
void MainWindow::setCurrentInstrument(QString name)
{
    // Find the instrument specified
    auto instIt =
        std::find_if(instruments_.begin(), instruments_.end(), [name](const auto &inst) { return inst.name() == name; });
    if (instIt == instruments_.end())
        throw(std::runtime_error("Selected instrument does not exist!\n"));

    currentInstrument_ = *instIt;

    ui_.instrumentButton->setText(name);

    // Clear any mass search results since they're instrument-specific
    cachedMassSearch_.clear();

    // Make sure we have a valid journal source before we request any data
    if (!currentJournalSource_ || !currentJournalSource().instrumentSubdirectories())
        return;

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.listJournals(currentJournalSource(),
                          currentJournalSource().instrumentSubdirectories() ? currentInstrument().journalDirectory() : "",
                          [=](HttpRequestWorker *worker) { handleListJournals(worker); });
}

// Return current instrument
const Instrument &MainWindow::currentInstrument() const
{
    if (currentInstrument_)
        return currentInstrument_->get();

    throw(std::runtime_error("No current instrument defined.\n"));
}
