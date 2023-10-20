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
        DiskByDirectory
    };
    // Return text string for specified JournalSource type
    static QString journalSourceType(JournalSourceType type);
    // Convert text string to JournalSource type
    static JournalSourceType journalSourceType(QString typeString);
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
                  bool organisedByInstrument);

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
    // Whether this source is organised by ISIS instrument
    bool organisedByInstrument_{true};
    // Current state of the journal source
    JournalSourceState state_{JournalSourceState::Loading};

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
    // Return whether this source is organised by ISIS instrument
    bool organisedByInstrument() const;
    // Set current state of the journal source
    void setState(JournalSourceState state);
    // Return current state of the journal source
    JournalSourceState state() const;
};
