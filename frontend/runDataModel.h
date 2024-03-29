// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include "instrument.h"
#include "optionalRef.h"
#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QVector>

// JSON Run Data Model
class RunDataModel : public QAbstractTableModel
{
    public:
    RunDataModel();

    private:
    // journal source for the model
    OptionalReferenceWrapper<QJsonArray> runData_;
    OptionalReferenceWrapper<const Instrument::RunDataColumns> horizontalHeaders_;

    private:
    // Get Json data at row specified
    QJsonObject getData(int row) const;
    // Get Json data at index specified
    QJsonObject getData(const QModelIndex &index) const;

    public:
    // Set the source data for the model
    void setData(QJsonArray &array);
    // Append supplied data to the current data
    void appendData(const QJsonArray &newData);
    // Set the table column (horizontal) headers
    void setHorizontalHeaders(const Instrument::RunDataColumns &headers);
    // Get named data for specified row
    QString getData(const QString &targetData, int row) const;
    // Get named data for specified index
    QString getData(const QString &targetData, const QModelIndex &index) const;
    // Get index of specified run number (if it exists)
    const QModelIndex indexOfData(const QString &targetData, const QString &value) const;

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
