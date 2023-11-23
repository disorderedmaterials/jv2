// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2023 Team JournalViewer and contributors

#pragma once

#include <QString>

namespace CLIArgs
{
/*
 * Argument Name Strings
 */
const inline static QString LogLevel = QStringLiteral("log-level");
const inline static QString NoIDAaaS = QStringLiteral("no-idaaas");
const inline static QString NoISISArchive = QStringLiteral("no-isis-archive");
const inline static QString ISISArchiveDirectory = QStringLiteral("isis-archive-dir");
const inline static QString UseWaitress = QStringLiteral("use-waitress");
const inline static QString DebugBackend = QStringLiteral("debug-backend");

/*
 * Environment Variables
 */
// Environment variable prefix - must match that defined in backend/config module
constexpr auto ENVIRON_NAME_PREFIX = "JV2_";

/**
 * Take a program argument name and convert to a backend environment variable name.
 * Replace '-' with '_' and add prefix
 */
QString argToEnvironName(QString argName);
}; // namespace CLIArgs
