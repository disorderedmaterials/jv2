// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "locator.h"

Locator::Locator(){};

Locator::Locator(const QString &rootUrl, const QString &directory, const QString &filename)
    : rootUrl_(rootUrl), directory_(directory), filename_(filename)
{
}

/*
 * Public Functions
 */

// Return root URL
const QString &Locator::rootUrl() const { return rootUrl_; }

// Return directory within root URL
const QString &Locator::directory() const { return directory_; }

// Return filename
const QString &Locator::filename() const { return filename_; }
