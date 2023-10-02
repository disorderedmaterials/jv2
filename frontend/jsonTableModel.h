// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "optionalRef.h"
#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QVector>

// Model for json usage in table view
class JsonTableModel : public QAbstractTableModel
{
    public:
    JsonTableModel();

    // Assigning custom data types for table headings
    typedef QMap<QString, QString> Heading;
    typedef QVector<Heading> Header;

    private:
    Header tableHeader_;
    Header tableHoldHeader_;
    Header tableGroupedHeader_;
    OptionalReferenceWrapper<const QJsonArray> jsonData_;
    QJsonArray tableHoldJsonData_;

    private:
    // Get Json data at index specified
    QJsonObject getData(const QModelIndex &index) const;

    public:
    // Set the source data to display
    void setData(const QJsonArray &array);
    // Set the table headers
    void setHeader(const Header &array);
    void groupData();
    void unGroupData();
    void setColumnTitle(int section, QString title);

    /*
     * QAbstractTableModel Overrides
     */
    public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};
