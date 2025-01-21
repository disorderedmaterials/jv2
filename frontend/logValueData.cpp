// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "logValueData.h"

namespace JV2
{
LogValueData::LogValueData() {}
LogValueData::LogValueData(const QDateTime &start, const QDateTime &end, const std::vector<double> &times,
                           const std::vector<double> &values)
    : startTime_(start), endTime_(end), times_(times), values_(values)
{
}

// Return time points in seconds since startTime_
const std::vector<double> &LogValueData::times() const { return times_; }

// Return values
const std::vector<double> &LogValueData::values() const { return values_; }

} // namespace JV2
