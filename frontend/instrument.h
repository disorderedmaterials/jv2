// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>

// Instrument Definition
class Instrument
{
    public:
    // Instrument Types
    enum class InstrumentType
    {
        Neutron,
        Muon
    };
    Instrument(QString name, InstrumentType type, bool userDefined = false);

    /*
     * Basic Data
     */
    private:
    // Name (used for display)
    QString name_;
    // Type
    InstrumentType type_;
    // Whether this instrument is user-defined
    bool userDefined_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return lower cased name
    const QString lowerCaseName() const;
    // Return type
    InstrumentType type() const;
    // Return whether this instrument is user-defined
    bool userDefined() const;

    /*
     * Additional Information
     */
    private:
    // Run data directory
    QString dataDirectory_;

    public:
    // Set run data directory
    void setDataDirectory(QString dataDir);
    // Return run data directory
    const QString &dataDirectory() const;
};
