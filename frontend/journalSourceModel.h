// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalSourceViewer and contributors

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
    // JournalSource source for the model
    OptionalReferenceWrapper<std::vector<JournalSource>> data_;

    public:
    // Set the source data for the model
    void setData(OptionalReferenceWrapper<std::vector<JournalSource>> sources);
    // Get JournalSource at row specified
    OptionalReferenceWrapper<JournalSource> getData(int row) const;
    // Get JournalSource at index specified
    OptionalReferenceWrapper<JournalSource> getData(const QModelIndex &index) const;

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};