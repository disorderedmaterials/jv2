// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QObject>
#include <QSortFilterProxyModel>

// Forward Declarations
class JsonTableModel;
class QModelIndex;

class JsonTableFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    JsonTableFilterProxy(JsonTableModel &jsonTableModel);

    private:
    // Target model
    JsonTableModel &jsonTableModel_;
    // Text string to filter by
    QString filterString_;
    // Whether the filtering is case sensitive
    bool caseSensitive_{false};

    public:
    // Set text string to filter by
    void setFilterString(QString filterString);
    // Set whether the filtering is case sensitive
    void setCaseSensitivity(bool caseSensitive);
    // Get named data for specified proxy index from underlying model
    QString getData(const QString &targetData, const QModelIndex &index) const;

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
