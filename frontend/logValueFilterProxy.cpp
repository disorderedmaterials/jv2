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

    // Deal with selected state first
    if (selectedValueBehaviour_ == SelectedStateBehaviour::HideSelected && logValue.isSelected())
        return false;
    else if (selectedValueBehaviour_ == SelectedStateBehaviour::ShowOnlySelected && !logValue.isSelected())
        return false;

    return (filterString_.isEmpty() || logValueModel_.getData(sourceRow)->get().name().contains(filterString_));
}

// Set filter string, or empty string to disable
void LogValueFilterProxy::setFilterString(const QString &search)
{
    filterString_ = search;
    invalidate();
}

// Set selected value behaviour
void LogValueFilterProxy::setSelectedStateBehaviour(SelectedStateBehaviour behaviour)
{
    selectedValueBehaviour_ = behaviour;
    invalidate();
}

} // namespace JV2
