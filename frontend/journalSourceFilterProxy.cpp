// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "journalSourceFilterProxy.h"
#include "journalSourceModel.h"
#include <QModelIndex>
#include <QSortFilterProxyModel>

JournalSourceFilterProxy::JournalSourceFilterProxy(JournalSourceModel &journalSourceModel)
    : journalSourceModel_(journalSourceModel)
{
    setSourceModel(&journalSourceModel_);
}

bool JournalSourceFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return (!showAvailableOnly_ || journalSourceModel_.getData(sourceRow)->isAvailable());
}
