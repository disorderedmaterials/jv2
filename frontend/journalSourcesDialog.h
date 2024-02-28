// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#pragma once

#include "journalSourceModel.h"
#include "lock.h"
#include "ui_journalSourcesDialog.h"

// Forward Declarations
class MainWindow;
class JournalSource;

class JournalSourcesDialog : public QDialog
{
    Q_OBJECT

    public:
    JournalSourcesDialog(QWidget *parent);
    ~JournalSourcesDialog() = default;

    /*
     * UI
     */
    private:
    Ui::JournalSourcesDialog ui_;
    // Currently-selected source
    JournalSource *currentSource_{nullptr};
    // Model for journal sources
    JournalSourceModel sourceModel_;
    // Widget update lock
    Lock widgetUpdateLock_;

    private slots:
    // Sources
    void currentSourceChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    void on_AddNewSourceButton_clicked(bool checked);
    void on_RemoveSourceButton_clicked(bool checked);
    // Source Type
    void on_GeneratedTypeRadioButton_clicked(bool checked);
    void on_NetworkTypeRadioButton_clicked(bool checked);
    // Journal Location
    void on_JournalRootURLEdit_editingFinished();
    void on_JournalIndexFileEdit_editingFinished();
    void on_JournalInstrumentPathCombo_currentIndexChanged(int index);
    void on_JournalInstrumentPathUppercaseCheck_clicked(bool checked);
    // Run Data Location
    void on_RunDataRootURLEdit_editingFinished();
    void on_RunDataRootURLSelectButton_clicked(bool checked);
    void on_RunDataInstrumentPathCombo_currentIndexChanged(int index);
    void on_RunDataInstrumentPathUppercaseCheck_clicked(bool checked);
    // Journal Data Organisation
    void on_DataOrganisationCombo_currentIndexChanged(int index);
    // Dialog
    void on_CloseButton_clicked(bool checked);

    public:
    // Go!
    void go(std::vector<std::unique_ptr<JournalSource>> &sources);
};
