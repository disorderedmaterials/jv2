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
    explicit SELogTreeItem(const QList<QVariant> &data);
    ~SELogTreeItem();

    private:
    QList<SELogTreeItem *> children_;
    QList<QVariant> data_;
    SELogTreeItem *parent_{nullptr};

    public:
    void appendChild(SELogTreeItem *child);
    SELogTreeItem *appendChild(const QList<QVariant> &data);
    SELogTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    void setParent(SELogTreeItem *parent);
    SELogTreeItem *parent();
};

class SELogTreeModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
    explicit SELogTreeModel(QObject *parent = nullptr);
    ~SELogTreeModel();

    private:
    // Root item for the model
    SELogTreeItem *rootItem_{nullptr};

    public:
    // Set root item for the model
    void setRootItem(SELogTreeItem *rootItem);
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
};

class SELogChooserDialog : public QDialog
{
    Q_OBJECT

    public:
    SELogChooserDialog(QWidget *parent, SELogTreeItem *rootItem);
    ~SELogChooserDialog() = default;

    /*
     * UI
     */
    private:
    Ui::SELogChooserDialog ui_;
    SELogTreeModel treeModel_;

    private slots:
    void onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_CancelButton_clicked(bool checked);
    void on_SelectButton_clicked(bool checked);

    public:
    // Perform selection
    QString getValue();
    QStringList getValues();
};
