// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "instrument.h"
#include "journal.h"
#include "optionalRef.h"
#include <QJsonObject>
#include <QString>

// Forward Declarations
class HttpRequestWorker;

// Journal Source Definition
class JournalSource
{
    public:
    // Indexing Types
    enum class IndexingType
    {
        Network,
        Generated
    };
    // Return text string for specified IndexingType type
    static QString indexingType(IndexingType type);
    // Convert text string to IndexingType type
    static IndexingType indexingType(const QString &typeString);

    public:
    JournalSource(QString name, IndexingType type, bool userDefined = false);

    /*
     * Basic Data
     */
    private:
    // Name (used for display)
    QString name_;
    // Type
    IndexingType type_;
    // Whether the source is user-defined
    bool userDefined_{false};

    public:
    // Set name
    void setName(const QString &name);
    // Return name
    const QString &name() const;
    // Set type
    void setType(IndexingType type);
    // Return type
    IndexingType type() const;
    // Return whether the source is user-defined
    bool isUserDefined() const;

    /*
     * Journal Data
     */
    public:
    // Root URL for the journal source (if available)
    QString journalRootUrl_;
    // Journal index filename
    QString journalIndexFilename_;
    // Available journals
    std::vector<Journal> journals_;
    // Currently selected journal (if any)
    OptionalReferenceWrapper<Journal> currentJournal_;

    public:
    // Set journal location
    void setJournalLocation(const QString &journalRootUrl, const QString &indexFilename);
    // Root URL for the journal source (if available)
    QString journalRootUrl() const;
    // Return journal index filename
    QString journalIndexFilename() const;
    // Clear current journals
    void clearJournals();
    // Set journals
    void setJournals(const QJsonArray &journalData);
    // Return available journals
    std::vector<Journal> &journals();
    // Find named journal
    OptionalReferenceWrapper<Journal> findJournal(const QString &name);
    // Set current journal being displayed by name
    void setCurrentJournal(QString name);
    // Set current journal being displayed by index
    void setCurrentJournal(int index);
    // Return current journal
    OptionalReferenceWrapper<Journal> currentJournal() const;

    /*
     * Instrument Organisation
     */
    private:
    // Instrument-dependent journal organisation for this source
    Instrument::InstrumentPathType journalOrganisationByInstrument_{Instrument::InstrumentPathType::None};
    bool journalOrganisationByInstrumentUpperCased_{false};
    // Instrument-dependent run data organisation for this source
    Instrument::InstrumentPathType runDataOrganisationByInstrument_{Instrument::InstrumentPathType::None};
    bool runDataOrganisationByInstrumentUpperCased_{false};
    // Currently selected instrument (if any)
    OptionalReferenceWrapper<const Instrument> currentInstrument_;

    public:
    // Return whether the source requires an instrument to be specified
    bool instrumentRequired() const;
    // Set instrument-dependent journal organisation for this source
    void setJournalOrganisationByInstrument(Instrument::InstrumentPathType orgType, bool upperCased = false);
    // Return instrument-dependent journal organisation for this source
    Instrument::InstrumentPathType journalOrganisationByInstrument() const;
    // Return whether the instrument path component for journals should be uppercased
    bool isJournalOrganisationByInstrumentUppercased() const;
    // Set instrument-dependent run data organisation for this source
    void setRunDataOrganisationByInstrument(Instrument::InstrumentPathType orgType, bool upperCased = false);
    // Return instrument-dependent run data organisation for this source
    Instrument::InstrumentPathType runDataOrganisationByInstrument() const;
    // Return whether the instrument path component for run data should be uppercased
    bool isRunDataOrganisationByInstrumentUppercased() const;
    // Set current instrument
    void setCurrentInstrument(OptionalReferenceWrapper<const Instrument> optInst);
    // Return current instrument
    OptionalReferenceWrapper<const Instrument> currentInstrument() const;

    /*
     * Source ID
     */
    public:
    // Return our source ID
    QString sourceID() const;

    /*
     * Associated Run Data
     */
    private:
    // Root URL containing associated run data
    QString runDataRootUrl_;

    public:
    // Set run data location
    void setRunDataLocation(const QString &runDataRootUrl);
    // Return root URL containing associated run data
    const QString &runDataRootUrl() const;

    /*
     * Generated Data Organisation
     */
    public:
    // Data Organisation Types
    enum DataOrganisationType
    {
        Directory,
        RBNumber
    };
    // Return text string for specified DataOrganisationType
    static QString dataOrganisationType(DataOrganisationType type);
    // Return sort key associated to specified DataOrganisationType
    static QString dataOrganisationTypeSortKey(JournalSource::DataOrganisationType type);
    // Convert text string to DataOrganisationType
    static DataOrganisationType dataOrganisationType(QString typeString);

    private:
    // Run data organisation
    DataOrganisationType dataOrganisation_{DataOrganisationType::Directory};

    public:
    // Set run data organisation type
    void setDataOrganisation(DataOrganisationType orgType);
    // Return run data organisation
    DataOrganisationType dataOrganisation() const;

    /*
     * Object Data
     */
    public:
    // Return basic source data ready for network request
    QJsonObject sourceObjectData() const;
    // Return current journal data read for network request
    QJsonObject currentJournalObjectData() const;

    /*
     * State
     */
    public:
    // JournalSource States
    enum JournalSourceState
    {
        Loading,
        OK,
        Generating,
        Error
    };

    private:
    // Current state of the journal source
    JournalSourceState state_{JournalSourceState::Loading};
    // Last journal displayed before showing searched data
    OptionalReferenceWrapper<Journal> journalBeforeSearchedData_;

    public:
    // Set current state of the journal source
    void setState(JournalSourceState state);
    // Return current state of the journal source
    JournalSourceState state() const;
    // Flag that the source is showing searched data
    void setShowingSearchedData();
    // Flag that the source should return to showing journal data
    void stopShowingSearchedData();
    // Return whether the source is currently showing searched data
    bool showingSearchedData() const;
};
