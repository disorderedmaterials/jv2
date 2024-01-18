// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2024 Team JournalSourceViewer and contributors

#include <QString>

// Return unique name for object
template <class Range, class NameFunction>
static QString uniqueName(const QString &baseName, const Range &objects, NameFunction nameFunction)
{
    // Ensure our base string is valid and set the starting unique name
    QString base = baseName.isEmpty() ? "UnnamedObject" : QString(baseName);
    QString uniqueName{base};

    // Iterate until we find an unused name
    auto suffix = 0;
    while (std::find_if(objects.begin(), objects.end(),
                        [nameFunction, &uniqueName](const auto &object)
                        { return !nameFunction(object).isEmpty() && nameFunction(object) == uniqueName; }) != objects.end())
        uniqueName = QString("%1%2").arg(base).arg(++suffix);

    return uniqueName;
}
