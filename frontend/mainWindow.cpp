// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include "ui_mainWindow.h"
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    ui_.setupUi(this);

    auto instruments = getInstruments();
    fillInstruments(instruments);

    // Define initial variable states
    init_ = true;
    searchString_ = "";

    // View menu for column toggles
    viewMenu_ = ui_.menubar->addMenu("View");

    // Allows re-arranging of table columns
    ui_.runDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_.runDataTable->horizontalHeader()->setDragEnabled(true);
    ui_.runDataTable->setAlternatingRowColors(true);
    ui_.runDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");
    ui_.runDataTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

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
    ui_.runDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.runDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
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

MainWindow::~MainWindow() {}

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

// Fill instrument list
void MainWindow::fillInstruments(QList<std::tuple<QString, QString, QString>> instruments)
{
    // Only allow calls after initial population
    instrumentsMenu_ = new QMenu("instrumentsMenu");
    cyclesMenu_ = new QMenu("cyclesMenu");

    connect(ui_.instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_.instrumentButton->mapToGlobal(QPoint(0, ui_.instrumentButton->height()))); });
    connect(ui_.cycleButton, &QPushButton::clicked,
            [=]() { cyclesMenu_->exec(ui_.cycleButton->mapToGlobal(QPoint(0, ui_.cycleButton->height()))); });
    foreach (const auto instrument, instruments)
    {
        auto *action = new QAction(std::get<2>(instrument), this);
        connect(action, &QAction::triggered, [=]() { changeInst(instrument); });
        instrumentsMenu_->addAction(action);
    }
}

// Handle Instrument selection
void MainWindow::changeInst(std::tuple<QString, QString, QString> instrument)
{
    instType_ = std::get<1>(instrument);
    instName_ = std::get<0>(instrument);
    instDisplayName_ = std::get<2>(instrument);
    ui_.instrumentButton->setText(instDisplayName_);
    currentInstrumentChanged(instName_);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("recentInstrument", instDisplayName_);
    settings.setValue("recentCycle", ui_.cycleButton->text());

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

void MainWindow::checkForUpdates()
{
    QString url_str = "http://127.0.0.1:5000/pingCycle/" + instName_;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { refresh(workerProxy->response); });
    worker->execute(input);
}

void MainWindow::refresh(QString status)
{
    if (status != "")
    {
        qDebug() << "Update";
        currentInstrumentChanged(instName_);
        if (cyclesMap_[cyclesMenu_->actions()[0]->text()] != status) // if new cycle found
        {
            auto displayName = "Cycle " + status.split("_")[1] + "/" + status.split("_")[2].remove(".xml");
            cyclesMap_[displayName] = status;

            auto *action = new QAction(displayName, this);
            connect(action, &QAction::triggered, [=]() { changeCycle(displayName); });
            cyclesMenu_->insertAction(cyclesMenu_->actions()[0], action);
        }
        else if (cyclesMap_[ui_.cycleButton->text()] == status) // if current opened cycle changed
        {
            QString url_str = "http://127.0.0.1:5000/updateJournal/" + instName_ + "/" + status + "/" +
                              model_->getJsonObject(model_->index(model_->rowCount() - 1, 0))["run_number"].toString();
            HttpRequestInput input(url_str);
            auto *worker = new HttpRequestWorker(this);
            connect(worker, &HttpRequestWorker::on_execution_finished,
                    [=](HttpRequestWorker *workerProxy) { update(workerProxy); });
            worker->execute(input);
        }
    }
    else
    {
        qDebug() << "no change";
        return;
    }
}

void MainWindow::update(HttpRequestWorker *worker)
{
    for (auto row : worker->jsonArray)
    {
        auto rowObject = row.toObject();
        model_->insertRows(model_->rowCount(), 1);
        auto index = model_->index(model_->rowCount() - 1, 0);
        model_->setData(index, rowObject);
    }
}

void MainWindow::refreshTable()
{
    for (auto i = 0; i < model_->columnCount(); ++i)
    {
        ui_.runDataTable->setColumnHidden(i, true);
    }
    currentInstrumentChanged(instName_);
}

// Hide column on view menu change
void MainWindow::columnHider(int state)
{
    auto *action = qobject_cast<QCheckBox *>(sender());

    for (auto i = 0; i < model_->columnCount(); ++i)
    {
        if (action->text() == headersMap_[model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString()])
        {
            switch (state)
            {
                case Qt::Unchecked:
                    ui_.runDataTable->setColumnHidden(i, true);
                    break;
                case Qt::Checked:
                    ui_.runDataTable->setColumnHidden(i, false);
                    break;
                default:
                    action->setCheckState(Qt::Checked);
            }
            break;
        }
    }
}
