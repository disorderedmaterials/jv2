// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "instrument.h"
#include "optionalRef.h"
#include <QAbstractListModel>

// Model for Instrument definitions
class InstrumentModel : public QAbstractListModel
{
    public:
    InstrumentModel();

    private:
    // Instrument source for the model
    OptionalReferenceWrapper<std::vector<Instrument>> data_;

    private:
    // Get Instrument at row specified
    OptionalReferenceWrapper<Instrument> getData(int row) const;
    // Get Instrument at index specified
    OptionalReferenceWrapper<Instrument> getData(const QModelIndex &index) const;

    public:
    // Set the source data for the model
    void setData(std::vector<Instrument> &instruments);

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
