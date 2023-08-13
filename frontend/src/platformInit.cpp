// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 Team JournalViewer and contributors

// For static Qt builds dynamic platform plugins
// are not included by default.
// Here we include the import the relevant plugin
// to force the symbols to be included

#include <QtPlugin>

#if defined(Q_OS_LINUX)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
#else
#error "Static plugin configuration not supported on this platform."
#endif
