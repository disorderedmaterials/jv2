// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "journal.h"
#include "optionalRef.h"
#include <QJsonObject>
#include <QString>

// Forward Declarations
class HttpRequestWorker;
class Instrument;

// Journal Source Definition
class JournalSource
{
    public:
    // Indexing Types
    enum class IndexingType
    {
        Network,
        Cached
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
    // Convert text string to DataOrganisationType
    static DataOrganisationType dataOrganisationType(QString typeString);
    // JournalSource States
    enum JournalSourceState
    {
        _NoBackendError,
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
    JournalSource(QString name, IndexingType type);

    /*
     * Basic Data
     */
    private:
    // Name (used for display)
    QString name_;
    // Type
    IndexingType type_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return type
    IndexingType type() const;

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
    const QString &journalRootUrl() const;
    // Return journal index filename
    const QString &journalIndexFilename() const;
    // Clear current journals
    void clearJournals();
    // Add new journal
    Journal &addJournal(const QString &name, const Locator &location);
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
     * Instrument Subdirectories
     */
    private:
    // Whether this source has instrument subdirectories
    bool instrumentSubdirectories_{false};
    // Currently selected instrument (if any)
    OptionalReferenceWrapper<const Instrument> currentInstrument_;

    public:
    // Set whether this source has instrument subdirectories
    void setInstrumentSubdirectories(bool b);
    // Return whether this source has instrument subdirectories
    bool instrumentSubdirectories() const;
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

    public:
    // Set current state of the journal source
    void setState(JournalSourceState state);
    // Return current state of the journal source
    JournalSourceState state() const;
};
