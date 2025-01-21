// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "logValueData.h"
#include <QString>

namespace JV2
{
// Log Property Definition
class LogValue
{
    public:
    LogValue(const QString &name, const QString &location = {});
    ~LogValue() = default;

    /*
     * Basic Data
     */
    private:
    // Display name of the property
    QString name_;
    // NeXuS location of the property (if relevant)
    QString neXuSLocation_;
    // Whether this property is selected
    bool selected_{false};

    public:
    // Return the name of the property
    const QString &name() const;
    // Return the NeXuS location path of the property
    const QString &neXuSLocation() const;
    // Return whether the property is selected
    bool isSelected() const;
    // Set whether the property is selected
    void setSelected(bool selected);

    /*
     * Run Data
     */
    private:
    // Log value data per-run
    std::map<QString, LogValueData> data_;

    public:
    // Add data for specific run
    void addData(QString id, LogValueData data);
};
} // namespace JV2
