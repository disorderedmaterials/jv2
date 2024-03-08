// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

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
    settings.beginGroup("UserSources");
    settings.beginWriteArray("Source", std::count_if(journalSources_.begin(), journalSources_.end(),
                                                     [](const auto &source) { return source->isUserDefined(); }));
    auto index = 0;
    for (auto &source : journalSources_)
    {
        if (!source->isUserDefined())
            continue;

        settings.setArrayIndex(index++);

        source->toSettings(settings);
    }
}

// Get user-defined journal sources
void MainWindow::getUserJournalSources()
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
                settings.value("Type", JournalSource::indexingType(JournalSource::IndexingType::Generated)).toString()),
            true));

        source->fromSettings(settings);
    }
}
