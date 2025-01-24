// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "logValueGroup.h"

namespace JV2
{
LogValueGroup::LogValueGroup(const QString &name, bool selected, Mildred::DisplayGroup *displayGroup)
    : name_(name), selected_(selected), displayGroup_(displayGroup)
{
}

/*
 * Basic Data
 */

// Return the name of the property
const QString &LogValueGroup::name() const { return name_; }

// Return the associated DisplayGroup (if set)
Mildred::DisplayGroup *LogValueGroup::displayGroup() const { return displayGroup_; }

// Return whether the property is selected
bool LogValueGroup::isSelected() const { return selected_; }

// Set whether the property is selected
void LogValueGroup::setSelected(bool selected) { selected_ = selected; }

} // namespace JV2
