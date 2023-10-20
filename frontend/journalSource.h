// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>

// Journal Source Definition
class JournalSource
{
    public:
    // JournalSource Types
    enum class JournalSourceType
    {
        ISISNetwork,
        Disk
    };
    // Return text string for specified JournalSource type
    static QString journalSourceType(JournalSourceType type);
    // Convert text string to JournalSource type
    static JournalSourceType journalSourceType(QString typeString);
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
    };

    public:
    JournalSource(QString name, JournalSourceType type, QString rootUrl, QString runDataDirectory, QString indexFile,
                  bool instrumentSubdirectories_, DataOrganisationType runDataOrganisation);

    /*
     * Basic Data
     */
    private:
    // Name (used for display)
    QString name_;
    // Type
    JournalSourceType type_;
    // Root URL for the journal source
    QString rootUrl_;
    // Directory containing associated run data
    QString runDataDirectory_;
    // Name of the index file in the main directories, if known
    QString indexFile_;
    // Whether this source has instrument subdirectories
    bool instrumentSubdirectories_{true};
    // Run data organisation
    DataOrganisationType runDataOrganisation_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return type
    JournalSourceType type() const;
    // Return root URL for the source
    const QString &rootUrl() const;
    // Return directory containing associated run data
    const QString &runDataDirectory() const;
    // Return name of the index file in the main directories, if known
    const QString &indexFile() const;
    // Return whether this source has instrument subdirectories
    bool instrumentSubdirectories() const;
    // Return run data organisation
    DataOrganisationType runDataOrganisation() const;

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
