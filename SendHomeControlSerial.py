#!usrbinpython
#The libraries required
import serial  
import datetime
import sys
import time
from time import sleep

#A list (not in my case) of possible arduino devices.
locations=['COM11']

for device in locations:  
    try:  
        print "-Trying...",device  
        arduino = serial.Serial(device, 9600)
        arduino.timeout = 0.1
    except:  
        print "-Serial timout",device     

if (bool(time.localtime( ).tm_isdst)) :
     print("-Summer")
else :    
     print("-Winter")
     
try:  
     #Get the current PC time
     t = datetime.datetime.now();
     #Send the current time to the Arduino
     print ("-" + t.strftime("%Y-%m-%d %H:%M:%S"))
     arduino.write("T")
     if (bool(time.localtime( ).tm_isdst)):
         arduino.write(str(int(time.time())+7200)) #summer
         print "-Sent pc time T"+ str(int(time.time())+7200) + " to Arduino."
     else :    
         arduino.write(str(int(time.time())+3600)) #winter
         print "-Sent pc time T"+ str(int(time.time())+3600) + " to Arduino."

     print '-Received', len(sys.argv), 'arguments.'

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
                   
except:
         print "-Failed to send!"


for x in range(0, 20) :
    #Print the response from the Arduino - to show the communication has started.
    sleep(0.05)
    if (arduino.inWaiting()) :
        sys.stdout.softspace=False # This stops a space being inserted between print statements
        print arduino.readline().rstrip('\n') 
         
if (len(sys.argv) == 1) :  #started without arguments really
  while True :    
     print('-Waiting... please press Ctrl-C when you wish to enter a command string.')
     try:
          for i in range(0, 2*60): 
              time.sleep(1)

              for x in range(0, 30) :
                    if (arduino.inWaiting()) :
                        print arduino.readline().rstrip('\n') 

          print("-Oops, sorry, no input was received today. Come back tomorrow.")
     except KeyboardInterrupt:
            command = raw_input("-Enter command string? ")
            print (" Sending: " + command)
            arduino.write(command)
