// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journalSource.h"

// Return text string for specified JournalSource type
QString JournalSource::journalSourceType(JournalSource::JournalSourceType type)
{
    switch (type)
    {
        case (JournalSourceType::ISISNetwork):
            return "ISISArchive";
        case (JournalSourceType::DiskByDirectory):
            return "DiskByDirectory";
        default:
            throw(std::runtime_error("JournalSource type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to JournalSource type
JournalSource::JournalSourceType JournalSource::journalSourceType(QString typeString)
{
    if (typeString.toLower() == "isisnetwork")
        return JournalSourceType::ISISNetwork;
    else if (typeString.toLower() == "diskbydirectory")
        return JournalSourceType::DiskByDirectory;
    else
        throw(std::runtime_error("JournalSource string can't be converted to a JournalSourceType.\n"));
}

JournalSource::JournalSource(QString name, JournalSourceType type, QString rootUrl, QString runDataDirectory, QString indexFile,
                             bool organisedByInstrument)
    : name_(name), type_(type), rootUrl_(rootUrl), runDataDirectory_(runDataDirectory), indexFile_(indexFile),
      organisedByInstrument_(organisedByInstrument)
{
}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &JournalSource::name() const { return name_; }

// Return type
JournalSource::JournalSourceType JournalSource::type() const { return type_; }

// Return root URL for the source
const QString &JournalSource::rootUrl() const { return rootUrl_; }

// Return directory containing associated run data
const QString &JournalSource::runDataDirectory() const { return runDataDirectory_; }

// Return name of the index file in the main directories, if known
const QString &JournalSource::indexFile() const { return indexFile_; }

// Return whether the data is organised by ISIS instrument
bool JournalSource::organisedByInstrument() const { return organisedByInstrument_; }

// Set current state of the journal source
void JournalSource::setState(JournalSourceState state) { state_ = state; }

// Return current state of the journal source
JournalSource::JournalSourceState JournalSource::state() const { return state_; }