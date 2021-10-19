import subprocess
import sys
import os

if getattr(sys, 'frozen', False):
    # we are running in a bundle
    bundle_dir = sys._MEIPASS
else:
    # we are running in a normal Python environment
    bundle_dir = os.path.dirname(os.path.abspath(__file__))

isisInternal = subprocess.Popen(os.path.join(bundle_dir, 'isisInternal'))
frontend = subprocess.Popen(os.path.join(bundle_dir, 'jv2/jv2'))
