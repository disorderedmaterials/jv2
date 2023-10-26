// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "journalModel.h"

// Model to handle json data in table view
JournalModel::JournalModel() : QAbstractListModel() {}

/*
 * Private Functions
 */

// Get Journal row specified
OptionalReferenceWrapper<Journal> JournalModel::getData(int row) const
{
    if (!data_ || row == -1 || row >= rowCount())
        return {};

    return data_->get()[row];
}

// Get Journal at index specified
OptionalReferenceWrapper<Journal> JournalModel::getData(const QModelIndex &index) const { return getData(index.row()); }

/*
 * Public Functions
 */

// Set the source data for the model
void JournalModel::setData(std::vector<Journal> &journals)
{
    beginResetModel();
    data_ = journals;
    endResetModel();
}

/*
 * QAbstractListModel Overrides
 */

int JournalModel::rowCount(const QModelIndex &parent) const { return data_ ? data_->get().size() : 0; }

int JournalModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant JournalModel::data(const QModelIndex &index, int role) const
{
    if (!data_)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    // Get target data object
    auto optJournal = getData(index);
    const auto &journal = optJournal->get();

    return journal.name();
}

QVariant JournalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role)
    {
        case (Qt::DisplayRole):
            return "Name";
        default:
            return {};
    }
}
