// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
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
std::optional<QString> MainWindow::getRecentJournalSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    settings.beginGroup("Recent");

    currentJournalSource_ = findJournalSource(settings.value("Source").toString());
    if (!currentJournalSource_)
    {
        // In case the specified source isn't found, set it to the default / first one available
        if (journalSources_.empty())
            currentJournalSource_ = nullptr;
        else
            currentJournalSource_ = journalSources_.front().get();

        return {};
    }

    // Set up the rest of the source - instrument first, if relevant
    if (currentJournalSource_->instrumentRequired())
    {
        if (!settings.contains("Instrument"))
            return {};

        // Get the instrument and set the journals source here so we load relevant journals
        auto optInst = findInstrument(settings.value("Instrument").toString());
        currentJournalSource_->setCurrentInstrument(optInst.value_or(instruments_.front()));

        // If there was no valid instrument specified we can exit now
        if (!optInst)
            return {};
    }

    // Specific journal?  We can't set this directly, so need
    if (settings.contains("Journal"))
        return settings.value("Journal").toString();

    return {};
}

// Store user-defined journal sources
void MainWindow::storeUserJournalSources() const
{
    // Loop over sources
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.beginWriteArray("UserSources");
    auto index = 0;
    for (auto &source : journalSources_)
    {
        if (!source->isUserDefined())
            continue;

        settings.setArrayIndex(++index);

        // Basic information
        settings.setValue("Name", source->name());
        settings.setValue("Type", JournalSource::indexingType(source->type()));

        // Journal Data
        if (source->type() == JournalSource::IndexingType::Network)
        {
            settings.setValue("JournalRootUrl", source->journalRootUrl());
            settings.setValue("JournalIndexFilename", source->journalIndexFilename());
            settings.setValue("JournalInstrumentPathType",
                              Instrument::instrumentPathType(source->journalOrganisationByInstrument()));
        }
        else
        {
            settings.remove("JournalRootUrl");
            settings.remove("JournalIndexFilename");
            settings.remove("JournalInstrumentPathType");
        }

        // Run Data
        settings.setValue("RunDataRootUrl", source->runDataRootUrl());
        settings.setValue("RunDataInstrumentPathType",
                          Instrument::instrumentPathType(source->runDataOrganisationByInstrument()));

        // Generated Data Organisation
        if (source->type() == JournalSource::IndexingType::Generated)
            settings.setValue("RunDataOrganisation", JournalSource::dataOrganisationType(source->dataOrganisation()));
        else
            settings.remove("RunDataOrganisation");
    }
}

// Get user-defined journal sources
void MainWindow::getUserJournalSources() {}
