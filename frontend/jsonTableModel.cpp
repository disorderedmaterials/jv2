// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "jsonTableModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QTime>

// Model to handle json data in table view
JsonTableModel::JsonTableModel() : QAbstractTableModel() {}

/*
 * Private Functions
 */

// Get Json data at index specified
QJsonObject JsonTableModel::getData(const QModelIndex &index) const
{
    if (!jsonData_)
        return {};

    return jsonData_->get()[index.row()].toObject();
}

/*
 * Public Functions
 */

// Set the source data for the model
void JsonTableModel::setData(const QJsonArray &array)
{
    beginResetModel();
    jsonData_ = array;
    endResetModel();
}

// Set the table column (horizontal) headers
void JsonTableModel::setHorizontalHeaders(const Header &array)
{
    beginResetModel();
    horizontalHeaders_ = array;
    endResetModel();
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

    auto headers = horizontalHeaders_->get();

    QJsonObject obj = getData(index);
    const QString &key = headers[index.column()]["index"];
    if (!obj.contains(key))
        return {};
    QJsonValue v = obj[key];

    if (v.isDouble())
        return QString::number(v.toDouble());
    if (!v.isString())
        return {};

    // if title = Run Numbers then format (for grouped data)
    if (headers[index.column()]["title"] == "Run Numbers")
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

    auto headers = horizontalHeaders_->get();

    if (role == Qt::UserRole)
        return headers[section]["index"]; // Index == database name

    if (role != Qt::DisplayRole)
        return {};

    switch (orientation)
    {
        case Qt::Horizontal:
            return headers[section]["title"]; // Title == desired display name
        case Qt::Vertical:
            // return section + 1;
            return {};
        default:
            return {};
    }
}
