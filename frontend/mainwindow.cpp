// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QDebug>
#include <QDomDocument>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QWidgetAction>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    initialiseElements();
}

MainWindow::~MainWindow() { delete ui_; }

// Configure initial application state
void MainWindow::initialiseElements()
{
    ui_->instrumentsBox->hide();
    auto instruments = getInstruments();
    fillInstruments(instruments);

    // First Iteration variable for set-up commands
    init_ = true;

    // View menu for column toggles
    viewMenu_ = ui_->menubar->addMenu("View");

    // Allows re-arranging of table columns
    ui_->runDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_->runDataTable->horizontalHeader()->setDragEnabled(true);
    ui_->runDataTable->setAlternatingRowColors(true);
    ui_->runDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");

    // Sets instrument to last used
    QSettings settings;
    QString recentInstrument = settings.value("recentInstrument").toString();
    int instrumentIndex = -1;
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
    ui_->tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_->runDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->runDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");

    connect(ui_->action_Quit, SIGNAL(triggered()), this, SLOT(close()));

    searchString_ = "";

    ui_->runDataTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (ui_->cyclesBox->count() == 0)
        QWidget::setEnabled(false);
    QSettings settings;
    QString recentCycle = settings.value("recentCycle").toString();
    auto cycleIndex = ui_->cyclesBox->findText(recentCycle);

    // Sets cycle to last used/ most recent if unavailable
    if (instName_ != "")
    {
        if (cycleIndex != -1)
            ui_->cyclesBox->setCurrentIndex(cycleIndex);
        else
            ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
    }
    else
        ui_->cyclesBox->setEnabled(false);
}

// Fill instrument list
void MainWindow::fillInstruments(QList<QPair<QString, QString>> instruments)
{
    // Only allow calls after initial population
    instrumentsMenu_ = new QMenu("test");
    connect(ui_->instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_->instrumentButton->mapToGlobal(QPoint(0, ui_->instrumentButton->height()))); });
    foreach (const auto instrument, instruments)
    {
        auto *action = new QAction(instrument.first, this);
        connect(action, &QAction::triggered, [=]() { changeInst(instrument); });
        instrumentsMenu_->addAction(action);
    }
}

void MainWindow::changeInst(QPair<QString, QString> instrument)
{
    ui_->instrumentButton->setText(instrument.first.toUpper());
    currentInstrumentChanged(instrument.first);
    instType_ = instrument.second;
    instName_ = instrument.first;
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings;
    settings.setValue("recentInstrument", instName_.toLower());
    settings.setValue("recentCycle", ui_->cyclesBox->currentText());

    // Close server
    QString url_str = "http://127.0.0.1:5000/shutdown";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);

    event->accept();
}

void MainWindow::massSearch(QString name, QString value)
{
    QString textInput =
        QInputDialog::getText(this, tr("Enter search query"), tr(name.append(": ").toUtf8()), QLineEdit::Normal);
    QString text = name.append(textInput);
    if (textInput.isEmpty())
        return;

    for (auto tuple : cachedMassSearch_)
    {
        if (std::get<1>(tuple) == text)
        {
            ui_->cyclesBox->setCurrentText("[" + std::get<1>(tuple) + "]");
            setLoadScreen(true);
            handle_result_cycles(std::get<0>(tuple));
            return;
        }
    }

    QString url_str = "http://127.0.0.1:5000/getAllJournals/" + instName_ + "/" + value + "/" + textInput;
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handle_result_cycles(HttpRequestWorker *)));
    worker->execute(input);
    cachedMassSearch_.append(std::make_tuple(worker, text));
    ui_->cyclesBox->addItem("[" + text + "]");
    ui_->cyclesBox->setCurrentText("[" + text + "]");
    setLoadScreen(true);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_G && event->modifiers() == Qt::ControlModifier)
    {
        bool checked = ui_->groupButton->isChecked();
        ui_->groupButton->setChecked(!checked);
        on_groupButton_clicked(!checked);
    }
    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier && Qt::ShiftModifier)
    {
        searchString_ = "";
        updateSearch(searchString_);
        return;
    }
    event->accept();
}

QList<QPair<QString, QString>> MainWindow::getInstruments()
{
    QFile file("../extra/instrumentData.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    QList<QPair<QString, QString>> instruments;
    QPair<QString, QString> instrument;
    QDomNode node;
    QDomElement elem;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        instrument.first = elem.attribute("name");
        instrument.second = elem.elementsByTagName("type").item(0).toElement().text();
        instruments.append(instrument);
    }
    return instruments;
}

std::vector<std::pair<QString, QString>> MainWindow::getFields(QString instrument, QString instType)
{
    std::vector<std::pair<QString, QString>> desiredInstFields;
    QDomNodeList desiredInstrumentFields;

    QFile file("../extra/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    std::pair<QString, QString> column;

    auto rootelem = dom.documentElement();
    auto instList = rootelem.elementsByTagName("inst");
    for (auto i = 0; i < instList.count(); i++)
    {
        if (instList.item(i).toElement().attribute("name").toLower() == instrument)
        {
            desiredInstrumentFields = instList.item(i).toElement().elementsByTagName("Column");
            break;
        }
    }
    if (desiredInstrumentFields.isEmpty())
    {
        auto configDefault = rootelem.elementsByTagName(instType).item(0).toElement();
        auto configDefaultFields = configDefault.elementsByTagName("Column");

        if (configDefaultFields.isEmpty())
        {
            QFile file("../extra/instrumentData.xml");
            file.open(QIODevice::ReadOnly);
            dom.setContent(&file);
            file.close();
            auto rootelem = dom.documentElement();
            auto defaultColumns = rootelem.elementsByTagName(instType).item(0).toElement().elementsByTagName("Column");
            for (int i = 0; i < defaultColumns.count(); i++)
            {
                // Get column index and title from xml
                column.first = defaultColumns.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
                column.second = defaultColumns.item(i).toElement().attribute("name");
                desiredInstFields.push_back(column);
            }
            return desiredInstFields;
        }
        for (int i = 0; i < configDefaultFields.count(); i++)
        {
            column.first = configDefaultFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
            column.second = configDefaultFields.item(i).toElement().attribute("name");
            desiredInstFields.push_back(column);
        }
        return desiredInstFields;
    }
    for (int i = 0; i < desiredInstrumentFields.count(); i++)
    {
        column.first = desiredInstrumentFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
        column.second = desiredInstrumentFields.item(i).toElement().attribute("name");
        desiredInstFields.push_back(column);
    }
    return desiredInstFields;
}

void MainWindow::savePref() // Add title support
{

    QFile file("../extra/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    QString currentFields;
    int realIndex;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        realIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_->runDataTable->isColumnHidden(realIndex))
        {
            currentFields += model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString(); 
            currentFields += ",;";
        }
    }
    currentFields.chop(1);

    QDomNode node;
    QDomElement elem;
    QDomElement columns;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        if (elem.attribute("name") == instName_)
        {
            auto oldColumns = elem.elementsByTagName("Columns");
            if (!oldColumns.isEmpty())
                elem.removeChild(elem.elementsByTagName("Columns").item(0));
            columns = dom.createElement("Columns");
            for (QString field : currentFields.split(";"))
            {
                auto preferredFieldsElem = dom.createElement("Column");
                auto preferredFieldsDataElem = dom.createElement("Data");
                preferredFieldsElem.setAttribute("Title", "placeholder");
                preferredFieldsDataElem.appendChild(dom.createTextNode(field.left(field.indexOf(","))));
                preferredFieldsElem.appendChild(preferredFieldsDataElem);
                columns.appendChild(preferredFieldsElem);
            }
            elem.appendChild(columns);
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QFile file("../extra/tableConfig.xml");
        file.open(QIODevice::WriteOnly);
        file.write(dom.toByteArray());
        file.close();
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
