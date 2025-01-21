// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "logValueData.h"

namespace JV2
{
LogValueData::LogValueData() {}
LogValueData::LogValueData(const QDateTime &start, const QDateTime &end, const std::vector<double> &epochTimes,
                           const std::vector<double> &values)
    : startTime_(start), endTime_(end), epochTimes_(epochTimes), values_(values)
{
}

} // namespace JV2
