// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "ui_mainWindow.h"
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    ui_.setupUi(this);

    // Get available instruments
    getDefaultInstruments();
    fillInstruments();

    // Define initial variable states
    init_ = true;
    searchString_ = "";
    groupedTableHeaders_.push_back(JsonTableModel::Heading({{"title", "Title"}, {"index", "title"}}));
    groupedTableHeaders_.push_back(JsonTableModel::Heading({{"title", "Total Duration"}, {"index", "duration"}}));
    groupedTableHeaders_.push_back(JsonTableModel::Heading({{"title", "Run Numbers"}, {"index", "run_number"}}));

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

    // Sets instrument to last used
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentInstrument = settings.value("recentInstrument").toString();
    bool found = false;
    for (auto i = 0; i < instrumentsMenu_->actions().count(); i++)
        if (instrumentsMenu_->actions()[i]->text() == recentInstrument)
        {
            instrumentsMenu_->actions()[i]->trigger();
            found = true;
        }
    if (!found)
        instrumentsMenu_->actions()[instrumentsMenu_->actions().count() - 1]->trigger();

    // Disables closing data tab + handles tab closing
    ui_.MainTabs->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_.MainTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_.RunDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.RunDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");

    // Connect exit action
    connect(ui_.action_Quit, SIGNAL(triggered()), this, SLOT(close()));

    // Tests and assigns local sources from memory
    QString localSource = settings.value("localSource").toString();
    QString url_str;
    validSource_ = true;
    if (!localSource.isEmpty())
        url_str = "http://127.0.0.1:5000/clearLocalSource";
    else
        url_str = "http://127.0.0.1:5000/setLocalSource/" + localSource.replace("/", ";");
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);

    QString mountPoint = settings.value("mountPoint").toString();
    if (!mountPoint.isEmpty())
    {
        url_str = "http://127.0.0.1:5000/setRoot/" + mountPoint;
        HttpRequestInput input2(url_str);
        auto *worker2 = new HttpRequestWorker(this);
        worker2->execute(input2);
    }

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
        auto &inst = currentInstrument_->get();
        settings.setValue("recentInstrument", inst.name());
        settings.setValue("recentCycle", ui_.cycleButton->text());
    }

    // Close server
    QString url_str = "http://127.0.0.1:5000/shutdown";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);
    if (!validSource_)
    {
        url_str = "http://127.0.0.1:5000/clearLocalSource";
        HttpRequestInput input2(url_str);
        auto *worker2 = new HttpRequestWorker(this);
        worker2->execute(input2);
    }
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
