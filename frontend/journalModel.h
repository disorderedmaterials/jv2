// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "journal.h"
#include "optionalRef.h"
#include <QAbstractListModel>

// Model for Journal definitions
class JournalModel : public QAbstractListModel
{
    public:
    JournalModel();

    private:
    // Journal source for the model
    OptionalReferenceWrapper<std::vector<Journal>> data_;

    private:
    // Get Journal at row specified
    OptionalReferenceWrapper<Journal> getData(int row) const;
    // Get Journal at index specified
    OptionalReferenceWrapper<Journal> getData(const QModelIndex &index) const;

    public:
    // Set the source data for the model
    void setData(std::vector<Journal> &journals);

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
