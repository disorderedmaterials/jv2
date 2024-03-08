// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalSourceViewer and contributors

#include "journalSourceModel.h"
#include "uniqueName.h"

// Model to handle json data in table view
JournalSourceModel::JournalSourceModel() : QAbstractListModel() {}

/*
 * Public Functions
 */

// Set the source data for the model
void JournalSourceModel::setData(OptionalReferenceWrapper<std::vector<std::unique_ptr<JournalSource>>> sources,
                                 bool showAvailability)
{
    beginResetModel();
    data_ = sources;
    showAvailability_ = showAvailability;
    endResetModel();
}

// Get JournalSource row specified
JournalSource *JournalSourceModel::getData(int row) const
{
    if (!data_ || row == -1 || row >= rowCount())
        return {};

    return data_->get()[row].get();
}

// Get JournalSource at index specified
JournalSource *JournalSourceModel::getData(const QModelIndex &index) const { return getData(index.row()); }

// Append new source to the end of the current data
QModelIndex JournalSourceModel::appendNew()
{
    if (!data_)
        return {};
    auto &currentData = data_->get();

    beginInsertRows(QModelIndex(), currentData.size(), currentData.size());
    currentData.emplace_back(
        std::make_unique<JournalSource>(uniqueName("NewSource", currentData, [](const auto &source) { return source->name(); }),
                                        JournalSource::IndexingType::Generated, true));
    endInsertRows();

    return index(currentData.size() - 1, 0);
}

// Remove the source at the specified index
void JournalSourceModel::remove(const QModelIndex &index)
{
    if (!data_)
        return;
    auto &currentData = data_->get();

    beginRemoveRows({}, index.row(), index.row());
    currentData.erase(currentData.begin() + index.row());
    endRemoveRows();
}

/*
 * QAbstractListModel Overrides
 */

int JournalSourceModel::rowCount(const QModelIndex &parent) const { return data_ ? data_->get().size() : 0; }

int JournalSourceModel::columnCount(const QModelIndex &parent) const { return 1; }

Qt::ItemFlags JournalSourceModel::flags(const QModelIndex &index) const
{
    auto *source = getData(index);

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (source->isUserDefined())
        flags.setFlag(Qt::ItemIsEditable);
    if (showAvailability_)
        flags.setFlag(Qt::ItemIsUserCheckable);

    return flags;
}

QVariant JournalSourceModel::data(const QModelIndex &index, int role) const
{
    if (!data_)
        return {};

    // Column zero is the only relevant one
    if (index.column() != 0)
        return {};

    switch (role)
    {
        case (Qt::DisplayRole):
        case (Qt::EditRole):
            return getData(index)->name();
        case (Qt::CheckStateRole):
            if (showAvailability_)
                return getData(index)->isAvailable() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            else
                return {};
        default:
            return {};
    }
}

bool JournalSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!data_)
        return false;

    // Column zero is the only relevant one
    if (index.column() != 0)
        return false;

    auto *source = getData(index);

    switch (role)
    {
        case (Qt::EditRole):
            // If this is not a user-defined source, no editing of the name is allowed
            if (!source->isUserDefined())
                return false;

            // Ensure uniqueness of name
            source->setName(uniqueName(value.toString(), data_->get(),
                                       [source](const auto &other) { return other.get() == source ? "" : other->name(); }));
            return true;
        case (Qt::CheckStateRole):
            source->setAvailable(value.value<Qt::CheckState>() == Qt::Checked);
            return true;
        default:
            return false;
    }

    return false;
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
