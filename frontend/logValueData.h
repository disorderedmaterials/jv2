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
    LogValueData(const QDateTime &start, const QDateTime &end, const std::vector<double> &times,
                 const std::vector<double> &values);

    private:
    // Start and end times
    QDateTime startTime_, endTime_;
    // Time points in seconds relative to startTime_
    std::vector<double> times_;
    // Values
    std::vector<double> values_;

    public:
    // Return time points in seconds relative to startTime_
    const std::vector<double> &times() const;
    // Return values
    const std::vector<double> &values() const;
};
} // namespace JV2
