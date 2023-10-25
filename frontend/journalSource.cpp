// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journalSource.h"

// Return text string for specified IndexingType type
QString JournalSource::indexingType(JournalSource::IndexingType type)
{
    switch (type)
    {
        case (IndexingType::NetworkStatic):
            return "NetworkStatic";
        case (IndexingType::Generated):
            return "Generated";
        default:
            throw(std::runtime_error("IndexingType type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to IndexingType type
JournalSource::IndexingType JournalSource::indexingType(QString typeString)
{
    if (typeString.toLower() == "networkstatic")
        return IndexingType::NetworkStatic;
    else if (typeString.toLower() == "generated")
        return IndexingType::Generated;
    else
        throw(std::runtime_error("IndexingType string can't be converted to a IndexingType.\n"));
}

// Return text string for specified DataOrganisationType
QString JournalSource::dataOrganisationType(JournalSource::DataOrganisationType type)
{
    switch (type)
    {
        case (DataOrganisationType::Directory):
            return "Directory";
        case (DataOrganisationType::RBNumber):
            return "RBNumber";
        default:
            throw(std::runtime_error("DataOrganisationType not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to DataOrganisationType
JournalSource::DataOrganisationType JournalSource::dataOrganisationType(QString typeString)
{
    if (typeString.toLower() == "directory")
        return DataOrganisationType::Directory;
    else if (typeString.toLower() == "rbnumber")
        return DataOrganisationType::RBNumber;
    else
        throw(std::runtime_error("DataOrganisationType string can't be converted to a DataOrganisationType.\n"));
}

JournalSource::JournalSource(QString name, IndexingType type) : name_(name), type_(type) {}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &JournalSource::name() const { return name_; }

// Return type
JournalSource::IndexingType JournalSource::type() const { return type_; }

// Set whether this source has instrument subdirectories
void JournalSource::setInstrumentSubdirectories(bool b) { instrumentSubdirectories_ = b; }

// Return whether this source has instrument subdirectories
bool JournalSource::instrumentSubdirectories() const { return instrumentSubdirectories_; }

/*
 * Journal Data
 */

// Set journal data
void JournalSource::setJournalData(const QString &journalRootUrl, const QString &indexFilename)
{
    journalRootUrl_ = journalRootUrl;
    journalIndexFilename_ = indexFilename;
}

// Root URL for the journal source (if available)
const QString &JournalSource::journalRootUrl() const { return journalRootUrl_; }

// Return name of the index file in the main directories, if known
const QString &JournalSource::journalIndexFilename() const { return journalIndexFilename_; }

/*
 * Associated Run Data
 */

// Set run data location
void JournalSource::setRunDataLocation(const QString &runDataRootUrl, DataOrganisationType orgType)
{
    runDataRootUrl_ = runDataRootUrl;
    runDataOrganisation_ = orgType;
}

// Return root URL containing associated run data
const QString &JournalSource::runDataRootUrl() const { return runDataRootUrl_; }

// Return run data organisation
JournalSource::DataOrganisationType JournalSource::runDataOrganisation() const { return runDataOrganisation_; }

/*
 * State
 */

// Set current state of the journal source
void JournalSource::setState(JournalSourceState state) { state_ = state; }

// Return current state of the journal source
JournalSource::JournalSourceState JournalSource::state() const { return state_; }
