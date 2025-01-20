// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team LogValueViewer and contributors

#pragma once

#include "logValue.h"
#include "optionalRef.h"
#include <QAbstractListModel>

namespace JV2
{
// Model for LogValue definitions
class LogValueModel : public QAbstractListModel
{
    public:
    LogValueModel();

    private:
    // LogValue data for the model
    OptionalReferenceWrapper<std::vector<LogValue>> data_;
    // Whether to show availability as checkboxes
    bool showAvailability_{false};

    public:
    // Set the source data for the model
    void setData(OptionalReferenceWrapper<std::vector<LogValue>> properties);
    // Get LogValue at row specified
    OptionalReferenceWrapper<LogValue> getData(int row) const;
    // Get LogValue at index specified
    OptionalReferenceWrapper<LogValue> getData(const QModelIndex &index) const;

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
