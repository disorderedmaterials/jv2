// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>
#include <map>

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
    // Return text string for specified instrument type
    static QString instrumentType(InstrumentType type);
    // Convert text string to instrument type
    static InstrumentType instrumentType(QString typeString);

    public:
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
     * Run Data Columns
     */
    public:
    using RunDataColumn = std::pair<QString, QString>;
    using RunDataColumns = std::vector<std::pair<QString, QString>>;

    private:
    // Default columns for instrument types
    static std::map<InstrumentType, RunDataColumns> defaultColumns_;
    // Custom columns for this instrument
    RunDataColumns customColumns_;

    public:
    // Get default instrument columns
    static void getDefaultColumns();
    // Return whether the instrument has a custom column definition
    bool hasCustomColumns() const;
    // Get run data columns to use for this instrument
    const std::vector<RunDataColumn> &runDataColumns() const;

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
