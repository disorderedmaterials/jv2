// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "optionalRef.h"
#include "ui_seLogChooserDialog.h"
#include <QAbstractItemModel>
#include <QDialog>
#include <map>
#include <vector>

class SELogTreeItem
{
    public:
    explicit SELogTreeItem(const QList<QVariant> &data, SELogTreeItem *parentItem = nullptr);
    ~SELogTreeItem();

    void appendChild(SELogTreeItem *child);

    SELogTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    SELogTreeItem *parentItem();

    private:
    QList<SELogTreeItem *> m_childItems;
    QList<QVariant> m_itemData;
    SELogTreeItem *m_parentItem;
};

class SELogTreeModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
    explicit SELogTreeModel(QObject *parent = nullptr);
    ~SELogTreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    private:
    // Root item for the model
    SELogTreeItem *rootItem_{nullptr};

    public:
    // Set root item for the model
    void setRootItem(SELogTreeItem *rootItem);
};

class SELogChooserDialog : public QDialog
{
    Q_OBJECT

    public:
    SELogChooserDialog(QWidget *parent = nullptr);
    ~SELogChooserDialog() = default;

    /*
     * UI
     */
    private:
    Ui::SELogChooserDialog ui_;
    SELogTreeModel treeModel_;

    public:
    // Set root data item for model
    void setRootItem(SELogTreeItem *rootItem);

    private slots:

    protected:
};
