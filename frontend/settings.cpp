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
            auto &source = currentJournalSource();
            settings.beginGroup("Recent");
            settings.setValue("Source", source.name());
            if (source.currentInstrument())
                settings.setValue("Instrument", source.currentInstrument()->get().name());
            if (source.currentJournal())
                settings.setValue("Journal", source.currentJournal()->get().name());
        }
    }
}

// Get recent journal settings
std::optional<QString> MainWindow::getRecentJournalSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.beginGroup("Recent");
    if (!settings.contains("Source"))
        return {};
    qDebug() << settings.value("Source").toString();
    auto optSource = findJournalSource(settings.value("Source").toString());
    if (!optSource)
    {
        // In case the specified source isn't found, set it to the default / first one available
        if (journalSources_.empty())
            currentJournalSource_ = std::nullopt;
        else
            currentJournalSource_ = journalSources_.front();

        return {};
    }

    auto &source = optSource->get();
    currentJournalSource_ = source;

    // Set up the rest of the source - instrument first, if relevant
    if (source.instrumentSubdirectories())
    {
        if (!settings.contains("Instrument"))
            return {};

        // Get the instrument and set the journals source here so we load relevant journals
        auto optInst = findInstrument(settings.value("Instrument").toString());
        source.setCurrentInstrument(optInst);
        qDebug() << settings.value("Instrument").toString();

        // If there was no valid instrument specified we can exit now
        if (!optInst)
            return {};
    }

    // Specific journal?  We can't set this directly, so need
    if (settings.contains("Journal"))
        return settings.value("Journal").toString();

    return {};
}
