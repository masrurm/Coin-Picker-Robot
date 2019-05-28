# import the necessary packages
from picamera.array import PiRGBArray
from picamera import PiCamera
import RPi.GPIO as GPIO
import time
import cv2

count = 0
sensitivity = 4

GPIO.setmode(GPIO.BOARD)
GPIO.setup(18,GPIO.OUT)
#p = GPIO.PWM(18,60)
#GPIO.cleanup()
#p.start(60)

image_q_x = 640
image_q_y = 480

raw_img_x = 2592
raw_img_y = 1944

img_y = raw_img_y /2 *0.8
img_h = raw_img_y /2 *0.4
img_x = 0
img_w = 800

'''
for i in range(1458,1464):
    img2 =cv2.imread("IMG_" + str(i) +".PNG")
    #cv2.imshow("Frame", img2)
    #cv2.waitKey(0)
    print "IMG_" + str(i) +".PNG -->" + str(cv2.mean(img2))
    time.sleep(0.2)

#ci2=img2[int(img_y):int(img_y+img_h),int(img_x):int(img_x+img_w)]
#cv2.imshow("Frame", ci2)
cv2.waitKey(0)
'''

'''
cap = cv2.VideoCapture('IMG_1467.MOV')

while(cap.isOpened()):
    ret, frame = cap.read()
    frame2 = cv2.resize(frame,(640,480))
    cv2.imshow('frame',frame2)
    mean = cv2.mean(frame)
    
    if count == 20:
        print str(mean)
        if (abs(oldmean[0] - mean[0]) > sensitivity and abs(oldmean[1] - mean[1]) > sensitivity and abs(oldmean[1] - mean[1]) > sensitivity):
            print "!!!!!!!!!!!!!!!!!!!!!!Coin!!!!!!!!!!!!!!!!!!!!!!!!"
            GPIO.output(18,GPIO.HIGH)
    else:
        count = count + 1        
    
    oldmean = mean
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    

GPIO.output(18,GPIO.LOW)
cap.release()
cv2.destroyAllWindows()

'''


# initialize the camera and grab a reference to the raw camera capture
camera = PiCamera()
camera.resolution = (image_q_x, image_q_y)
camera.framerate = 32
#rawCapture = PiRGBArray(camera, size=(640, 480))
rawCapture = PiRGBArray(camera)
# allow the camera to warmup
time.sleep(0.1)
 
# capture frames from the camera
for frame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
	# grab the raw NumPy array representing the image, then initialize the timestamp
	# and occupied/unoccupied text
	img = frame.array
	#cv2.imshow("Frame", img)
	#key = cv2.waitKey(1) & 0xFF
        
        #ci2 = img
        ci2 = img[200:600,0:550]
        #ci2 = img[int(img_y):int(img_y+img_h),int(img_x):int(img_x+img_w)]
	# show the frame
	#cv2.imshow("Frame", ci2)
	key = cv2.waitKey(1) & 0xFF
 
        #mean = cv2.mean(ci2)
        
        #frame2 = cv2.resize(ci2,(640,480))
        cv2.imshow('frame',ci2)
        mean = cv2.mean(ci2)
        
        
        if count == 20:
            print str(mean)
            if (abs(oldmean[1] - mean[1]) > sensitivity and abs(oldmean[1] - mean[1]) > sensitivity and abs(oldmean[1] - mean[1]) > sensitivity):
                print "!!!!!!!!!!!!!!!!!!!!!!Coin!!!!!!!!!!!!!!!!!!!!!!!!"
                GPIO.output(18,GPIO.HIGH)
                time.sleep(0.3)
            else:
                GPIO.output(18,GPIO.LOW)
        else:
            count = count + 1        
        
        oldmean = mean
        
        #if cv2.waitKey(1) & 0xFF == ord('q'):
        #    break        
 
	# clear the stream in preparation for the next frame
	rawCapture.truncate(0)
 
	# if the `q` key was pressed, break from the loop
	if key == ord("q"):
		break		