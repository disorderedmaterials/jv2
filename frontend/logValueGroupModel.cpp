// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team LogValueGroupViewer and contributors

#include "logValueGroupModel.h"
#include <QIcon>
#include <QImage>

namespace JV2
{
// Model to handle json data in table view
LogValueGroupModel::LogValueGroupModel() : QAbstractListModel() {}

/*
 * Public Functions
 */

// Set the source data for the model
void LogValueGroupModel::setData(OptionalReferenceWrapper<std::vector<LogValueGroup>> properties)
{
    beginResetModel();
    data_ = properties;
    endResetModel();
}

// Get LogValueGroup row specified
OptionalReferenceWrapper<LogValueGroup> LogValueGroupModel::getData(int row) const
{
    if (!data_ || row == -1 || row >= rowCount())
        return {};

    return data_->get()[row];
}

// Get LogValueGroup at index specified
OptionalReferenceWrapper<LogValueGroup> LogValueGroupModel::getData(const QModelIndex &index) const
{
    return getData(index.row());
}

/*
 * QAbstractListModel Overrides
 */

int LogValueGroupModel::rowCount(const QModelIndex &parent) const { return data_ ? data_->get().size() : 0; }

int LogValueGroupModel::columnCount(const QModelIndex &parent) const { return 1; }

Qt::ItemFlags LogValueGroupModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant LogValueGroupModel::data(const QModelIndex &index, int role) const
{
    if (!data_)
        return {};

    // Column zero is the only relevant one
    if (index.column() != 0)
        return {};

    auto optData = getData(index);
    if (!optData)
        return {};
    auto &data = optData->get();

    switch (role)
    {
        case (Qt::DisplayRole):
        case (Qt::EditRole):
            return data.name();
        case (Qt::DecorationRole):
            if (data.displayGroup())
                return data.displayGroup()->colourPolicyIcon({16, 16});
            else
                return {};
        case (Qt::CheckStateRole):
            return data.isSelected() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
        default:
            return {};
    }
}

bool LogValueGroupModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!data_)
        return false;

    // Column zero is the only relevant one
    if (index.column() != 0)
        return false;

    auto optData = getData(index);
    if (!optData)
        return {};
    auto &data = optData->get();

    if (role != Qt::CheckStateRole)
        return false;

    data.setSelected(value.value<Qt::CheckState>() == Qt::Checked);

    emit(dataChanged(index, index));

    return true;
}

QVariant LogValueGroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    return "Log Value";
}
} // namespace JV2
