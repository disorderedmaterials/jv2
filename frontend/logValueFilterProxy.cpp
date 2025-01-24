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
    const auto &logValue = logValueModel_.getData(sourceRow)->get();

    if (showSelectedOnly_ && !logValue.isSelected())
        return false;

    return (filterString_.isEmpty() || logValueModel_.getData(sourceRow)->get().name().contains(filterString_));
}

// Set filter string, or empty string to disable
void LogValueFilterProxy::setFilterString(const QString &search)
{
    filterString_ = search;
    invalidate();
}

// Set whether to show only selected log values
void LogValueFilterProxy::setShowSelectedOnly(bool selectedOnly)
{
    showSelectedOnly_ = selectedOnly;
    invalidate();
}

} // namespace JV2
