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
    JsonTableModel();

    // Assigning custom data types for table headings
    typedef QMap<QString, QString> Heading;
    typedef QVector<Heading> Header;

    private:
    Header tableHeader_;
    Header tableHoldHeader_;
    Header tableGroupedHeader_;
    QJsonArray tableJsonData_;
    QJsonArray tableHoldJsonData_;

    public:
    bool setJson(const QJsonArray &array);
    bool setHeader(const Header &array);
    QJsonObject getJsonObject(const QModelIndex &index) const; // get row data
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
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
};
