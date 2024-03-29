// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "instrument.h"
#include <QDomDocument>
#include <QFile>

// Static Singleton
// -- Default columns for instrument types
std::map<Instrument::InstrumentType, Instrument::RunDataColumns> Instrument::defaultColumns_;

// Return text string for specified instrument type
QString Instrument::instrumentType(Instrument::InstrumentType type)
{
    switch (type)
    {
        case (InstrumentType::Neutron):
            return "Neutron";
        case (InstrumentType::Muon):
            return "Muon";
        default:
            throw(std::runtime_error("Instrument type not known and can't be converted to a QString.\n"));
    }
}

// Convert text string to instrument type
Instrument::InstrumentType Instrument::instrumentType(QString typeString)
{
    if (typeString.toLower() == "neutron")
        return InstrumentType::Neutron;
    else if (typeString.toLower() == "muon")
        return InstrumentType::Muon;
    else
        throw(std::runtime_error("Instrument string can't be converted to an InstrumentType.\n"));
}

Instrument::Instrument(QString name, std::optional<QString> altName, InstrumentType type, bool userDefined)
    : name_(name), alternativeName_(altName), type_(type), userDefined_(userDefined)
{
}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &Instrument::name() const { return name_; }

// Return type
Instrument::InstrumentType Instrument::type() const { return type_; }

// Return whether this instrument is user-defined
bool Instrument::userDefined() const { return userDefined_; }

/*
 * Run Data Columns
 */

// Get default instrument columns
void Instrument::getDefaultColumns()
{
    QFile file(":/data/defaultColumns.xml");
    if (!file.exists())
        throw(std::runtime_error("Internal default columns data not found.\n"));

    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto docRoot = dom.documentElement();
    auto dcNodes = docRoot.elementsByTagName("defaultColumns");

    // Loop over sets of default columns
    for (auto i = 0; i < dcNodes.count(); ++i)
    {
        auto columnsElement = dcNodes.item(i).toElement();

        // Get instrument type to which the columns apply
        auto instType = Instrument::instrumentType(columnsElement.attribute("type"));
        auto &columnVector = defaultColumns_[instType];

        // Get columns
        auto columns = columnsElement.elementsByTagName("column");
        for (auto c = 0; c < columns.count(); ++c)
        {
            auto colElement = columns.item(c).toElement();
            columnVector.emplace_back(colElement.attribute("name"), colElement.attribute("data"));
        }
    }
}

// Return whether the instrument has a custom column definition
bool Instrument::hasCustomColumns() const { return !customColumns_.empty(); }

// Get run data columns to use for this instrument
const Instrument::RunDataColumns &Instrument::runDataColumns() const
{
    // If there are no defined custom columns, return the defaults instead
    return customColumns_.empty() ? defaultColumns_[type_] : customColumns_;
}

// Return default columns for specified instrument type
const Instrument::RunDataColumns &Instrument::runDataColumns(Instrument::InstrumentType type) { return defaultColumns_[type]; }

/*
 * Paths
 */

// Return text string for specified instrument path type
QString Instrument::pathType(Instrument::PathType type)
{
    switch (type)
    {
        case (PathType::None):
            return "None";
        case (PathType::Name):
            return "Name";
        case (PathType::NDXName):
            return "NDXName";
        case (PathType::AltNDXName):
            return "AltNDXName";
        default:
            throw(std::runtime_error("Can't convert instrument path type to string.\n"));
    }
}

// Convert text string to instrument path type
Instrument::PathType Instrument::pathType(const QString &typeString)
{
    if (typeString.toLower() == "none")
        return PathType::None;
    else if (typeString.toLower() == "name")
        return PathType::Name;
    else if (typeString.toLower() == "ndxname")
        return PathType::NDXName;
    else if (typeString.toLower() == "altndxname")
        return PathType::AltNDXName;
    else
        throw(std::runtime_error("Can't convert string to instrument path type.\n"));
}

// Return specified path component for this instrument
QString Instrument::pathComponent(PathType pathType, bool upperCased) const
{
    QString result;
    switch (pathType)
    {
        case (PathType::None):
            return {};
        case (PathType::Name):
            result = name_;
            break;
        case (PathType::NDXName):
            result = QString("ndx%1").arg(name_);
            break;
        case (PathType::AltNDXName):
            result = QString("ndx%1").arg(alternativeName_.value_or(name_));
            break;
        default:
            return "UNKNOWN_PATH_TYPE";
    }
    return upperCased ? result.toUpper() : result.toLower();
}
