// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "jsonTableFilterProxy.h"
#include "jsonTableModel.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

JsonTableFilterProxy::JsonTableFilterProxy(JsonTableModel &jsonTableModel) : jsonTableModel_(jsonTableModel)
{
    setSourceModel(&jsonTableModel_);
}

// Set text string to filter by
void JsonTableFilterProxy::setFilterString(QString filterString)
{
    filterString_ = filterString;

    invalidateFilter();
}

// Set whether the filtering is case sensitive
void JsonTableFilterProxy::setCaseSensitivity(bool caseSensitive)
{
    caseSensitive_ = caseSensitive;

    invalidateFilter();
}

bool JsonTableFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
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
QString JsonTableFilterProxy::getData(const QString &targetData, const QModelIndex &index) const
{
    return jsonTableModel_.getData(targetData, mapToSource(index));
}