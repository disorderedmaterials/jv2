// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "jsonTableFilterProxy.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

void JsonTableFilterProxy::setFilterString(QString filterString) { filterString_ = filterString; }

QString JsonTableFilterProxy::filterString() const { return filterString_; }

void JsonTableFilterProxy::toggleCaseSensitivity(bool caseSensitive)
{
    caseSensitive_ = caseSensitive;
    emit updateFilter();
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
