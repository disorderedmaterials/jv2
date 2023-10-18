// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journal.h"

Journal::Journal(QString name) : name_(name) {}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &Journal::name() const { return name_; }

// Return description
const QString &Journal::description() const { return description_; }

// Set location
void Journal::setLocation(const Locator &location) { location_ = location; }

// Return location
const Locator &Journal::location() const { return location_; }
