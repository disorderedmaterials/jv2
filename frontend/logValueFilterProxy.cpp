// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "logValueFilterProxy.h"
#include "logValueModel.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

namespace JV2
{
LogValueFilterProxy::LogValueFilterProxy(LogValueModel &logValueModel) : logValueModel_(logValueModel)
{
    setSourceModel(&logValueModel_);
}

bool LogValueFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return (searchString_.isEmpty() || logValueModel_.getData(sourceRow)->get().name().contains(searchString_));
}

// Set search string
void LogValueFilterProxy::setSearchString(const QString &search)
{
    searchString_ = search;
    invalidate();
}

} // namespace JV2
