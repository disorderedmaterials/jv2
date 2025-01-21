// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "logValueData.h"
#include <QDateTime>

namespace JV2
{
// Log value data
class LogValueData
{
    public:
    LogValueData();
    LogValueData(const QDateTime &start, const QDateTime &end, const std::vector<double> &epochTimes,
                 const std::vector<double> &values);

    private:
    // Start and end times
    QDateTime startTime_, endTime_;
    // Time values in milliseconds since epoch
    std::vector<double> epochTimes_;
    // Values
    std::vector<double> values_;
};
} // namespace JV2
