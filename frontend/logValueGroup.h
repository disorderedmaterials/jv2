// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "logValueData.h"
#include <QString>

namespace JV2
{
// Log Value Group
class LogValueGroup
{
    public:
    LogValueGroup(const QString &name, bool selected = false);
    ~LogValueGroup() = default;

    /*
     * Basic Data
     */
    private:
    // Display name of the group
    QString name_;
    // Whether this group is selected
    bool selected_{false};

    public:
    // Return the name of the property
    const QString &name() const;
    // Return whether the property is selected
    bool isSelected() const;
    // Set whether the property is selected
    void setSelected(bool selected);
};
} // namespace JV2
