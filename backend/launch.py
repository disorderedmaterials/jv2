import subprocess

startupinfo = subprocess.STARTUPINFO()
startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
isisInternal = subprocess.Popen("../isisInternal/isisInternal",  startupinfo=startupinfo)
frontend = subprocess.Popen("../jv2/jv2")
