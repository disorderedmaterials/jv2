// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journal.h"
#include <QDomDocument>
#include <QFile>

Journal::Journal(QString name) : name_(name), location_(Journal::JournalLocation::ISISServer) {}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &Journal::name() const { return name_; }

// Return description
const QString &Journal::description() const { return description_; }

// Set location
void Journal::setFileLocation(Journal::JournalLocation location, QString url)
{
    location_ = location;
    locationURL_ = url;
}

// Return location type
Journal::JournalLocation Journal::location() const { return location_; }

// Return location UTL
const QString &Journal::locationURL() const { return locationURL_; }