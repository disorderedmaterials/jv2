// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>

/*
 * Private Functions
 */

// Perform mass search across cycles
void MainWindow::massSearch(QString name, QString value)
{
    if (!currentInstrument_)
        return;
    auto &inst = currentInstrument_->get();

    const char *prompt;
    QString textInput;
    QString text;
    bool caseSensitivity = false;
    name.append(": ");
    // configure for ranges
    if (name == "Run Range: ")
        prompt = "StartRun-EndRun:";
    else if (name == "Date Range: ")
        prompt = "yyyy/mm/dd-yyyy/mm/dd:";
    else
        prompt = name.toUtf8();

    if (name.contains("Range"))
    {
        QDialog dialog(this);
        QFormLayout form(&dialog);

        form.addRow(new QLabel(name));
        auto *start = new QLineEdit(&dialog);
        form.addRow("Start:", start);
        auto *end = new QLineEdit(&dialog);
        form.addRow("End:", end);

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        form.addRow(&buttonBox);
        QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

        if (dialog.exec() == QDialog::Accepted)
            textInput = start->text() + "-" + end->text();
        else
            return;
    }
    else
    {
        QDialog dialog(this);
        QFormLayout form(&dialog);

        form.addRow(new QLabel(name));
        auto *input = new QLineEdit(&dialog);
        form.addRow(name, input);
        auto *caseSensitive = new QCheckBox(&dialog);
        form.addRow("Case sensitive", caseSensitive);

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        form.addRow(&buttonBox);
        QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

        if (dialog.exec() == QDialog::Accepted)
        {
            textInput = input->text();
            caseSensitivity = caseSensitive->isChecked();
        }
        else
            return;
    }
    text = name.append(textInput);
    textInput.replace("/", ";");
    if (textInput.isEmpty())
        return;
    for (auto tuple : cachedMassSearch_)
    {
        if (std::get<1>(tuple) == text)
        {
            for (QAction *action : journalsMenu_->actions())
            {
                if (action->text() == "[" + std::get<1>(tuple) + "]")
                    action->trigger();
            }

            return;
        }
    }

    // mass search for data
    QString searchOptions;
    QString sensitivityText = "caseSensitivity=";
    sensitivityText.append(caseSensitivity ? "true" : "false");
    searchOptions.append(sensitivityText);

    backend_.findRuns(inst.journalDirectory(), value, textInput, searchOptions,
                      [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });

    // configure caching [FIXME]
    cachedMassSearch_.append(std::make_tuple(nullptr, text));

    auto *action = new QAction("[" + text + "]", this);
    // connect(action, &QAction::triggered, [=]() { setCurrentCycle("[" + text + "]"); });  [FIXME]!
    journalsMenu_->addAction(action);
    ui_.journalButton->setText("[" + text + "]");
}

/*
 * UI
 */

void MainWindow::on_actionMassSearchRBNo_triggered() { massSearch("RB No.", "experiment_identifier"); }

void MainWindow::on_actionMassSearchTitle_triggered() { massSearch("Title", "title"); }

void MainWindow::on_actionMassSearchUser_triggered() { massSearch("User name", "user_name"); }

void MainWindow::on_actionMassSearchRunRange_triggered() { massSearch("Run Range", "run_number"); }

void MainWindow::on_actionMassSearchDateRange_triggered() { massSearch("Date Range", "start_date"); }

void MainWindow::on_actionClearCachedSearches_triggered()
{
    cachedMassSearch_.clear();
    for (auto i = journalsMenu_->actions().count() - 1; i >= 0; i--)
    {
        if (journalsMenu_->actions()[i]->text()[0] == '[')
        {
            journalsMenu_->removeAction(journalsMenu_->actions()[i]);
        }
    }
    if (ui_.journalButton->text()[0] == '[')
        journalsMenu_->actions()[0]->trigger();
}
