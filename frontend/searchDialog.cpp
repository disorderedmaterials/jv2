// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "searchDialog.h"

SearchDialog::SearchDialog(QWidget *parent) : QDialog(parent)
{
    ui_.setupUi(this);
}

void SearchDialog::on_CancelButton_clicked(bool checked) { reject(); }

void SearchDialog::on_SearchButton_clicked(bool checked) { accept(); }

// Get search query
std::map<QString, QString> SearchDialog::getQuery()
{
    if (exec() != QDialog::Accepted)
        return {};

    // Assemble the search query parameters
    std::map<QString, QString> parameters;

    return parameters;
}
