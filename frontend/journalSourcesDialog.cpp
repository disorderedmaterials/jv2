// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalViewer and contributors

#include "journalSourcesDialog.h"
#include <QFileDialog>
#include <QMessageBox>

JournalSourcesDialog::JournalSourcesDialog(QWidget *parent) : QDialog(parent)
{
    ui_.setupUi(this);
    ui_.SourcesListView->setModel(&sourceModel_);
    connect(ui_.SourcesListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
            SLOT(currentSourceChanged(const QModelIndex &, const QModelIndex &)));
}

/*
 * Sources
 */

void JournalSourcesDialog::currentSourceChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
    // Get the selected source
    currentSource_ = sourceModel_.getData(currentIndex);

    Locker updateLock(widgetUpdateLock_);

    // Overall group control
    ui_.SourceTypGroup->setEnabled(currentSource_);
    ui_.JournalLocationGroup->setEnabled(currentSource_ && currentSource_->type() == JournalSource::IndexingType::Network);
    ui_.RunDataLocationGroup->setEnabled(currentSource_);
    ui_.DataOrganisationGroup->setEnabled(currentSource_ && currentSource_->type() == JournalSource::IndexingType::Generated);
    ui_.RemoveSourceButton->setEnabled(currentSource_ && currentSource_->isUserDefined());

    if (!currentSource_)
        return;

    // Update individual controls
    // -- Type
    if (currentSource_->type() == JournalSource::IndexingType::Network)
        ui_.NetworkTypeRadioButton->setEnabled(true);
    else
        ui_.GeneratedTypeRadioButton->setEnabled(true);
    // -- Journal Location
    ui_.JournalRootURLEdit->setText(currentSource_->journalRootUrl());
    ui_.JournalIndexFileEdit->setText(currentSource_->journalIndexFilename());
    ui_.JournalInstrumentPathCombo->setCurrentIndex(currentSource_->journalOrganisationByInstrument());
    ui_.JournalInstrumentPathUppercaseCheck->setChecked(currentSource_->isJournalOrganisationByInstrumentUpperCased());
    // -- Run Data Location
    ui_.RunDataRootURLEdit->setText(currentSource_->runDataRootUrl());
    ui_.RunDataRootRegExpEdit->setText(currentSource_->runDataRootRegExp());
    ui_.RunDataInstrumentPathCombo->setCurrentIndex(currentSource_->runDataOrganisationByInstrument());
    ui_.RunDataInstrumentPathUppercaseCheck->setChecked(currentSource_->isRunDataOrganisationByInstrumentUpperCased());
    // -- Data Organisation
    ui_.DataOrganisationCombo->setCurrentIndex(currentSource_->dataOrganisation());
}

void JournalSourcesDialog::on_AddNewSourceButton_clicked(bool checked)
{
    auto index = sourceModel_.appendNew();
    ui_.SourcesListView->setCurrentIndex(index);
}

void JournalSourcesDialog::on_RemoveSourceButton_clicked(bool checked)
{
    // Get the selected source
    if (!currentSource_)
        return;

    if (QMessageBox::question(
            this, "Remove Source",
            QString("Are you sure you want to remove the source '%1'?\nThis cannot be undone!").arg(currentSource_->name())) ==
        QMessageBox::StandardButton::Yes)
    {
        sourceModel_.remove(ui_.SourcesListView->currentIndex());
    }
}

/*
 * Source Type
 */

void JournalSourcesDialog::on_GeneratedTypeRadioButton_clicked(bool checked)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    if (checked)
        currentSource_->setType(JournalSource::IndexingType::Generated);
}

void JournalSourcesDialog::on_NetworkTypeRadioButton_clicked(bool checked)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    if (checked)
        currentSource_->setType(JournalSource::IndexingType::Network);
}

/*
 * Journal Location
 */

void JournalSourcesDialog::on_JournalRootURLEdit_editingFinished()
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setJournalLocation(ui_.JournalRootURLEdit->text(), ui_.JournalIndexFileEdit->text());
}

void JournalSourcesDialog::on_JournalIndexFileEdit_editingFinished()
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setJournalLocation(ui_.JournalRootURLEdit->text(), ui_.JournalIndexFileEdit->text());
}

void JournalSourcesDialog::on_JournalInstrumentPathCombo_currentIndexChanged(int index)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setJournalOrganisationByInstrument((Instrument::PathType)ui_.JournalInstrumentPathCombo->currentIndex(),
                                                       ui_.JournalInstrumentPathUppercaseCheck->isChecked());
}

void JournalSourcesDialog::on_JournalInstrumentPathUppercaseCheck_clicked(bool checked)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setJournalOrganisationByInstrument((Instrument::PathType)ui_.JournalInstrumentPathCombo->currentIndex(),
                                                       ui_.JournalInstrumentPathUppercaseCheck->isChecked());
}

/*
 * Run Data Location
 */

void JournalSourcesDialog::on_RunDataRootURLEdit_editingFinished()
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setRunDataLocation(ui_.RunDataRootURLEdit->text());
}

void JournalSourcesDialog::on_RunDataRootRegExpEdit_editingFinished()
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setRunDataRootRegExp(ui_.RunDataRootRegExpEdit->text());
}

void JournalSourcesDialog::on_RunDataRootURLSelectButton_clicked(bool checked)
{
    auto dir = QFileDialog::getExistingDirectory(this, "Choose Run Data Location", ui_.RunDataRootURLEdit->text());
    if (dir.isEmpty())
        return;

    ui_.RunDataRootURLEdit->setText(dir);
    currentSource_->setRunDataLocation(dir);
}

void JournalSourcesDialog::on_RunDataInstrumentPathCombo_currentIndexChanged(int index)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setRunDataOrganisationByInstrument((Instrument::PathType)ui_.RunDataInstrumentPathCombo->currentIndex(),
                                                       ui_.RunDataInstrumentPathUppercaseCheck->isChecked());
}

void JournalSourcesDialog::on_RunDataInstrumentPathUppercaseCheck_clicked(bool checked)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setRunDataOrganisationByInstrument((Instrument::PathType)ui_.RunDataInstrumentPathCombo->currentIndex(),
                                                       ui_.RunDataInstrumentPathUppercaseCheck->isChecked());
}

/*
 * Journal Data Organisation
 */

void JournalSourcesDialog::on_DataOrganisationCombo_currentIndexChanged(int index)
{
    if (widgetUpdateLock_ || !currentSource_)
        return;

    currentSource_->setDataOrganisation((JournalSource::DataOrganisationType)ui_.DataOrganisationCombo->currentIndex());
}

/*
 * Dialog
 */

void JournalSourcesDialog::on_CloseButton_clicked(bool checked) { accept(); }

// Go
void JournalSourcesDialog::go(std::vector<std::unique_ptr<JournalSource>> &sources)
{
    sourceModel_.setData(sources, true);

    exec();
}
