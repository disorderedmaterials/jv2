// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>

// Data Source Definition
class DataSource
{
    public:
    // DataSource Types
    enum class DataSourceType
    {
        Network,
        Disk
    };
    // Return text string for specified DataSource type
    static QString dataSourceType(DataSourceType type);
    // Convert text string to DataSource type
    static DataSourceType dataSourceType(QString typeString);

    public:
    DataSource(QString name, DataSourceType type, QString rootUrl, QString networkDataDirectory = {});

    /*
     * Basic Data
     */
    private:
    // Name (used for display)
    QString name_;
    // Type
    DataSourceType type_;
    // Root URL for the data source
    QString rootUrl_;
    // Data directory for network sources
    QString networkDataDirectory_;
    // Whether the data is organised by ISIS instrument
    bool organisedByInstrument_{true};

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return type
    DataSourceType type() const;
    // Return root URL for the source
    const QString &rootUrl() const;
    // Return data directory for network sources
    const QString &networkDataDirectory() const;
    // Return whether the data is organised by ISIS instrument
    bool organisedByInstrument() const;
};
