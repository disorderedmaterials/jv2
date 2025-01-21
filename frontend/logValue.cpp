// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "logValue.h"

namespace JV2
{
LogValue::LogValue(const QString &name, const QString &location) : name_(name), neXuSLocation_(location) {}

/*
 * Basic Data
 */

// Return the name of the property
const QString &LogValue::name() const { return name_; }

// Return the NeXuS location path of the property
const QString &LogValue::neXuSLocation() const { return neXuSLocation_; }

// Return whether the property is selected
bool LogValue::isSelected() const { return selected_; }

// Set whether the property is selected
void LogValue::setSelected(bool selected) { selected_ = selected; }

/*
 * Run Data
 */

// Add data for specific run
void LogValue::addData(QString id, LogValueData data) { data_[id] = std::move(data); }

} // namespace JV2
