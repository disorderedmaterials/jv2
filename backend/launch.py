# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin and T. Youngs

import subprocess

isisInternal = subprocess.Popen('./bin/launch/isisInternal/isisInternal')
frontend = subprocess.Popen('./bin/jv2')
