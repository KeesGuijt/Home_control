#!usrbinpython
#The libraries required
import serial  
import time  
import datetime

import sys
     
#A list (not in my case) of possible arduino devices.
locations=['COM11']

for device in locations:  
    try:  
        print "-Trying...",device  
        arduino = serial.Serial(device, 9600)
 	#arduino.setDTR(False)
        arduino.timeout = 0.5   
	#time.sleep(0.5)

        #Print the response from the Arduino - to show the communication has started.
        #for x in range(0, 10) :
        #     print arduino.readline()    
        #break 
    except:  
        print "-Serial timout",device     

print '-Received', len(sys.argv), 'arguments.'

if (bool(time.localtime( ).tm_isdst)) :
     print("-Summer")
else :    
     print("-Winter")   
try:  
     #Get the current PC time
     t = datetime.datetime.now();
     #Send the current time to the Arduino
     print ("-" + t.strftime("%Y-%m-%d %H:%M:%S"))
     #arduino.write(t.strftime("%Y-%m-%d %H:%M:%S"))
     arduino.write("T")
     if (bool(time.localtime( ).tm_isdst)):
         arduino.write(str(int(time.time())+7200)) #summer
     else :    
         arduino.write(str(int(time.time())+3600)) #winter
         print str(int(time.time())+3600)
     print "-Sent pc time T"+ str(int(time.time())+3600) + " to Arduino."
     #Receive the time string from above back from the Ardunio.

     #time.sleep(2) # delays for 5 seconds

     #sys.stdout.softspace=False # This stops a space being inserted between print statements
     if (len(sys.argv) == 2) :  
         kikaUnit = sys.argv[1]  #0 is the filename, 1 is the code to send
         arduino.write(str(kikaUnit) )
         print "-Sent control command " + kikaUnit + " to Arduino."

     if (len(sys.argv) == 3) :  
         kikaUnit = sys.argv[1]  #0 is the filename, 1 is the wait time in minutes
         arduino.write(str(kikaUnit) )
         kikaUnit = sys.argv[2]  #2 is the code to send
         arduino.write(" "+str(kikaUnit) )
         print "-Sent wait "+ kikaUnit + " and control command " + kikaUnit + " to Arduino."

     if (len(sys.argv) == 4) :  
         kikaUnit = sys.argv[1]  #0 is the filename, 1 is the wait time in minutes
         arduino.write(str(kikaUnit) )
         kikaUnit = sys.argv[2]  #2 is the code to send
         arduino.write(" "+ kikaUnit)
         print "-Sent wait "+ kikaUnit + " and control command " +kikaUnit+ " to Arduino, keeping console open."
         
         #permanent output
         while True :
           print arduino.readline()
   
except:
         print "Failed to send!"
         serial.close()
         exit() 

for x in range(0, 10) :
    for device in locations:  
        try:  
            #print "-Trying...",device  
            #arduino = serial.Serial(device, 9600)
            #arduino.setDTR(False)
            #arduino.timeout = 0.5   
            #time.sleep(0.5)

            #Print the response from the Arduino - to show the communication has started.
            #for x in range(0, 10) :
            print arduino.readline()    
            #break 
        except:  
            print "-Serial timout",device     
            serial.close()
            exit() 
     

