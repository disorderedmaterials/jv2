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

// Save current user settings
void MainWindow::saveSettings() const
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

    // Local source?
    auto localSource = settings.value("localSource").toString();
    if (!localSource.isEmpty())
    {
        auto *worker = new HttpRequestWorker(this);
        worker->execute({"http://127.0.0.1:5000/setLocalSource/" + localSource.replace("/", ";")});
    }

    // Mount point?
    auto mountPoint = settings.value("mountPoint").toString();
    if (!mountPoint.isEmpty())
    {
        auto *worker = new HttpRequestWorker(this);
        worker->execute({"http://127.0.0.1:5000/setRoot/" + mountPoint});
    }

    // Last used instrument?
    auto recentInstrument = settings.value("recentInstrument", instruments_.front().name()).toString();
    setCurrentInstrument(recentInstrument);
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

void MainWindow::on_actionSetLocalSource_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Set local source"), tr("source:"), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("localSource", textInput);

    QString msg = "If table fails to load, the local source cannot be found";
    QMessageBox::information(this, "", msg);

    QString url_str = "http://127.0.0.1:5000/setLocalSource/" + textInput.replace("/", ";");
    HttpRequestInput input(url_str);
    // TODO
    // auto *worker = new HttpRequestWorker(this);
    // connect(worker, &HttpRequestWorker::on_execution_finished, [=]() { refreshTable(); });
    // worker->execute(input);
}

void MainWindow::on_actionClearLocalSource_triggered()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("localSource", "");

    QString url_str = "http://127.0.0.1:5000/clearLocalSource";
    HttpRequestInput input(url_str);
    // TODO
    // auto *worker = new HttpRequestWorker(this);
    // connect(worker, &HttpRequestWorker::on_execution_finished, [=]() { refreshTable(); });
    // worker->execute(input);
}
