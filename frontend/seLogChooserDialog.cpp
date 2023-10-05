// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "seLogChooserDialog.h"

/*
 * SELogTreeItem
 */

SELogTreeItem::SELogTreeItem(const QList<QVariant> &data) : data_(data) {}

SELogTreeItem::~SELogTreeItem() { qDeleteAll(children_); }

void SELogTreeItem::appendChild(SELogTreeItem *item)
{
    children_.append(item);
    item->setParent(this);
}

SELogTreeItem *SELogTreeItem::appendChild(const QList<QVariant> &data)
{
    auto *item = new SELogTreeItem(data);
    item->setParent(this);
    children_.append(item);
    return item;
}

SELogTreeItem *SELogTreeItem::child(int row)
{
    if (row < 0 || row >= children_.size())
        return nullptr;
    return children_.at(row);
}

int SELogTreeItem::childCount() const { return children_.count(); }

int SELogTreeItem::row() const
{
    if (parent_)
        return parent_->children_.indexOf(const_cast<SELogTreeItem *>(this));

    return 0;
}

int SELogTreeItem::columnCount() const { return data_.count(); }

QVariant SELogTreeItem::data(int column) const
{
    if (column < 0 || column >= data_.count())
        return QVariant();
    return data_.at(column);
}

void SELogTreeItem::setParent(SELogTreeItem *parent) { parent_ = parent; }

SELogTreeItem *SELogTreeItem::parent() { return parent_; }

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
    SELogTreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem_)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SELogTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!rootItem_)
        return 0;

    auto *queryItem = parent.isValid() ? static_cast<SELogTreeItem *>(parent.internalPointer()) : rootItem_;

    return queryItem->childCount();
}

int SELogTreeModel::columnCount(const QModelIndex &parent) const
{
    if (!rootItem_)
        return 0;

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

SELogChooserDialog::SELogChooserDialog(QWidget *parent, SELogTreeItem *rootItem) : QDialog(parent)
{
    ui_.setupUi(this);

    treeModel_.setRootItem(rootItem);
    ui_.SELogTree->setModel(&treeModel_);
    ui_.SELogTree->expandAll();
    ui_.SELogTree->resizeColumnToContents(0);
    ui_.SELogTree->resizeColumnToContents(1);
    ui_.SELogTree->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui_.SELogTree->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this,
            SLOT(onTreeSelectionChanged(const QItemSelection &, const QItemSelection &)));

    ui_.SelectButton->setDisabled(true);
}

void SELogChooserDialog::onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &previous)
{
    ui_.SelectButton->setDisabled(selected.isEmpty());
}

void SELogChooserDialog::on_CancelButton_clicked(bool checked) { reject(); }

void SELogChooserDialog::on_SelectButton_clicked(bool checked) { accept(); }

// Perform selection
QString SELogChooserDialog::getValue()
{
    ui_.SELogTree->setSelectionMode(QAbstractItemView::SingleSelection);

    if (exec() == QDialog::Accepted)
    {
        auto selection = ui_.SELogTree->selectionModel()->selectedIndexes();

        for (const auto &index : selection)
            if (index.column() == 1)
                return treeModel_.data(index, Qt::DisplayRole).toString();
    }

    return {};
}

QStringList SELogChooserDialog::getValues()
{
    ui_.SELogTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringList result;

    if (exec() == QDialog::Accepted)
    {
        auto selection = ui_.SELogTree->selectionModel()->selectedIndexes();
        for (const auto &index : selection)
            if (index.column() == 1)
                result << treeModel_.data(index, Qt::DisplayRole).toString();
    }

    return result;
}
