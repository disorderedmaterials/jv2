// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "jsonTableModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QTime>

// Model to handle json data in table view
JsonTableModel::JsonTableModel() : QAbstractTableModel()
{
    tableGroupedHeader_.push_back(Heading({{"title", "Title"}, {"index", "title"}}));
    tableGroupedHeader_.push_back(Heading({{"title", "Total Duration"}, {"index", "duration"}}));
    tableGroupedHeader_.push_back(Heading({{"title", "Run Numbers"}, {"index", "run_number"}}));
}

// Sets json data to populate table
bool JsonTableModel::setJson(const QJsonArray &array)
{
    beginResetModel();
    tableJsonData_ = array;
    endResetModel();
    return true;
}

// Sets header_ data to define table
bool JsonTableModel::setHeader(const Header &array)
{
    beginResetModel();
    tableHeader_ = array;
    endResetModel();
    return true;
}

// Get row data
QJsonObject JsonTableModel::getJsonObject(const QModelIndex &index) const
{
    if (!tableJsonData_)
        return {};

    return tableJsonData_->get()[index.row()].toObject();
}

// Returns grouped table data
void JsonTableModel::groupData()
{
    if (!tableJsonData_)
        return;

    QJsonArray groupedJson;
    auto &data = tableJsonData_->get();

    // holds data in tuple as QJson referencing is incomplete
    std::vector<std::tuple<QString, QString, QString>> groupedData;
    for (QJsonValue value : data)
    {
        const QJsonObject &valueObj = value.toObject();
        bool unique = true;

        // add duplicate title data to stack
        for (std::tuple<QString, QString, QString> &data : groupedData)
        {
            if (std::get<0>(data) == valueObj["title"].toString())
            {
                auto currentTotal = QTime::fromString(std::get<1>(data), "HH:mm:ss");
                // convert duration to seconds
                auto newTime = QTime(0, 0, 0).secsTo(QTime::fromString(valueObj["duration"].toString(), "HH:mm:ss"));
                auto totalRunTime = currentTotal.addSecs(newTime).toString("HH:mm:ss");
                std::get<1>(data) = QString(totalRunTime);
                std::get<2>(data) += ";" + valueObj["run_number"].toString();
                unique = false;
                break;
            }
        }
        if (unique)
            groupedData.emplace_back(valueObj["title"].toString(), valueObj["duration"].toString(),
                                     valueObj["run_number"].toString());
    }
    for (std::tuple<QString, QString, QString> data : groupedData)
    {
        auto groupData = QJsonObject({qMakePair(QString("title"), QJsonValue(std::get<0>(data))),
                                      qMakePair(QString("duration"), QJsonValue(std::get<1>(data))),
                                      qMakePair(QString("run_number"), QJsonValue(std::get<2>(data)))});
        groupedJson.push_back(QJsonValue(groupData));
    }

    // Hold ungrouped values
    tableHoldJsonData_ = tableJsonData_->get();
    tableHoldHeader_ = tableHeader_;

    // Get and assign array headers
    setHeader(tableGroupedHeader_);
    setJson(groupedJson);
}

// Apply held (ungrouped) values to table
void JsonTableModel::unGroupData()
{
    setHeader(tableHoldHeader_);
    setJson(tableHoldJsonData_);
}

void JsonTableModel::setColumnTitle(int section, QString title) { tableHeader_[section]["index"] = title; }

/*
 * QAbstractTableModel Overrides
 */

int JsonTableModel::rowCount(const QModelIndex &parent) const { return tableJsonData_ ? tableJsonData_->get().size() : 0; }

int JsonTableModel::columnCount(const QModelIndex &parent) const { return tableHeader_.size(); }

QVariant JsonTableModel::data(const QModelIndex &index, int role) const
{
    if (!tableJsonData_)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    QJsonObject obj = getJsonObject(index);
    const QString &key = tableHeader_[index.column()]["index"];
    if (!obj.contains(key))
        return {};
    QJsonValue v = obj[key];

    if (v.isDouble())
        return QString::number(v.toDouble());
    if (!v.isString())
        return {};

    // if title = Run Numbers then format (for grouped data)
    if (tableHeader_[index.column()]["title"] == "Run Numbers")
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
    if (tableHeader_.empty())
        return {};

    if (role == Qt::UserRole)
        return tableHeader_[section]["index"]; // Index == database name

    if (role != Qt::DisplayRole)
        return {};

    switch (orientation)
    {
        case Qt::Horizontal:
            return tableHeader_[section]["title"]; // Title == desired display name
        case Qt::Vertical:
            // return section + 1;
            return {};
        default:
            return {};
    }
}
