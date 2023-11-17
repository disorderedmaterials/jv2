// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#include "args.h"

namespace CLIArgs
{
/*
 * Environment Variables
 */
/**
 * Take a program argument name and convert to a backend environment variable name.
 * Replace '-' with '_' and add prefix
 */
QString argToEnvironName(QString argName) { return ENVIRON_NAME_PREFIX + argName.replace("-", "_").toUpper(); }
}; // namespace CLIArgs
