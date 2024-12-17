// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "args.h"
#include "mainWindow.h"
#include <QCommandLineParser>
#include <QDomDocument>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>

/*
 * Private Functions
 */

// Save custom column settings
void MainWindow::saveCustomColumnSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    // Save customised column views
    QDomDocument customColumns;
    for (const auto &inst : instruments_)
    {
        if (!inst.hasCustomColumns())
            continue;
    }
}

// Store recent journal settings
void MainWindow::storeRecentJournalSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    if (currentJournalSource_)
    {
        settings.remove("Recent");
        if (currentJournalSource_)
        {
            settings.beginGroup("Recent");
            settings.setValue("Source", currentJournalSource_->name());
            if (currentJournalSource_->currentInstrument())
                settings.setValue("Instrument", currentJournalSource_->currentInstrument()->get().name());
            if (currentJournalSource_->currentJournal())
                settings.setValue("Journal", currentJournalSource_->currentJournal()->get().name());
        }
    }
}

// Get recent journal settings
void MainWindow::getRecentJournalSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    settings.beginGroup("Recent");

    // Establish if we have a recent Source, and if it is valid / enabled
    auto lastJournalSource = findJournalSource(settings.value("Source").toString());
    if (!lastJournalSource || !lastJournalSource->isAvailable())
    {
        auto it = std::find_if(journalSources_.begin(), journalSources_.end(),
                               [](const auto &source) { return source->isAvailable(); });
        if (it != journalSources_.end())
            lastJournalSource = it->get();
    }
    if (!lastJournalSource)
    {
        setCurrentJournalSource(nullptr);
        return;
    }

    // Set up the rest of the source - instrument first, if relevant
    if (lastJournalSource->instrumentRequired())
    {
        if (settings.contains("Instrument"))
        {
            // Get the instrument and set the journals source here so we load relevant journals
            auto optInst = findInstrument(settings.value("Instrument").toString());
            if (optInst)
                lastJournalSource->setCurrentInstrument(*optInst);
            else
            {
                setCurrentJournalSource(lastJournalSource);
                return;
            }
        }
    }

    // Finally, check for a specific journal to go to
    setCurrentJournalSource(lastJournalSource, settings.contains("Journal") ? settings.value("Journal").toString() : QString());
}

// Store journal sources in settings
void MainWindow::storeJournalSourcesToSettings() const
{
    // Loop over sources
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.beginGroup("Sources");
    settings.beginWriteArray("Source", journalSources_.size());
    auto index = 0;
    for (auto &source : journalSources_)
    {
        settings.setArrayIndex(index++);

        source->toSettings(settings);
    }
}

// Get journal sources from settings
void MainWindow::getJournalSourcesFromSettings(QCommandLineParser &cliParser)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.beginGroup("Sources");
    auto nSources = settings.beginReadArray("Source");
    for (auto index = 0; index < nSources; ++index)
    {
        settings.setArrayIndex(index);
        auto &source = journalSources_.emplace_back(std::make_unique<JournalSource>(
            settings.value("Name", "NewSource").toString(),
            JournalSource::indexingType(
                settings.value("Type", JournalSource::indexingType(JournalSource::IndexingType::Generated)).toString())));

        source->fromSettings(settings);
    }

    // Add default sources if not found
    // -- The main ISIS Archive
    if (!findJournalSource("ISIS Archive"))
    {
        auto &isisArchive =
            journalSources_.emplace_back(std::make_unique<JournalSource>("ISIS Archive", JournalSource::IndexingType::Network));
        isisArchive->setJournalOrganisationByInstrument(Instrument::PathType::AltNDXName);
        isisArchive->setRunDataOrganisationByInstrument(Instrument::PathType::NDXName, true);
        isisArchive->setJournalLocation("http://data.isis.rl.ac.uk/journals", "journal_main.xml");
        isisArchive->setRunDataLocation(settings
                                            .value("ISISArchiveDataUrl", cliParser.isSet(CLIArgs::ISISArchiveDirectory)
                                                                             ? cliParser.value(CLIArgs::ISISArchiveDirectory)
                                                                             : "/archive")
                                            .toString());
    }
    // -- IDAaaS RB Directories
    if (!findJournalSource("IDAaaS Data Cache"))
    {
        auto &idaaasDataCache = journalSources_.emplace_back(
            std::make_unique<JournalSource>("IDAaaS Data Cache", JournalSource::IndexingType::Generated));
        idaaasDataCache->setRunDataOrganisationByInstrument(Instrument::PathType::Name, true);
        idaaasDataCache->setRunDataLocation("/mnt/ceph/instrument_data_cache");
        idaaasDataCache->setDataOrganisation(JournalSource::DataOrganisationType::RBNumber);
        idaaasDataCache->setRunDataRootRegExp("^[0-9]+");
    }

    // Handle CLI options modifying default sources
    auto *isisArchive = findJournalSource("ISIS Archive");
    if (cliParser.isSet(CLIArgs::ISISArchiveDirectory))
    {
        isisArchive->setRunDataLocation(cliParser.value(CLIArgs::ISISArchiveDirectory));
    }
    if (cliParser.isSet(CLIArgs::HideISISArchive))
        isisArchive->setAvailable(false);
    auto *idaaasDataCache = findJournalSource("IDAaaS Data Cache");
    if (cliParser.isSet(CLIArgs::HideIDAaaS))
        idaaasDataCache->setAvailable(false);
}
