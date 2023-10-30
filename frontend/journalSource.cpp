// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journalSource.h"
#include "instrument.h"
#include <QJsonArray>

// Return text string for specified IndexingType type
QString JournalSource::indexingType(JournalSource::IndexingType type)
{
    switch (type)
    {
        case (IndexingType::Network):
            return "Network";
        case (IndexingType::Cached):
            return "Cached";
        default:
            throw(std::runtime_error("IndexingType type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to IndexingType type
JournalSource::IndexingType JournalSource::indexingType(const QString &typeString)
{
    if (typeString.toLower() == "network")
        return IndexingType::Network;
    else if (typeString.toLower() == "cached")
        return IndexingType::Cached;
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
QString JournalSource::journalRootUrl() const { return journalRootUrl_; }

// Return name of the index file in the main directories, if known
QString JournalSource::journalIndexFilename() const
{
    return type_ == IndexingType::Cached ? "index.xml" : journalIndexFilename_;
}

// Clear current journals
void JournalSource::clearJournals()
{
    journals_.clear();
    currentJournal_ = std::nullopt;
}

// Set journals
void JournalSource::setJournals(const QJsonArray &journalData)
{
    clearJournals();

    for (auto i = journalData.count() - 1; i >= 0; i--)
    {
        auto value = journalData[i].toObject();

        auto &journal = journals_.emplace_back(value["display_name"].toString());
        journal.setLocation({value["server_root"].toString(), value["directory"].toString(), value["filename"].toString()});
    }

    // Set a current journal
    if (!journals_.empty())
        currentJournal_ = journals_.front();
}

// Return available journals
std::vector<Journal> &JournalSource::journals() { return journals_; }

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

// Set current journal being displayed by index
void JournalSource::setCurrentJournal(int index)
{
    if (index == -1 || index >= journals_.size())
        currentJournal_ = std::nullopt;
    else
        currentJournal_ = journals_[index];
}

// Return current journal
OptionalReferenceWrapper<Journal> JournalSource::currentJournal() const { return currentJournal_; }

/*
 * Instrument Subdirectories
 */

// Set whether this source has instrument subdirectories
void JournalSource::setInstrumentSubdirectories(bool b) { instrumentSubdirectories_ = b; }

// Return whether this source has instrument subdirectories
bool JournalSource::instrumentSubdirectories() const { return instrumentSubdirectories_; }

// Set current instrument
void JournalSource::setCurrentInstrument(OptionalReferenceWrapper<const Instrument> optInst) { currentInstrument_ = optInst; }

// Return current instrument
OptionalReferenceWrapper<const Instrument> JournalSource::currentInstrument() const { return currentInstrument_; }

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

/*
 * Object Data
 */

// Return basic object data ready for network request
QJsonObject JournalSource::sourceObjectData() const
{
    QJsonObject data;
    data["sourceID"] = name_;
    data["sourceType"] = indexingType(type_);
    data["journalRootUrl"] = journalRootUrl_;
    data["journalFilename"] = journalIndexFilename();
    if (instrumentSubdirectories_)
        data["directory"] = currentInstrument_ ? currentInstrument_->get().journalDirectory() : "UNKNOWN";
    data["runDataRootUrl"] = runDataRootUrl_;
    data["dataOrganisation"] = dataOrganisationType(runDataOrganisation_);
    return data;
}

// Return current journal data read for network request
QJsonObject JournalSource::currentJournalObjectData() const
{
    QJsonObject data;
    data["sourceID"] = name_;
    data["sourceType"] = indexingType(type_);
    data["journalRootUrl"] = journalRootUrl_;
    data["journalFilename"] = currentJournal_ ? currentJournal_->get().location().filename() : "UNKNOWN";
    if (instrumentSubdirectories_)
        data["directory"] = currentInstrument_ ? currentInstrument_->get().journalDirectory() : "UNKNOWN";
    data["runDataRootUrl"] = runDataRootUrl_;
    return data;
}
