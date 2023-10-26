// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "instrumentModel.h"

// Model to handle json data in table view
InstrumentModel::InstrumentModel() : QAbstractListModel() {}

/*
 * Private Functions
 */

// Get Instrument row specified
OptionalReferenceWrapper<Instrument> InstrumentModel::getData(int row) const
{
    if (!data_ || row == -1 || row >= rowCount())
        return {};

    return data_->get()[row];
}

// Get Instrument at index specified
OptionalReferenceWrapper<Instrument> InstrumentModel::getData(const QModelIndex &index) const { return getData(index.row()); }

/*
 * Public Functions
 */

// Set the source data for the model
void InstrumentModel::setData(std::vector<Instrument> &instruments)
{
    beginResetModel();
    data_ = instruments;
    endResetModel();
}

/*
 * QAbstractListModel Overrides
 */

int InstrumentModel::rowCount(const QModelIndex &parent) const { return data_ ? data_->get().size() : 0; }

int InstrumentModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant InstrumentModel::data(const QModelIndex &index, int role) const
{
    if (!data_)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    // Get target data object
    auto optInst = getData(index);
    const auto &inst = optInst->get();

    return inst.name();
}

QVariant InstrumentModel::headerData(int section, Qt::Orientation orientation, int role) const
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
