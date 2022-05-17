#include "mysortfilterproxymodel.h"
#include "jsontablemodel.h"
#include <QSortFilterProxyModel>
#include <QObject>
#include <QModelIndex>

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    filterString_ = "";
}

void MySortFilterProxyModel::setFilterString(QString filterString)
{
    filterString_ = filterString;
}

bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow,
                                              const QModelIndex &sourceParent) const
{
    // for loop over columns.
    // If fails all - return false, else - return true
    auto accept = false;
    for (auto i = 0; i< sourceModel()->columnCount(); i++)
    {
        QModelIndex index = sourceModel()->index(sourceRow, i, sourceParent);
        if (sourceModel()->data(index).toString().contains(filterString_))
            accept = true;
    }

    return (accept);
}