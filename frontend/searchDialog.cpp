// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "searchDialog.h"

SearchDialog::SearchDialog(QWidget *parent) : QDialog(parent) { ui_.setupUi(this); }

void SearchDialog::on_CancelButton_clicked(bool checked) { reject(); }

void SearchDialog::on_SearchButton_clicked(bool checked) { accept(); }

// Get search query
std::map<QString, QString> SearchDialog::getQuery()
{
    if (exec() != QDialog::Accepted)
        return {};

    // Assemble the search query parameters
    std::map<QString, QString> parameters;
    if (ui_.RunTitleCheckBox->isChecked() && !ui_.RunTitleEdit->text().isEmpty())
        parameters["title"] = ui_.RunTitleEdit->text();
    if (ui_.RunNumberCheckBox->isChecked())
    {
        if (ui_.RunNumberRangeRadio->isChecked())
            parameters["run_number"] =
                QString("%1-%2").arg(ui_.RunNumberFromSpinBox->value()).arg(ui_.RunNumberToSpinBox->value());
        else if (ui_.RunNumberBeforeRadio->isChecked())
            parameters["run_number"] = QString("<%1").arg(ui_.RunNumberBeforeSpinBox->value());
        else if (ui_.RunNumberAfterRadio->isChecked())
            parameters["run_number"] = QString(">%1").arg(ui_.RunNumberAfterSpinBox->value());
    }

    return parameters;
}
