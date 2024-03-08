// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalSourceViewer and contributors

#pragma once

#include "journalSource.h"
#include "optionalRef.h"
#include <QAbstractListModel>

// Model for JournalSource definitions
class JournalSourceModel : public QAbstractListModel
{
    public:
    JournalSourceModel();

    private:
    // JournalSource data for the model
    OptionalReferenceWrapper<std::vector<std::unique_ptr<JournalSource>>> data_;
    // Whether to show availability as checkboxes
    bool showAvailability_{false};

    public:
    // Set the source data for the model
    void setData(OptionalReferenceWrapper<std::vector<std::unique_ptr<JournalSource>>> sources, bool showAvailability = false);
    // Get JournalSource at row specified
    JournalSource *getData(int row) const;
    // Get JournalSource at index specified
    JournalSource *getData(const QModelIndex &index) const;
    // Append new source to the end of the current data
    QModelIndex appendNew();

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
