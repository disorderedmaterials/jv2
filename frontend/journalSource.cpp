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
        case (JournalSourceType::Disk):
            return "Disk";
        default:
            throw(std::runtime_error("JournalSource type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to JournalSource type
JournalSource::JournalSourceType JournalSource::journalSourceType(const QString &typeString)
{
    if (typeString.toLower() == "isisnetwork")
        return JournalSourceType::ISISNetwork;
    else if (typeString.toLower() == "disk")
        return JournalSourceType::Disk;
    else
        throw(std::runtime_error("JournalSource string can't be converted to a JournalSourceType.\n"));
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

JournalSource::JournalSource(QString name, JournalSourceType type, QString rootUrl, QString runDataDirectory, QString indexFile,
                             bool instrumentSubdirectories, DataOrganisationType runDataOrganisation)
    : name_(name), type_(type), rootUrl_(rootUrl), runDataDirectory_(runDataDirectory), indexFile_(indexFile),
      instrumentSubdirectories_(instrumentSubdirectories), runDataOrganisation_(runDataOrganisation)
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

// Return whether this source has instrument subdirectories
bool JournalSource::instrumentSubdirectories() const { return instrumentSubdirectories_; }

// Return run data organisation
JournalSource::DataOrganisationType JournalSource::runDataOrganisation() const { return runDataOrganisation_; }

/*
 * State
 */

// Set current state of the journal source
void JournalSource::setState(JournalSourceState state) { state_ = state; }

// Return current state of the journal source
JournalSource::JournalSourceState JournalSource::state() const { return state_; }

/*
 * Journals
 */

// Clear current journals
void JournalSource::clearJournals()
{
    journals_.clear();
    currentJournal_ = std::nullopt;
}

// Add new journal
Journal &JournalSource::addJournal(const QString &name, const Locator &location)
{
    auto &journal = journals_.emplace_back(name);
    journal.setLocation(location);

    return journal;
}

// Return available journals
const std::vector<Journal> &JournalSource::journals() const { return journals_; }

// Find named journal
OptionalReferenceWrapper<Journal> JournalSource::findJournal(const QString &name)
{
    auto journalIt =
        std::find_if(journals_.begin(), journals_.end(), [name](const auto &journal) { return journal.name() == name; });

    if (journalIt == journals_.end())
        return std::nullopt;

    return *journalIt;
}

// Set current journal being displayed
void JournalSource::setCurrentJournal(QString name)
{
    // Find the journal specified
    auto optJournal = findJournal(name);
    if (!optJournal)
        throw(std::runtime_error("Selected journal does not exist!\n"));

    currentJournal_ = *optJournal;
}

// Return current journal
OptionalReferenceWrapper<Journal> JournalSource::currentJournal() const { return currentJournal_; }
