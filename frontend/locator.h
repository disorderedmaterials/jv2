// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>

// Locator
class Locator
{
    public:
    Locator();
    Locator(const QString &rootUrl, const QString &directory, const QString &filename);

    private:
    // Root URL
    QString rootUrl_;
    // Directory within root URL
    QString directory_;
    // Filename
    QString filename_;

    public:
    // Return root URL
    const QString &rootUrl() const;
    // Return directory within root URL
    const QString &directory() const;
    // Return filename
    const QString &filename() const;
};
