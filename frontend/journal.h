// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

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
    // Filename
    QString filename_;

    public:
    // Return name (used for display)
    const QString &name() const;
    // Return description
    const QString &description() const;
    // Set filename
    void setFilename(const QString &filename);
    // Return filename
    const QString &filename() const;
};
