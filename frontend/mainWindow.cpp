// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "ui_mainWindow.h"
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    ui_.setupUi(this);

    // Get default instrument run data columns
    Instrument::getDefaultColumns();

    // Get available instrument data
    getDefaultInstruments();
    fillInstruments();

    // Define initial variable states
    init_ = true;
    searchString_ = "";
    groupedRunDataColumns_.emplace_back("Run Numbers", "run_number");
    groupedRunDataColumns_.emplace_back("Title", "title");
    groupedRunDataColumns_.emplace_back("Total Duration", "duration");

    // View menu for column toggles
    viewMenu_ = ui_.menubar->addMenu("View");

    // Set up the main data table
    runDataFilterProxy_.setSourceModel(&runDataModel_);
    ui_.RunDataTable->setModel(&runDataFilterProxy_);
    // -- Allow re-arranging of table columns
    ui_.RunDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_.RunDataTable->horizontalHeader()->setDragEnabled(true);
    ui_.RunDataTable->setAlternatingRowColors(true);
    ui_.RunDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");
    ui_.RunDataTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    // Disables closing data tab + handles tab closing
    ui_.MainTabs->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_.MainTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_.RunDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.RunDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");

    // Connect exit action
    connect(ui_.action_Quit, SIGNAL(triggered()), this, SLOT(close()));

    // Get user settings
    loadSettings();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=]() { checkForUpdates(); });
    timer->start(30000);
}

/*
 * UI
 */

void MainWindow::setLoadScreen(bool state)
{
    if (state)
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QWidget::setEnabled(false);
    }
    else
    {
        QWidget::setEnabled(true);
        QGuiApplication::restoreOverrideCursor();
    }
}

void MainWindow::removeTab(int index) { delete ui_.MainTabs->widget(index); }

// Hide column on view menu change
void MainWindow::columnHider(int state)
{
    auto *action = qobject_cast<QCheckBox *>(sender());

    for (auto i = 0; i < runDataModel_.columnCount(); ++i)
    {
        if (action->text() == headersMap_[runDataModel_.headerData(i, Qt::Horizontal, Qt::UserRole).toString()])
        {
            switch (state)
            {
                case Qt::Unchecked:
                    ui_.RunDataTable->setColumnHidden(i, true);
                    break;
                case Qt::Checked:
                    ui_.RunDataTable->setColumnHidden(i, false);
                    break;
                default:
                    action->setCheckState(Qt::Checked);
            }
            break;
        }
    }
}

/*
 * Window
 */

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    if (currentInstrument_)
    {
        settings.setValue("recentInstrument", currentInstrument().name());
        settings.setValue("recentCycle", ui_.cycleButton->text());
    }

    // Shut down backend
    auto *worker = new HttpRequestWorker(this);
    worker->execute({"http://127.0.0.1:5000/shutdown"});

    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_G && event->modifiers() == Qt::ControlModifier)
    {
        bool checked = ui_.GroupRunsButton->isChecked();
        ui_.GroupRunsButton->setChecked(!checked);
        on_GroupRunsButton_clicked(!checked);
    }
    if (event->key() == Qt::Key_R && event->modifiers() == Qt::ControlModifier)
        checkForUpdates();
    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier && Qt::ShiftModifier)
    {
        searchString_ = "";
        updateSearch(searchString_);
        return;
    }
}
