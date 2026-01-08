// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team LogValueGroupViewer and contributors

#pragma once

#include "logValueGroup.h"
#include "optionalRef.h"
#include <QAbstractListModel>

namespace JV2
{
// Model for LogValueGroup definitions
class LogValueGroupModel : public QAbstractListModel
{
    public:
    LogValueGroupModel();

    private:
    // LogValueGroup data for the model
    OptionalReferenceWrapper<std::vector<LogValueGroup>> data_;
    // Whether to show availability as checkboxes
    bool showAvailability_{false};

    public:
    // Set the source data for the model
    void setData(OptionalReferenceWrapper<std::vector<LogValueGroup>> properties);
    // Get LogValueGroup at row specified
    OptionalReferenceWrapper<LogValueGroup> getData(int row) const;
    // Get LogValueGroup at index specified
    OptionalReferenceWrapper<LogValueGroup> getData(const QModelIndex &index) const;

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
} // namespace JV2
