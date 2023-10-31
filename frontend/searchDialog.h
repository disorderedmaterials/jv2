// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include "ui_searchDialog.h"

// Forward Declarations
class MainWindow;

class SearchDialog : public QDialog
{
    Q_OBJECT

    public:
    SearchDialog(QWidget *parent);
    ~SearchDialog() = default;

    /*
     * UI
     */
    private:
    Ui::SearchDialog ui_;

    private slots:
    void on_CancelButton_clicked(bool checked);
    void on_SearchButton_clicked(bool checked);

    public:
    // Get search query
    std::map<QString, QString> getQuery();
};
