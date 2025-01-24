// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

namespace JV2
{
// Forward Declarations
class LogValueModel;

class LogValueFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    LogValueFilterProxy(LogValueModel &journalSourceModel);

    private:
    // Target model
    LogValueModel &logValueModel_;
    // Search string
    QString filterString_;
    // Whether to show only selected log values
    bool showSelectedOnly_{false};

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    public:
    // Set filter string, or empty string to disable
    void setFilterString(const QString &search);
    // Set whether to show only selected log values
    void setShowSelectedOnly(bool selectedOnly);
};
} // namespace JV2
