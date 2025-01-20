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
    QString searchString_;

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    public:
    // Set search string
    void setSearchString(const QString &search);
};
} // namespace JV2
