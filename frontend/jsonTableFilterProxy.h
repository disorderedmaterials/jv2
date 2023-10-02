// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

class JsonTableFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    JsonTableFilterProxy() = default;

    public slots:
    void setFilterString(QString filterString);
    void toggleCaseSensitivity(bool caseSensitive);
    QString filterString() const;

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
    QString filterString_;
    bool caseSensitive_{false};

    signals:
    void updateFilter();
};
