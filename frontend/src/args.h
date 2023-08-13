// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 Team JournalViewer and contributors

#pragma once

#include <QString>

struct Args
{
    const inline static QString LogLevel = QStringLiteral("log-level");
    const inline static QString RunLocatorClass = QStringLiteral("run-locator-class");
    const inline static QString RunLocatorPrefix = QStringLiteral("run-locator-prefix");
};

