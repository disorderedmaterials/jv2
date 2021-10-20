import subprocess
import sys
import os
# import time

# time.sleep(5)

if getattr(sys, 'frozen', False):
    # we are running in a bundle
    bundle_dir = sys._MEIPASS
else:
    # we are running in a normal Python environment
    bundle_dir = os.path.dirname(os.path.abspath(__file__))

while(os.path.isdir(bundle_dir) is False):
    continue
isisInternal = subprocess.Popen(os.path.join(bundle_dir, 'isisInternal'))
frontend = subprocess.Popen(os.path.join(bundle_dir, 'jv2/jv2'))
