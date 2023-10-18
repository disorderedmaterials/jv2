// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "locator.h"
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
    private:
    // Name (used for display)
    QString name_;
    // Description
    QString description_;
    // Location
    Locator location_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return description
    const QString &description() const;
    // Set location
    void setLocation(const Locator &location);
    // Return location
    const Locator &location() const;

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
