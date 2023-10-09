// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>
#include <map>

// Journal Definition
class Journal
{
    public:
    Journal(QString name);

    /*
     * Basic Data
     */
    public:
    // Journal Location
    enum class JournalLocation
    {
        ISISServer,
        Disk
    };

    private:
    // Name (used for display)
    QString name_;
    // Description
    QString description_;
    // Location Type
    JournalLocation location_;
    // Location URL
    QString locationURL_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return description
    const QString &description() const;
    // Set location
    void setFileLocation(JournalLocation location, QString url);
    // Return location type
    JournalLocation location() const;
    // Return location UTL
    const QString &locationURL() const;

    /*
     * Associated Run Data
     */
    public:
    // Run Data Range Typedef
    using RunDataRange = std::pair<int, std::optional<int>>;

    private:
    // Available run number information
    std::vector<RunDataRange> runData_;
    // Run data directory
    QString dataDirectory_;

    public:
    // Set data directory location
    void setDataDirectory(QString dataDir);
    // Return data directory
    const QString &dataDirectory() const;
};
