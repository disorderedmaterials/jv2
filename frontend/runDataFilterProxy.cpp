// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "runDataFilterProxy.h"
#include "runDataModel.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

RunDataFilterProxy::RunDataFilterProxy(RunDataModel &runDataModel) : runDataModel_(runDataModel)
{
    setSourceModel(&runDataModel_);
}

// Set text string to filter by
void RunDataFilterProxy::setFilterString(QString filterString)
{
    filterString_ = filterString;

    invalidateFilter();
}

// Set whether the filtering is case sensitive
void RunDataFilterProxy::setCaseSensitivity(bool caseSensitive)
{
    caseSensitive_ = caseSensitive;

    invalidateFilter();
}

bool RunDataFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (filterString_.isEmpty())
        return true;

    auto filterString = filterString_;
    if (!caseSensitive_)
        filterString = filterString.toLower();

    auto accept = false;
    for (auto i = 0; i < sourceModel()->columnCount(); i++)
    {
        auto index = sourceModel()->index(sourceRow, i, sourceParent);
        auto tableData = sourceModel()->data(index).toString();
        if (!caseSensitive_)
            tableData = tableData.toLower();

        if (tableData.contains(filterString))
            return true;
    }

    return false;
}

// Get named data for specified proxy index from underlying model
QString RunDataFilterProxy::getData(const QString &targetData, const QModelIndex &index) const
{
    return runDataModel_.getData(targetData, mapToSource(index));
}
