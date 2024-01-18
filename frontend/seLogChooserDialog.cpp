// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "seLogChooserDialog.h"

SELogChooserDialog::SELogChooserDialog(QWidget *parent, GenericTreeItem *rootItem) : QDialog(parent)
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
