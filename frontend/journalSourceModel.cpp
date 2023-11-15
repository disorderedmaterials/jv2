// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalSourceViewer and contributors

#include "journalSourceModel.h"
#include "uniqueName.h"

// Model to handle json data in table view
JournalSourceModel::JournalSourceModel() : QAbstractListModel() {}

/*
 * Private Functions
 */

// Get JournalSource row specified
JournalSource *JournalSourceModel::getData(int row) const
{
    if (!data_ || row == -1 || row >= rowCount())
        return {};

    return data_->get()[row].get();
}

// Get JournalSource at index specified
JournalSource *JournalSourceModel::getData(const QModelIndex &index) const { return getData(index.row()); }

/*
 * Public Functions
 */

// Set the source data for the model
void JournalSourceModel::setData(OptionalReferenceWrapper<std::vector<std::unique_ptr<JournalSource>>> sources)
{
    beginResetModel();
    data_ = sources;
    endResetModel();
}

// Append new source to the end of the current data
QModelIndex JournalSourceModel::appendNew()
{
    if (!data_)
        return {};
    auto &currentData = data_->get();

    beginInsertRows(QModelIndex(), currentData.size(), currentData.size());
    currentData.emplace_back(
        std::make_unique<JournalSource>(uniqueName("NewSource", currentData, [](const auto &source) { return source->name(); }),
                                        JournalSource::IndexingType::Generated));
    endInsertRows();

    return index(currentData.size() - 1, 0);
}

/*
 * QAbstractListModel Overrides
 */

int JournalSourceModel::rowCount(const QModelIndex &parent) const { return data_ ? data_->get().size() : 0; }

int JournalSourceModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant JournalSourceModel::data(const QModelIndex &index, int role) const
{
    if (!data_)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    // Get target data
    return getData(index)->name();
}

QVariant JournalSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
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
