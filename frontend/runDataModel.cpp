// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "runDataModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QTime>

// Model to handle json data in table view
JsonTableModel::JsonTableModel() : QAbstractTableModel() {}

/*
 * Private Functions
 */

// Get Json data at row specified
QJsonObject JsonTableModel::getData(int row) const
{
    if (!jsonData_)
        return {};

    return jsonData_->get()[row].toObject();
}

// Get Json data at index specified
QJsonObject JsonTableModel::getData(const QModelIndex &index) const { return getData(index.row()); }

/*
 * Public Functions
 */

// Set the source data for the model
void JsonTableModel::setData(QJsonArray &array)
{
    beginResetModel();
    jsonData_ = array;
    endResetModel();
}

// Append supplied data to the current data
void JsonTableModel::appendData(const QJsonArray &newData)
{
    if (!jsonData_)
        throw(std::runtime_error("Tried to append data in JsonTableModel but no current data reference is set.\n"));
    auto &currentData = jsonData_->get();

    beginInsertRows(QModelIndex(), currentData.size(), currentData.size() + newData.count() - 1);
    foreach (auto &item, newData)
        currentData.append(item);
    endInsertRows();
}

// Set the table column (horizontal) headers
void JsonTableModel::setHorizontalHeaders(const Instrument::RunDataColumns &headers)
{
    beginResetModel();
    horizontalHeaders_ = headers;
    endResetModel();
}

// Get named data for specified row
QString JsonTableModel::getData(const QString &targetData, int row) const
{
    auto data = getData(row);
    return data.contains(targetData) ? data[targetData].toString() : QString();
}

// Get data for specified index
QString JsonTableModel::getData(const QString &targetData, const QModelIndex &index) const
{
    return getData(targetData, index.row());
}

/*
 * QAbstractTableModel Overrides
 */

int JsonTableModel::rowCount(const QModelIndex &parent) const { return jsonData_ ? jsonData_->get().size() : 0; }

int JsonTableModel::columnCount(const QModelIndex &parent) const
{
    return horizontalHeaders_ ? horizontalHeaders_->get().size() : 0;
}

QVariant JsonTableModel::data(const QModelIndex &index, int role) const
{
    if (!jsonData_ || !horizontalHeaders_)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    const auto &headers = horizontalHeaders_->get();
    auto &[columnTitle, targetData] = headers[index.column()];

    // Get target data object
    QJsonObject obj = getData(index);

    // Search to see if the target data specified by the column exists in the object
    if (!obj.contains(targetData))
        return {};
    QJsonValue v = obj[targetData];

    // Format the value
    if (v.isDouble())
        return QString::number(v.toDouble());
    if (!v.isString())
        return {};

    // if title = Run Numbers then format (for grouped data)
    if (columnTitle == "Run Numbers")
    {
        // Format grouped runs display
        auto runArr = v.toString().split(";");
        if (runArr.size() == 1)
            return runArr[0];
        QString displayString = runArr[0];
        for (auto i = 1; i < runArr.size(); i++)
            if (runArr[i].toInt() == runArr[i - 1].toInt() + 1)
                displayString += "-" + runArr[i];
            else
                displayString += "," + runArr[i];
        QStringList splitDisplay;
        foreach (const auto &string, displayString.split(","))
        {
            if (string.contains("-"))
                splitDisplay.append(string.left(string.indexOf("-") + 1) +
                                    string.right(string.size() - string.lastIndexOf("-") - 1));
            else
                splitDisplay.append(string);
        }
        return splitDisplay.join(",");
    }
    return v.toString();
}

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!horizontalHeaders_)
        return {};

    if (orientation != Qt::Horizontal)
        return {};

    const auto &headers = horizontalHeaders_->get();

    switch (role)
    {
        case (Qt::UserRole):
            return headers[section].second;
        case (Qt::DisplayRole):
            return headers[section].first;
        default:
            return {};
    }
}
