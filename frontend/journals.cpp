// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

// Clear current journals
void MainWindow::clearJournals()
{
    clearRunData();
    journals_.clear();
    journalsMenu_->clear();
    currentJournal_ = std::nullopt;
}

// Add new journal
Journal &MainWindow::addJournal(const QString &name, const Locator &location)
{
    auto &journal = journals_.emplace_back(name);
    journal.setLocation(location);

    auto *action = new QAction(name, this);
    connect(action, &QAction::triggered, [=]() { setCurrentJournal(name); });
    journalsMenu_->addAction(action);

    return journal;
}

// Find named journal
OptionalReferenceWrapper<Journal> MainWindow::findJournal(const QString &name)
{
    auto journalIt =
        std::find_if(journals_.begin(), journals_.end(), [name](const auto &journal) { return journal.name() == name; });

    if (journalIt == journals_.end())
        return std::nullopt;

    return *journalIt;
}

// Set current journal being displayed
void MainWindow::setCurrentJournal(QString name)
{
    if (name[0] == '[')
    {
        auto it = std::find_if(cachedMassSearch_.begin(), cachedMassSearch_.end(),
                               [name](const auto &tuple) { return std::get<1>(tuple) == name.mid(1, name.length() - 2); });
        if (it != cachedMassSearch_.end())
        {
            ui_.journalButton->setText(name);

            handleCompleteJournalRunData(std::get<0>(*it));
        }
        return;
    }

    // Find the journal specified
    auto optJournal = findJournal(name);
    if (!optJournal)
        throw(std::runtime_error("Selected journal does not exist!\n"));

    setCurrentJournal(*optJournal);
}

void MainWindow::setCurrentJournal(Journal &journal)
{
    currentJournal_ = journal;

    ui_.journalButton->setText(journal.name());

    updateForCurrentSource(JournalSource::JournalSourceState::Loading);

    backend_.getJournal(journal.location(), [=](HttpRequestWorker *worker) { handleCompleteJournalRunData(worker); });
}

// Return current journal
const Journal &MainWindow::currentJournal() const
{
    if (currentJournal_)
        return currentJournal_->get();

    throw(std::runtime_error("No current journal defined.\n"));
}

/*
 * Network Handling
 */

// Handle returned journal information for an instrument
void MainWindow::handleListJournals(HttpRequestWorker *worker)
{
    clearJournals();

    // Check network reply
    if (networkRequestHasError(worker, "trying to list journals"))
    {
        updateForCurrentSource(JournalSource::JournalSourceState::NetworkError);
        return;
    }

    // Special case - for disk-based sources we may get an error stating that the index file was not found.
    // This may just be because it hasn't been generated yet, so we can offer to do it now...
    if (worker->response.startsWith("\"Index File Not Found\""))
    {
        auto &journalSource = currentJournalSource();

        updateForCurrentSource(JournalSource::JournalSourceState::NoIndexFileError);

        if (QMessageBox::question(this, "Index File Doesn't Exist",
                                  QString("No index file %1/%2 currently exists.\nWould you like to generate it now?")
                                      .arg(currentJournalSource().rootUrl(), currentJournalSource().indexFile())) ==
            QMessageBox::StandardButton::Yes)
        {
            bool orgByInst = currentJournalSource_->get().instrumentSubdirectories();

            backend_.listDataDirectory(currentJournalSource(), orgByInst ? currentInstrument().journalDirectory() : "",
                                       [=](HttpRequestWorker *worker) { handleListDataDirectory(journalSource, worker); });

            return;
        }
    }

    // Add returned journals
    for (auto i = worker->jsonArray.count() - 1; i >= 0; i--)
    {
        auto value = worker->jsonArray[i].toObject();

        addJournal(value["display_name"].toString(),
                   {value["server_root"].toString(), value["directory"].toString(), value["filename"].toString()});
    }

    // If there is no current journal, set one
    if (!currentJournal_ && !journals_.empty())
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
        auto optJournal = findJournal(settings.value("recentJournal").toString());
        if (optJournal)
            setCurrentJournal(*optJournal);
        else
            setCurrentJournal(journals_.front());
    }
    else
        updateForCurrentSource(JournalSource::JournalSourceState::OK);
}

// Handle get journal updates result
void MainWindow::handleGetJournalUpdates(HttpRequestWorker *worker)
{
    // A null response indicates no change
    if (worker->response.startsWith("null"))
        return;

    // The main body of the request contains any run numbers we don't currently have.
    // If we are currently displaying grouped data we append the new data directly then refresh the grouping
    if (ui_.GroupRunsButton->isChecked())
    {
        foreach (const auto &item, worker->jsonArray)
            runData_.append(item);

        generateGroupedData();

        runDataModel_.setData(groupedRunData_);
        runDataModel_.setHorizontalHeaders(groupedRunDataColumns_);

        ui_.RunDataTable->resizeColumnsToContents();
    }
    else
    {
        // Update via the model
        runDataModel_.appendData(worker->jsonArray);
    }
}
