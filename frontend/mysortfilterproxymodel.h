#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "jsontablemodel.h"
#include <QObject>
#include <QModelIndex>

class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    
    public:
        MySortFilterProxyModel(QObject *parent = 0);

    public slots:
    void setFilterString(QString filterString);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    
    private:
        QString filterString_;
};

#endif // MYSORTFILTERPROXYMODEL_H