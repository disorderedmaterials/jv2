#include "mainwindow.h"

// Hide column on view menu change
void MainWindow::columnHider(int state)
{
    QCheckBox *action = qobject_cast<QCheckBox *>(sender());

    for (int i = 0; i < ui->runDataTable->horizontalHeader()->count(); i++)
    {
        if (action->text() == ui->runDataTable->horizontalHeader()->model()->headerData(i, Qt::Horizontal))
        {
            switch (action->checkState())
            {
                case Qt::Unchecked:
                    ui->runDataTable->setColumnHidden(i, true);
                    break;
                case Qt::Checked:
                    ui->runDataTable->setColumnHidden(i, false);
                    break;
                default:
                    ui->runDataTable->setColumnHidden(i, false);
            }
            break;
        }
    }
}

// Filter table data
void MainWindow::on_filterBox_textChanged(const QString &arg1)
{
    proxyModel->setFilterFixedString(arg1.trimmed());
    proxyModel->setFilterKeyColumn(-1);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
}

// Groups table data
void MainWindow::on_groupButton_clicked(bool checked)
{
    if (checked)
    {
        model->groupData();
    }
    else
    {
        model->unGroupData();
    }
}

// Clears filter parameters
void MainWindow::on_clearSearchButton_clicked() { ui->filterBox->clear(); }