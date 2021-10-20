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

while(True):
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/platforms/qwindows.dll')):
        break
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/D3Dcompiler_47.dll')):
        break
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/libgcc_s_seh-1.dll')):
        break
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/libstdc++-6.dll')):
        break
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/libwinpthread-1.dll')):
        break
    if os.path.isfile(os.path.join(bundle_dir, 'jv2/lopengl32sw.dll')):
        break
isisInternal = subprocess.Popen(os.path.join(bundle_dir, 'isisInternal'))
frontend = subprocess.Popen(os.path.join(bundle_dir, 'jv2/jv2'))
