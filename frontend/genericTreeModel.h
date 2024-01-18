// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include <QAbstractItemModel>
#include <QDialog>

class GenericTreeItem
{
    public:
    explicit GenericTreeItem(const QList<QVariant> &data);
    ~GenericTreeItem();

    private:
    QList<GenericTreeItem *> children_;
    QList<QVariant> data_;
    GenericTreeItem *parent_{nullptr};

    public:
    void appendChild(GenericTreeItem *child);
    GenericTreeItem *appendChild(const QList<QVariant> &data);
    GenericTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    void setParent(GenericTreeItem *parent);
    GenericTreeItem *parent();
};

class GenericTreeModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
    explicit GenericTreeModel(QObject *parent = nullptr);
    ~GenericTreeModel();

    private:
    // Root item for the model
    GenericTreeItem *rootItem_{nullptr};

    public:
    // Set root item for the model
    void setRootItem(GenericTreeItem *rootItem);
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
};
