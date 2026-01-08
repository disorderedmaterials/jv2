// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 Team JournalViewer and contributors

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

namespace JV2
{
// Forward Declarations
class LogValueModel;

class LogValueFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    LogValueFilterProxy(LogValueModel &journalSourceModel);
    // Selected Sate Behaviour
    enum class SelectedStateBehaviour
    {
        Ignore,
        HideSelected,
        ShowOnlySelected
    };

    private:
    // Target model
    LogValueModel &logValueModel_;
    // Search string
    QString filterString_;
    // Behaviour for selected log values in the model
    SelectedStateBehaviour selectedValueBehaviour_{SelectedStateBehaviour::Ignore};

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    public:
    // Set filter string, or empty string to disable
    void setFilterString(const QString &search);
    // Set selected value behaviour
    void setSelectedStateBehaviour(SelectedStateBehaviour behaviour);
};
} // namespace JV2
