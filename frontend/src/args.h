// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef ARGS_H
#define ARGS_H

#include <QString>

struct Args
{
    const inline static QString RunLocatorClass = QStringLiteral("run-locator-class");
    const inline static QString RunLocatorPrefix = QStringLiteral("run-locator-prefix");
};

#endif
