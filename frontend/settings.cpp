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

void MainWindow::savePref()
{
    auto dom = getConfig();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");
    // Get current table fields
    QString currentFields;
    int realIndex;
    for (auto i = 0; i < ui_.RunDataTable->horizontalHeader()->count(); ++i)
    {
        realIndex = ui_.RunDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_.RunDataTable->isColumnHidden(realIndex))
        {
            currentFields += runDataModel_.headerData(realIndex, Qt::Horizontal, Qt::UserRole).toString();
            currentFields += ",";
            currentFields += runDataModel_.headerData(realIndex, Qt::Horizontal).toString();
            currentFields += ",;";
        }
    }
    currentFields.chop(1);

    // Add preferences to xml file
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
                preferredFieldsElem.setAttribute("name", field.split(",")[1]);
                preferredFieldsDataElem.appendChild(dom.createTextNode(field.split(",")[0]));
                preferredFieldsElem.appendChild(preferredFieldsDataElem);
                columns.appendChild(preferredFieldsElem);
            }
            elem.appendChild(columns);
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
        settings.setValue("tableConfig", dom.toByteArray());
    }
}

void MainWindow::clearPref()
{
    auto dom = getConfig();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    // Clear preferences from xml file
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
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
        settings.setValue("tableConfig", dom.toByteArray());
    }
}

// Get instrument data from config file
QList<std::tuple<QString, QString, QString>> MainWindow::getInstruments()
{
    QFile file(":/data/instrumentData.xml");
    if (!file.exists())
        return {};

    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");
    auto headersList = rootelem.elementsByTagName("header");
    headersMap_.clear();
    QString header;
    QString data;
    for (auto i = 0; i < headersList.count(); i++)
    {
        header = headersList.item(i).toElement().attribute("name");
        data = headersList.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
        headersMap_[data] = header;
    }

    QList<std::tuple<QString, QString, QString>> instruments;
    std::tuple<QString, QString, QString> instrument;
    QDomNode node;
    QDomElement elem;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        auto instrumentDisplayName = elem.elementsByTagName("displayName").item(0).toElement().text();
        auto instrumentType = elem.elementsByTagName("type").item(0).toElement().text();
        auto instrumentName = elem.attribute("name");
        instruments.append(std::make_tuple(instrumentName, instrumentType, instrumentDisplayName));
    }
    return instruments;
}

QDomDocument MainWindow::getConfig()
{
    QDomDocument dom;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    if (!settings.contains("tableConfig"))
    {
        QFile file(":/data/tableConfig.xml");
        file.open(QIODevice::ReadOnly);
        dom.setContent(&file);
        file.close();
    }
    else
        dom.setContent(settings.value("tableConfig", "fail").toString());
    return dom;
}

// Get the desired fields and their titles
std::vector<std::pair<QString, QString>> MainWindow::getFields(QString instrument, QString instType)
{
    std::vector<std::pair<QString, QString>> desiredInstFields;
    QDomNodeList desiredInstrumentFields;
    auto dom = getConfig();

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
    // If inst preferences blank
    if (desiredInstrumentFields.isEmpty())
    {
        auto configDefault = rootelem.elementsByTagName(instType).item(0).toElement();
        auto configDefaultFields = configDefault.elementsByTagName("Column");
        // If config preferences blank
        if (configDefaultFields.isEmpty())
        {
            QFile file(":/data/instrumentData.xml");
            file.open(QIODevice::ReadOnly);
            dom.setContent(&file);
            file.close();
            auto rootelem = dom.documentElement();
            auto defaultColumns = rootelem.elementsByTagName(instType).item(0).toElement().elementsByTagName("Column");
            // Get config preferences
            for (int i = 0; i < defaultColumns.count(); i++)
            {
                // Get column index and title from xml
                column.first = defaultColumns.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
                column.second = defaultColumns.item(i).toElement().attribute("name");
                desiredInstFields.push_back(column);
            }
            return desiredInstFields;
        }
        // Get config default
        for (int i = 0; i < configDefaultFields.count(); i++)
        {
            column.first = configDefaultFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
            column.second = configDefaultFields.item(i).toElement().attribute("name");
            desiredInstFields.push_back(column);
        }
        return desiredInstFields;
    }
    // Get instrument preferences
    for (int i = 0; i < desiredInstrumentFields.count(); i++)
    {
        column.first = desiredInstrumentFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
        column.second = desiredInstrumentFields.item(i).toElement().attribute("name");
        desiredInstFields.push_back(column);
    }
    return desiredInstFields;
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
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished, [=]() { refreshTable(); });
    worker->execute(input);
}

void MainWindow::on_actionClearLocalSource_triggered()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("localSource", "");

    QString url_str = "http://127.0.0.1:5000/clearLocalSource";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished, [=]() { refreshTable(); });
    worker->execute(input);
}
