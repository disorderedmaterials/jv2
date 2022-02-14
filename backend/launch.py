# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin and T. Youngs

import subprocess
import os
print(os.getcwd())
os.chdir('/bin/launch')
print(os.getcwd())
isisInternal = subprocess.Popen('./isisInternal/isisInternal')
frontend = subprocess.Popen('../jv2')
for i in range(1, 10):
  print("")