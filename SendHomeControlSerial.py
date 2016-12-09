#!usrbinpython
#The libraries required
import serial  
import time  
import datetime

import sys
#print 'Number of arguments', len(sys.argv), 'arguments.'
#print 'Argument List', str(sys.argv)

SLAAPKAMER = 0
HUISKAMER = 1

if (len(sys.argv) == 2) :  
     kikaUnit = sys.argv[1]  #0 is the filename, 1 is the code to send
     print(kikaUnit)
else :    
     print("No argument.")

     
#A list (not in my case) of possible arduino devices.
locations=['COM11']

if (bool(time.localtime( ).tm_isdst)) :
     print("Summer")
else :    
     print("Winter")

for device in locations:  
    try:  
        print "Trying...",device  
        arduino = serial.Serial(device, 9600)
 	#arduino.setDTR(False)
	#time.sleep(0.5)
        #Print the response from the Arduino - to show the communiction has started.
        print arduino.readline()    
        break 
    except:  
        print "Failed to connect on",device     
   
try:  
    if (HUISKAMER == 1) :
         #Get the current PC time
         t = datetime.datetime.now();
         #Send the current time to the Arduino
         print t.strftime("%Y-%m-%d %H:%M:%S")
         #arduino.write(t.strftime("%Y-%m-%d %H:%M:%S"))
         arduino.write("T")
         if (bool(time.localtime( ).tm_isdst)):
             arduino.write(str(int(time.time())+7200)) #summer
         else :    
             arduino.write(str(int(time.time())+3600)) #winter
         #Receive the time string from above back from the Ardunio.

         time.sleep(2) # delays for 5 seconds

    #sys.stdout.softspace=False # This stops a space being inserted between print statements
    if (len(sys.argv) == 2) :  
         kikaUnit = sys.argv[1]  #0 is the filename, 1 is the code to send
         arduino.write("T"+str(kikaUnit) )
    #arduino.writeln(kikaUnit)  #send kika command
    print "Sent control command."
    
    #//serial.close()
    #//exit(); 
    #while True :
    for x in xrange(1, 11):
         print arduino.readline()
  
except:
    print "Failed to send!"
 

