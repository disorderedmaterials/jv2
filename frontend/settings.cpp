// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>

/*
 * Private Functions
 */

// Save custom column settings
void MainWindow::saveCustomColumnSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    // Save customised column views
    QDomDocument customColumns;
    for (const auto &inst : instruments_)
    {
        if (!inst.hasCustomColumns())
            continue;
    }
}

// Retrieve user settings
void MainWindow::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");

    // Mount point?
    auto mountPoint = settings.value("mountPoint").toString();
    if (!mountPoint.isEmpty())
    {
        auto *worker = new HttpRequestWorker(this);
        worker->execute({"http://127.0.0.1:5000/setRoot/" + mountPoint});
    }
}

/*
 * UI
 */

void MainWindow::on_actionMountPoint_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Set Mount Point"), tr("Location:"), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("mountPoint", textInput);

    QString url_str = "http://127.0.0.1:5000/setRoot/";
    url_str += textInput;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);
}

void MainWindow::on_actionClearMountPoint_triggered()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("mountPoint", "");

    QString url_str = "http://127.0.0.1:5000/setRoot/Default";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);
}
