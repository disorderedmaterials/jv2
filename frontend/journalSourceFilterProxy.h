// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

namespace JV2
{
// Forward Declarations
class JournalSourceModel;

class JournalSourceFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    JournalSourceFilterProxy(JournalSourceModel &journalSourceModel);

    private:
    // Target model
    JournalSourceModel &journalSourceModel_;
    // Whether to show only those sources marked as available
    bool showAvailableOnly_{true};

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
} // namespace JV2
