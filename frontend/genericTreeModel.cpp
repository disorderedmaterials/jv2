// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "genericTreeModel.h"

/*
 * GenericTreeItem
 */

GenericTreeItem::GenericTreeItem(const QList<QVariant> &data) : data_(data) {}

GenericTreeItem::~GenericTreeItem() { qDeleteAll(children_); }

void GenericTreeItem::appendChild(GenericTreeItem *item)
{
    children_.append(item);
    item->setParent(this);
}

GenericTreeItem *GenericTreeItem::appendChild(const QList<QVariant> &data)
{
    auto *item = new GenericTreeItem(data);
    item->setParent(this);
    children_.append(item);
    return item;
}

GenericTreeItem *GenericTreeItem::child(int row)
{
    if (row < 0 || row >= children_.size())
        return nullptr;
    return children_.at(row);
}

int GenericTreeItem::childCount() const { return children_.count(); }

int GenericTreeItem::row() const
{
    if (parent_)
        return parent_->children_.indexOf(const_cast<GenericTreeItem *>(this));

    return 0;
}

int GenericTreeItem::columnCount() const { return data_.count(); }

QVariant GenericTreeItem::data(int column) const
{
    if (column < 0 || column >= data_.count())
        return QVariant();
    return data_.at(column);
}

void GenericTreeItem::setParent(GenericTreeItem *parent) { parent_ = parent; }

GenericTreeItem *GenericTreeItem::parent() { return parent_; }

/*
 * Model
 */

GenericTreeModel::GenericTreeModel(QObject *parent) : QAbstractItemModel(parent) {}

GenericTreeModel::~GenericTreeModel()
{
    if (rootItem_)
        delete rootItem_;
}

QModelIndex GenericTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!rootItem_ || !hasIndex(row, column, parent))
        return QModelIndex();

    GenericTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<GenericTreeItem *>(parent.internalPointer());

    GenericTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex GenericTreeModel::parent(const QModelIndex &index) const
{
    if (!rootItem_ || !index.isValid())
        return QModelIndex();

    GenericTreeItem *childItem = static_cast<GenericTreeItem *>(index.internalPointer());
    GenericTreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem_)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int GenericTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!rootItem_)
        return 0;

    auto *queryItem = parent.isValid() ? static_cast<GenericTreeItem *>(parent.internalPointer()) : rootItem_;

    return queryItem->childCount();
}

int GenericTreeModel::columnCount(const QModelIndex &parent) const
{
    if (!rootItem_)
        return 0;

    if (parent.isValid())
        return static_cast<GenericTreeItem *>(parent.internalPointer())->columnCount();

    return rootItem_->columnCount();
}

QVariant GenericTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    GenericTreeItem *item = static_cast<GenericTreeItem *>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags GenericTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant GenericTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!rootItem_)
        return {};

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem_->data(section);

    return {};
}

// Set root item for the model
void GenericTreeModel::setRootItem(GenericTreeItem *rootItem)
{
    beginResetModel();
    rootItem_ = rootItem;
    endResetModel();
}
