// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QInputDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

/*
 * UI
 */

// Set current journal being displayed
void MainWindow::setCurrentJournal(QString name)
{
    if (cycleName[0] == '[')
    {
        auto it = std::find_if(cachedMassSearch_.begin(), cachedMassSearch_.end(),
                               [cycleName](const auto &tuple)
                               { return std::get<1>(tuple) == cycleName.mid(1, cycleName.length() - 2); });
        if (it != cachedMassSearch_.end())
        {
            ui_.cycleButton->setText(cycleName);
            setLoadScreen(true);
            handleCycleRunData(std::get<0>(*it));
        }
        return;
    }
    ui_.cycleButton->setText(cycleName);

    backend_.getJournal(currentInstrument().journalDirectory(), cyclesMap_[cycleName],
                        [=](HttpRequestWorker *worker) { handleCycleRunData(worker); });

    setLoadScreen(true);
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (cyclesMenu_->actions().count() == 0)
        QWidget::setEnabled(false);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentCycle = settings.value("recentCycle").toString();
    // Sets cycle to last used/ most recent if unavailable
    for (QAction *action : cyclesMenu_->actions())
    {
        if (action->text() == recentCycle)
        {
            action->trigger();
            return;
        }
    }
    cyclesMenu_->actions()[0]->trigger();
}
