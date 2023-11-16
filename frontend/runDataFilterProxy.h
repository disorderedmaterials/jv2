// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QObject>
#include <QSortFilterProxyModel>

// Forward Declarations
class RunDataModel;
class QModelIndex;

class RunDataFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    RunDataFilterProxy(RunDataModel &runDataModel);

    private:
    // Target model
    RunDataModel &runDataModel_;
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
