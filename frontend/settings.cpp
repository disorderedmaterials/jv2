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
void MainWindow::getRecentJournalSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.beginGroup("Recent");
    if (!settings.contains("Source"))
        return;
    currentJournalSource_ = findJournalSource(settings.value("Source").toString());
    if (!currentJournalSource_)
    {
        // In case the specified source isn't found, set it to the first one available
        
    }
    if (source.currentInstrument())
        settings.setValue("Instrument", source.currentInstrument()->get().name());
    if (source.currentJournal())
        settings.setValue("Journal", source.currentJournal()->get().name());
}
