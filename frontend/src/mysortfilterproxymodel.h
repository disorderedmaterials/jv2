// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include "jsontablemodel.h"
#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    MySortFilterProxyModel(QObject *parent = 0);

    public slots:
    void setFilterString(QString filterString);
    void toggleCaseSensitivity(bool caseSensitive);
    QString filterString() const;

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
    QString filterString_;
    bool caseSensitive_;

    signals:
    void updateFilter();
};

#endif // MYSORTFILTERPROXYMODEL_H
