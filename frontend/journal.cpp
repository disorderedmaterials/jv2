// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "journal.h"

Journal::Journal(QString name) : name_(name) {}

/*
 * Basic Data
 */

// Return name (used for display)
const QString &Journal::name() const { return name_; }

// Return description
const QString &Journal::description() const { return description_; }

// Set filename
void Journal::setFilename(const QString &filename) { filename_ = filename; }

// Return filename
const QString &Journal::filename() const { return filename_; }
