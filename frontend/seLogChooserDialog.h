// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include "genericTreeModel.h"
#include "optionalRef.h"
#include "ui_seLogChooserDialog.h"

namespace JV2
{
class SELogChooserDialog : public QDialog
{
    Q_OBJECT

    public:
    SELogChooserDialog(QWidget *parent, GenericTreeItem *rootItem);
    ~SELogChooserDialog() = default;

    /*
     * UI
     */
    private:
    Ui::SELogChooserDialog ui_;
    GenericTreeModel treeModel_;

    private slots:
    void onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_CancelButton_clicked(bool checked);
    void on_SelectButton_clicked(bool checked);

    public:
    // Perform selection
    QString getValue();
    QStringList getValues();
};
} // namespace JV2
