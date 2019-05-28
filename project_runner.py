import subprocess
import time

proc1 = subprocess.Popen(["/usr/bin/python","/home/pi/Desktop/ws.py"])
time.sleep(5) # sleep for 5 seconds
proc2 =subprocess.Popen(["/usr/bin/python","/home/pi/Desktop/coin.py"])