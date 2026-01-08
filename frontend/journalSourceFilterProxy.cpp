// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#include "journalSourceFilterProxy.h"
#include "journalSourceModel.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

namespace JV2
{
JournalSourceFilterProxy::JournalSourceFilterProxy(JournalSourceModel &journalSourceModel)
    : journalSourceModel_(journalSourceModel)
{
    setSourceModel(&journalSourceModel_);
}

bool JournalSourceFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return (!showAvailableOnly_ || journalSourceModel_.getData(sourceRow)->isAvailable());
}
} // namespace JV2
