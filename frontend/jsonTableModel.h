// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

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
    // Assigning custom data types for table headings
    typedef QMap<QString, QString> Heading;
    typedef QVector<Heading> Header;
    JsonTableModel();

    bool setJson(const QJsonArray &array);
    QJsonArray getJson();
    bool setHeader(const Header &array);
    Header getHeader();

    QJsonObject getJsonObject(const QModelIndex &index) const; // get row data

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void groupData();
    void unGroupData();
    void setColumnTitle(int section, QString title);
    bool setData(const QModelIndex &index, QJsonObject rowData, int role = Qt::EditRole);
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());

    private:
    Header tableHeader_;
    Header tableHoldHeader_;
    Header tableGroupedHeader_;
    QJsonArray tableJsonData_;
    QJsonArray tableHoldJsonData_;
};
