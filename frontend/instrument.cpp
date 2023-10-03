// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "instrument.h"

Instrument::Instrument(QString name, InstrumentType type, bool userDefined)
    : name_(name), type_(type), userDefined_(userDefined)
{
}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &Instrument::name() const { return name_; }

// Return lower cased name
const QString Instrument::lowerCaseName() const { return name_.toLower(); }

// Return type
Instrument::InstrumentType Instrument::type() const { return type_; }

// Return whether this instrument is user-defined
bool Instrument::userDefined() const { return userDefined_; }

/*
 * Additional Information
 */

// Set run data directory
void Instrument::setDataDirectory(QString dataDir) { dataDirectory_ = dataDir; }

// Return run data directory
const QString &Instrument::dataDirectory() const { return dataDirectory_; }