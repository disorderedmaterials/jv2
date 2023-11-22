# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

# Don't assume that the command which called the package is the main package
# This is needed for nix
import sys
import os.path
sys.argv[0] = os.path.dirname(sys.argv[0])

import app


if __name__ == "__main__":
    app.go()
