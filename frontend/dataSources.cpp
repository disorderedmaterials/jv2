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
        auto sourceRootURL = sourceElement.attribute("rootURL");

        // Get source data directory (if type == Network)
        auto sourceDataDirectory = sourceType == DataSource::DataSourceType::Network ? sourceElement.attribute("rootURL") : "";

        auto &dataSource = dataSources_.emplace_back(sourceName, sourceType, sourceRootURL, sourceDataDirectory);
    }

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
void MainWindow::setCurrentDataSource(QString name)
{
    // Find the source specified
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