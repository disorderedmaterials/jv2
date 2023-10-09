// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

// Add new journal
Journal &MainWindow::addJournal(const QString &name, Journal::JournalLocation location, const QString &locationURL)
{
    auto &journal = journals_.emplace_back(name);
    journal.setFileLocation(location, locationURL);

    auto *action = new QAction(name, this);
    connect(action, &QAction::triggered, [=]() { setCurrentJournal(name); });
    cyclesMenu_->addAction(action);

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
            ui_.cycleButton->setText(name);
            setLoadScreen(true);
            handleCycleRunData(std::get<0>(*it));
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

    ui_.cycleButton->setText(journal.name());

    backend_.getJournal(currentInstrument().journalDirectory(), journal.locationURL(),
                        [=](HttpRequestWorker *worker) { handleCycleRunData(worker); });

    setLoadScreen(true);
}
