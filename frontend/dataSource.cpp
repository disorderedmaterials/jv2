// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "dataSource.h"

// Return text string for specified DataSource type
QString DataSource::dataSourceType(DataSource::DataSourceType type)
{
    switch (type)
    {
        case (DataSourceType::ISISArchive):
            return "ISISArchive";
        case (DataSourceType::DiskByDirectory):
            return "DiskByDirectory";
        default:
            throw(std::runtime_error("DataSource type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to DataSource type
DataSource::DataSourceType DataSource::dataSourceType(QString typeString)
{
    if (typeString.toLower() == "isisarchive")
        return DataSourceType::ISISArchive;
    else if (typeString.toLower() == "diskbydirectory")
        return DataSourceType::DiskByDirectory;
    else
        throw(std::runtime_error("DataSource string can't be converted to an DataSourceType.\n"));
}

DataSource::DataSource(QString name, DataSourceType type, QString rootUrl, QString networkDataDirectory, QString indexFile)
    : name_(name), type_(type), rootUrl_(rootUrl), networkDataDirectory_(networkDataDirectory), indexFile_(indexFile)
{
}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &DataSource::name() const { return name_; }

// Return type
DataSource::DataSourceType DataSource::type() const { return type_; }

// Return root URL for the source
const QString &DataSource::rootUrl() const { return rootUrl_; }

// Return data directory for network sources
const QString &DataSource::networkDataDirectory() const { return networkDataDirectory_; }

// Return whether the data is organised by ISIS instrument
bool DataSource::organisedByInstrument() const { return organisedByInstrument_; }

// Return name of the index file in the main directories, if known
const QString &DataSource::indexFile() const { return indexFile_; }
