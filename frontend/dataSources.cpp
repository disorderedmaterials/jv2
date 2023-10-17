// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "mainWindow.h"
#include <QDomDocument>
#include <QFile>

/*
 * Private Functions
 */

// Parse data sources from specified source
bool MainWindow::parseDataSources(const QDomDocument &source)
{
    // Clear old sources
    dataSources_.clear();
    ui_.DataSourceComboBox->clear();

    auto docRoot = source.documentElement();
    auto sourceNodes = docRoot.elementsByTagName("source");

    // Loop over sources
    for (auto i = 0; i < sourceNodes.count(); ++i)
    {
        auto sourceElement = sourceNodes.item(i).toElement();

        // Get source name
        auto sourceName = sourceElement.attribute("name");

        // Get source type
        auto sourceType = DataSource::dataSourceType(sourceElement.attribute("type", "Disk"));

        // Get source root URL
        auto sourceRootURL = sourceElement.attribute("rootUrl");

        // Optional attributes
        auto sourceDataDirectory = sourceElement.attribute("dataDirectory");
        auto sourceIndexFile = sourceElement.attribute("indexFile");

        // Create the source
        auto &dataSource =
            dataSources_.emplace_back(sourceName, sourceType, sourceRootURL, sourceDataDirectory, sourceIndexFile);
    }

    // Populate the combo box with options
    for (const auto &source : dataSources_)
        ui_.DataSourceComboBox->addItem(source.name());

    return true;
}

// Get default data sources complement
void MainWindow::getDefaultDataSources()
{
    QFile file(":/data/sources.xml");
    if (!file.exists())
        throw(std::runtime_error("Internal data sources not found.\n"));

    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    if (!parseDataSources(dom))
        throw(std::runtime_error("Couldn't parse internal data sources.\n"));
}

/*
 * UI
 */

// Set current data source
void MainWindow::setCurrentDataSource(std::optional<QString> optName)
{
    // If no source is specified, clear everything
    if (!optName)
    {
        clearRunData();
        clearJournals();
        journals_.clear();
        return;
    }

    // Find the source specified
    auto name = *optName;
    auto sourceIt =
        std::find_if(dataSources_.begin(), dataSources_.end(), [name](const auto &source) { return source.name() == name; });
    if (sourceIt == dataSources_.end())
        throw(std::runtime_error("Selected data source does not exist!\n"));

    currentDataSource_ = *sourceIt;

    // ui_.sourceButton->setText(name);

    // Clear any mass search results since they're source-specific
    cachedMassSearch_.clear();

    // backend_.listJournals(currentDataSource().journalDirectory(),
    //                       [=](HttpRequestWorker *worker) { handleListJournals(worker); });

    setLoadScreen(true);
}

// Return current data source
const DataSource &MainWindow::currentDataSource() const
{
    if (currentDataSource_)
        return currentDataSource_->get();

    throw(std::runtime_error("No current data source defined.\n"));
}

/*
 * Private SLots
 */

void MainWindow::on_DataSourceComboBox_currentIndexChanged(int index)
{
    if (index == -1)
        setCurrentDataSource({});
    else
        setCurrentDataSource(ui_.DataSourceComboBox->currentText());
}