// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "seLogChooserDialog.h"

/*
 * SELogTreeItem
 */

SELogTreeItem::SELogTreeItem(const QList<QVariant> &data, SELogTreeItem *parent) : m_itemData(data), m_parentItem(parent) {}

SELogTreeItem::~SELogTreeItem() { qDeleteAll(m_childItems); }
void SELogTreeItem::appendChild(SELogTreeItem *item) { m_childItems.append(item); }

SELogTreeItem *SELogTreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int SELogTreeItem::childCount() const { return m_childItems.count(); }
int SELogTreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<SELogTreeItem *>(this));

    return 0;
}

int SELogTreeItem::columnCount() const { return m_itemData.count(); }

QVariant SELogTreeItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
}
SELogTreeItem *SELogTreeItem::parentItem() { return m_parentItem; }

/*
 * Model
 */

SELogTreeModel::SELogTreeModel(QObject *parent) : QAbstractItemModel(parent) {}

SELogTreeModel::~SELogTreeModel()
{
    if (rootItem_)
        delete rootItem_;
}

QModelIndex SELogTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!rootItem_ || !hasIndex(row, column, parent))
        return QModelIndex();

    SELogTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<SELogTreeItem *>(parent.internalPointer());

    SELogTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex SELogTreeModel::parent(const QModelIndex &index) const
{
    if (!rootItem_ || !index.isValid())
        return QModelIndex();

    SELogTreeItem *childItem = static_cast<SELogTreeItem *>(index.internalPointer());
    SELogTreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem_)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SELogTreeModel::rowCount(const QModelIndex &parent) const
{
    SELogTreeItem *parentItem;
    if (!rootItem_ || parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<SELogTreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

int SELogTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SELogTreeItem *>(parent.internalPointer())->columnCount();
    return rootItem_->columnCount();
}

QVariant SELogTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    SELogTreeItem *item = static_cast<SELogTreeItem *>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags SELogTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant SELogTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!rootItem_)
        return {};

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem_->data(section);

    return {};
}

// Set root item for the model
void SELogTreeModel::setRootItem(SELogTreeItem *rootItem)
{
    beginResetModel();
    rootItem_ = rootItem;
    endResetModel();
}

/*
 * Dialog
 */

SELogChooserDialog::SELogChooserDialog(QWidget *parent) : QDialog(parent)
{
    ui_.setupUi(this);

    ui_.SELogTree->setModel(&treeModel_);

    show();
}

// Set root data item for model
void SELogChooserDialog::setRootItem(SELogTreeItem *rootItem) { treeModel_.setRootItem(rootItem); }