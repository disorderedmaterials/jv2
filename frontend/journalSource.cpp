// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

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
        case (IndexingType::Generated):
            return "Generated";
        default:
            throw(std::runtime_error("IndexingType type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to IndexingType type
JournalSource::IndexingType JournalSource::indexingType(const QString &typeString)
{
    if (typeString.toLower() == "network")
        return IndexingType::Network;
    else if (typeString.toLower() == "generated")
        return IndexingType::Generated;
    else
        throw(std::runtime_error("IndexingType string can't be converted to a IndexingType.\n"));
}

JournalSource::JournalSource(QString name, IndexingType type, bool userDefined)
    : name_(name), type_(type), userDefined_(userDefined)
{
}

/*
 * Basic Data
 */

// Set name
void JournalSource::setName(const QString &name) { name_ = name; }

// Return name
const QString &JournalSource::name() const { return name_; }

// Set type
void JournalSource::setType(IndexingType type) { type_ = type; }

// Return type
JournalSource::IndexingType JournalSource::type() const { return type_; }

// Return whether the source is user-defined
bool JournalSource::isUserDefined() const { return userDefined_; }

// Set whether the source should be available for use
void JournalSource::setAvailable(bool available) { available_ = available; }

// Return whether the source should be available for use
bool JournalSource::isAvailable() const { return available_; }

/*
 * Journal Data
 */

// Set journal data
void JournalSource::setJournalLocation(const QString &journalRootUrl, const QString &indexFilename)
{
    journalRootUrl_ = journalRootUrl;
    journalIndexFilename_ = indexFilename;
}

// Root URL for the journal source (if available)
QString JournalSource::journalRootUrl() const { return journalRootUrl_; }

// Return name of the index file in the main directories, if known
QString JournalSource::journalIndexFilename() const
{
    return type_ == IndexingType::Generated ? "index.xml" : journalIndexFilename_;
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
        journal.setFilename(value["filename"].toString());
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
 * Instrument Organisation
 */

// Return whether the source requires an instrument to be specified
bool JournalSource::instrumentRequired() const
{
    return journalOrganisationByInstrument_ != Instrument::PathType::None ||
           runDataOrganisationByInstrument_ != Instrument::PathType::None;
}

// Set instrument-dependent journal organisation for this source
void JournalSource::setJournalOrganisationByInstrument(Instrument::PathType pathType, bool upperCased)
{
    journalOrganisationByInstrument_ = pathType;
    journalOrganisationByInstrumentUpperCased_ = upperCased;
}

// Return instrument-dependent journal organisation for this source
Instrument::PathType JournalSource::journalOrganisationByInstrument() const { return journalOrganisationByInstrument_; }

// Return whether the instrument path component for journals should be uppercased
bool JournalSource::isJournalOrganisationByInstrumentUpperCased() const { return journalOrganisationByInstrumentUpperCased_; }

// Set instrument-dependent run data organisation for this source
void JournalSource::setRunDataOrganisationByInstrument(Instrument::PathType pathType, bool upperCased)
{
    runDataOrganisationByInstrument_ = pathType;
    runDataOrganisationByInstrumentUpperCased_ = upperCased;
}

// Return instrument-dependent run data organisation for this source
Instrument::PathType JournalSource::runDataOrganisationByInstrument() const { return runDataOrganisationByInstrument_; }

// Return whether the instrument path component for run data should be uppercased
bool JournalSource::isRunDataOrganisationByInstrumentUpperCased() const { return runDataOrganisationByInstrumentUpperCased_; }

// Set current instrument
void JournalSource::setCurrentInstrument(OptionalReferenceWrapper<const Instrument> optInst) { currentInstrument_ = optInst; }

// Return current instrument
OptionalReferenceWrapper<const Instrument> JournalSource::currentInstrument() const { return currentInstrument_; }

/*
 * Source ID
 */

// Return our source ID
QString JournalSource::sourceID() const
{
    return instrumentRequired() ? QString("%1/%2").arg(name_, currentInstrument()->get().name()) : name_;
}

/*
 * Associated Run Data
 */

// Set run data location
void JournalSource::setRunDataLocation(const QString &runDataRootUrl) { runDataRootUrl_ = runDataRootUrl; }

// Return root URL containing associated run data
const QString &JournalSource::runDataRootUrl() const { return runDataRootUrl_; }

// Set regular expression to select directories directly under the root URL
void JournalSource::setRunDataRootRegExp(const QString &regexp) { runDataRootRegExp_ = regexp; }

// Return regular expression to select directories directly under the root URL
const QString &JournalSource::runDataRootRegExp() const { return runDataRootRegExp_; }

/*
 * Generated Data Organisation
 */

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

// Return sort key associated to specified DataOrganisationType
QString JournalSource::dataOrganisationTypeSortKey(JournalSource::DataOrganisationType type)
{
    switch (type)
    {
        case (DataOrganisationType::Directory):
            return "data_directory";
        case (DataOrganisationType::RBNumber):
            return "experiment_identifier";
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

// Set run data organisation type
void JournalSource::setDataOrganisation(JournalSource::DataOrganisationType orgType) { dataOrganisation_ = orgType; }

// Return run data organisation
JournalSource::DataOrganisationType JournalSource::dataOrganisation() const { return dataOrganisation_; }

/*
 * Object Data
 */

// Return basic object data ready for network request
QJsonObject JournalSource::sourceObjectData() const
{
    QJsonObject data;
    // Basic source information
    data["sourceID"] = name_;
    data["sourceType"] = indexingType(type_);

    // Journal Location
    if (type_ == IndexingType::Network)
    {
        data["journalRootUrl"] =
            journalOrganisationByInstrument_ != Instrument::PathType::None && currentInstrument_
                ? QString("%1/%2").arg(journalRootUrl_,
                                       currentInstrument_->get().pathComponent(journalOrganisationByInstrument_,
                                                                               journalOrganisationByInstrumentUpperCased_))
                : journalRootUrl_;
    }
    data["journalFilename"] = journalIndexFilename();

    // Current instrument
    if (currentInstrument_)
        data["instrument"] = currentInstrument_->get().name();

    // Run data location
    data["runDataRootUrl"] =
        runDataOrganisationByInstrument_ != Instrument::PathType::None && currentInstrument_
            ? QString("%1/%2").arg(runDataRootUrl_,
                                   currentInstrument_->get().pathComponent(runDataOrganisationByInstrument_,
                                                                           runDataOrganisationByInstrumentUpperCased_))
            : runDataRootUrl_;

    return data;
}

// Return current journal data read for network request
QJsonObject JournalSource::currentJournalObjectData() const
{
    QJsonObject data = sourceObjectData();
    data["journalFilename"] = currentJournal_ ? currentJournal_->get().filename() : "UNKNOWN";
    return data;
}

/*
 * State
 */

// Set current state of the journal source
void JournalSource::setState(JournalSourceState state) { state_ = state; }

// Return current state of the journal source
JournalSource::JournalSourceState JournalSource::state() const { return state_; }

// Flag that the source is showing searched data
void JournalSource::setShowingSearchedData() { journalBeforeSearchedData_ = currentJournal_; }

// Flag that the source should return to showing journal data
void JournalSource::stopShowingSearchedData()
{
    if (journalBeforeSearchedData_)
        currentJournal_ = findJournal(journalBeforeSearchedData_->get().name());

    journalBeforeSearchedData_ = std::nullopt;
}

// Return whether the source is currently showing searched data
bool JournalSource::showingSearchedData() const { return journalBeforeSearchedData_.has_value(); }

/*
 * Settings Storage
 */

// Store data in the supplied QSettings
void JournalSource::toSettings(QSettings &settings) const
{
    // Basic information
    settings.setValue("Name", name_);
    settings.setValue("Type", JournalSource::indexingType(type_));

    // Journal Data
    if (type_ == JournalSource::IndexingType::Network)
    {
        settings.setValue("JournalRootUrl", journalRootUrl_);
        settings.setValue("JournalIndexFilename", journalIndexFilename_);
        settings.setValue("JournalPathType", Instrument::pathType(journalOrganisationByInstrument_));
        settings.setValue("JournalPathTypeUpperCased", journalOrganisationByInstrumentUpperCased_);
    }
    else
    {
        settings.remove("JournalRootUrl");
        settings.remove("JournalIndexFilename");
        settings.remove("JournalPathType");
    }

    // Run Data
    settings.setValue("RunDataRootUrl", runDataRootUrl_);
    settings.setValue("RunDataRootRegExp", runDataRootRegExp_);
    settings.setValue("RunDataPathType", Instrument::pathType(runDataOrganisationByInstrument_));
    settings.setValue("RunDataPathTypeUpperCased", runDataOrganisationByInstrumentUpperCased_);

    // Generated Data Organisation
    if (type_ == JournalSource::IndexingType::Generated)
        settings.setValue("DataOrganisation", JournalSource::dataOrganisationType(dataOrganisation_));
    else
        settings.remove("DataOrganisation");
}

// Retrieve data from the supplied QSettings
void JournalSource::fromSettings(const QSettings &settings)
{
    // Journal Data
    if (type_ == JournalSource::IndexingType::Network)
    {
        journalRootUrl_ = settings.value("JournalRootUrl").toString();
        journalIndexFilename_ = settings.value("JournalIndexFilename").toString();

        journalOrganisationByInstrument_ = Instrument::pathType(
            settings.value("JournalPathType", Instrument::pathType(Instrument::PathType::None)).toString());
        journalOrganisationByInstrumentUpperCased_ = settings.value("JournalPathTypeUpperCased").toBool();
    }

    // Run Data
    runDataRootUrl_ = settings.value("RunDataRootUrl").toString();
    runDataRootRegExp_ = settings.value("RunDataRootRegExp").toString();
    runDataOrganisationByInstrument_ =
        Instrument::pathType(settings.value("RunDataPathType", Instrument::pathType(Instrument::PathType::None)).toString());
    runDataOrganisationByInstrumentUpperCased_ = settings.value("RunDataPathTypeUpperCased").toBool();

    // Generated Data Organisation
    if (type_ == JournalSource::IndexingType::Generated)
    {
        dataOrganisation_ = JournalSource::dataOrganisationType(
            settings
                .value("DataOrganisation", JournalSource::dataOrganisationType(JournalSource::DataOrganisationType::Directory))
                .toString());
    }
}
