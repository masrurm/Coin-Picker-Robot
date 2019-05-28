from bottle import route, run, template
import urllib2
import RPi.GPIO as GPIO
import time


GPIO.setmode(GPIO.BOARD)
#GPIO.setwarnings(False)
GPIO.setup(40,GPIO.OUT)
p = GPIO.PWM(40,120)
#GPIO.cleanup()
p.start(50)

#p.ChangeFrequency(120)
@route('/b8/<command>')
def index(command):
    #print(command)
    try:
        p.ChangeFrequency(int(command) *  20 + 40)
        print command
        return "done"
    except ValueError:
        return "err"
    
    '''
    if(str(command) == "6"):
        p.ChangeFrequency(600)
    if(str(command) == "1"):
        p.ChangeFrequency(600)
    elif(str(command) == "2"):

    elif(str(command) == "3"):

        GPIO.output(40,GPIO.LOW)
    elif(str(command) == "4"):

        GPIO.output(40,GPIO.HIGH)
    elif(str(command) == "5"):
        GPIO.output(40,GPIO.HIGH)
    '''

'''
@route('/b8/<command>')
def index(command):
    #print(command)
    if(str(command) == "6"):
        GPIO.output(36,GPIO.HIGH)
        GPIO.output(38,GPIO.HIGH)
        GPIO.output(40,GPIO.HIGH)
    if(str(command) == "1"):
        GPIO.output(36,GPIO.HIGH)
        GPIO.output(38,GPIO.LOW)
        GPIO.output(40,GPIO.LOW)
    elif(str(command) == "2"):
        GPIO.output(36,GPIO.LOW)
        GPIO.output(38,GPIO.HIGH)
        GPIO.output(40,GPIO.LOW)
    elif(str(command) == "3"):
        GPIO.output(36,GPIO.HIGH)
        GPIO.output(38,GPIO.HIGH)
        GPIO.output(40,GPIO.LOW)
    elif(str(command) == "4"):
        GPIO.output(36,GPIO.LOW)
        GPIO.output(38,GPIO.LOW)
        GPIO.output(40,GPIO.HIGH)
    elif(str(command) == "5"):
        GPIO.output(36,GPIO.HIGH)
        GPIO.output(38,GPIO.LOW)
        GPIO.output(40,GPIO.HIGH)
'''        
    #contents = urllib2.urlopen("http://localhost:8080/b8/stop").read()
    #return template('<b>Hello {{name}}</b>!', name=name)
    #GPIO.output(12,GPIO.HIGH)

#run(host='206.87.123.238', port=8080)
run(host='172.20.10.14', port=8080, debug = False)
#run(host='localhost', port=8080)
'''
network={
        ssid="ubcsecure"
        scan_ssid=1
        key_mgmt=WPA-EAP
        eap=PEAP
        identity=""
        password=""
        phase2="auth=MSCHAPV2"
}


sudo nano /lib/systemd/system/ws.service
sudo systemctl daemon-reload
sudo systemctl enable ws.service

sudo ps -ax | grep python
systemctl status ws.service

[Unit]
Description=Start Clock

[Service]
Environment=DISPLAY=:0
Environment=XAUTHORITY=/home/pi/.Xauthority
ExecStart=/usr/bin/python /home/pi/Desktop/coin.py
Restart=always
RestartSec=10s
KillMode=process
TimeoutSec=infinity

[Install]
WantedBy=graphical.target


'''
