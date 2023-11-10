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
    // Data Organisation Types
    enum class DataOrganisationType
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
    // JournalSource States
    enum JournalSourceState
    {
        _NoBackendError,
        _NoSourceError,
        Loading,
        OK,
        NetworkError,
        NoIndexFileError,
        NoJournalsError,
        RunDataScanInProgress,
        RunDataScanNoFilesError,
        JournalGenerationInProgress,
        JournalGenerationError
    };

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
    // Return name (used for display)
    const QString &name() const;
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
    // Set journal data
    void setJournalData(const QString &journalRootUrl, const QString &indexFilename);
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
    // Instrument-dependent run data organisation for this source
    Instrument::InstrumentPathType runDataOrganisationByInstrument_{Instrument::InstrumentPathType::None};
    // Currently selected instrument (if any)
    OptionalReferenceWrapper<const Instrument> currentInstrument_;

    public:
    // Set instrument-dependent journal organisation for this source
    void setJournalOrganisationByInstrument(Instrument::InstrumentPathType orgType);
    // Return instrument-dependent journal organisation for this source
    Instrument::InstrumentPathType journalOrganisationByInstrument() const;
    // Set instrument-dependent run data organisation for this source
    void setRunDataOrganisationByInstrument(Instrument::InstrumentPathType orgType);
    // Return instrument-dependent run data organisation for this source
    Instrument::InstrumentPathType runDataOrganisationByInstrument() const;
    // Set current instrument
    void setCurrentInstrument(OptionalReferenceWrapper<const Instrument> optInst);
    // Return current instrument
    OptionalReferenceWrapper<const Instrument> currentInstrument() const;

    /*
     * Associated Run Data
     */
    private:
    // Root URL containing associated run data
    QString runDataRootUrl_;
    // Run data organisation
    DataOrganisationType runDataOrganisation_;

    public:
    // Set run data location
    void setRunDataLocation(const QString &runDataRootUrl, DataOrganisationType orgType);
    // Return root URL containing associated run data
    const QString &runDataRootUrl() const;
    // Return run data organisation
    DataOrganisationType runDataOrganisation() const;

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
