#!usrbinpython
#The libraries required
import serial  
import time  
import datetime

import sys
SLAAPKAMER = 0
HUISKAMER = 1

     
#A list (not in my case) of possible arduino devices.
locations=['COM11']


#Elro  4 button remote  - nieuwe settings    323ms
SLAAPK_BEELDSCH_AAN      =  4474193 # 10 (24Bit) device 1 on
SLAAPK_BEELDSCH_UIT      =  4474196 # 11 (24Bit) device 1 off 
DONT_USE_ON_AAN          =  4477265 # 12 (24Bit) device 2 on    
DONT_USE_ON_UIT          =  4477268 # 13 (24Bit) device 2 off   
SLAAPK__AAN     	 =  4478033 # 14 (24Bit) device 3 on    
SLAAPK__UIT       	 =  4478036 # 15 (24Bit) device 3 off   
SLAAPK_BUROLAMP_AAN      =  4478225 # 16 (24Bit) device 4 on    
SLAAPK_BUROLAMP_UIT      =  4478228 # 17 (24Bit) device 4 off   
#Dynamic energy tx,                       code 1410  177ms
SLAAPK_CHARGE_TX_AAN     =  4478259 # 30 (24Bit) device 1 on   
SLAAPK_CHARGE_TX_UIT     =  4478268 # 31 (24Bit) device 1 off 
SLAAPK_CHARGE_LIPO_AAN   =  4478403 # 32 (24Bit) device 2 on  
SLAAPK_CHARGE_LIPO_UIT   =  4478412 # 33 (24Bit) device 2 off 
SLAAPK_CHARGE_LAPTOP_AAN =  4478723 # 34 (24Bit) device 3 on  
SLAAPK_CHARGE_LAPTOP_UIT =  4478732 # 35 (24Bit) device 3 off 
#Klik-aan-klik-uit                        (draai schakelaars)
HUISK_LAMP_KAST_AAN      =  16405       # 40 (24Bit) device 1 on         FuzzyLogic   RemoteTransmitter.h  4400     
HUISK_LAMP_KAST_UIT      =  16404       # 41 (24Bit) device 1 off        FuzzyLogic   RemoteTransmitter.h  4398
#Elro  4 button remote                  zilver, Lianne         323ms
LIANNE_KERST_AAN         =  267345  # 20 (24Bit) device 3 on    
LIANNE_KERST_UIT         =  267348  # 21 (24Bit) device 3 off 
#Relay bar
SLAAPK_WD_HDD_AAN        =  800 # d3 on    
SLAAPK_WD_HDD_UIT        =  801 # d3 off   

#kikaUnit = SLAAPK_BEELDSCH_AAN     
#kikaUnit = SLAAPK_BEELDSCH_UIT     
#kikaUnit = DONT_USE_ON_AAN         
#kikaUnit = DONT_USE_ON_UIT         
#kikaUnit = SLAAPK__AAN     		
#kikaUnit = SLAAPK__UIT       		
#kikaUnit = SLAAPK_BUROLAMP_AAN     
#kikaUnit = SLAAPK_BUROLAMP_UIT     
#kikaUnit = SLAAPK_CHARGE_TX_AAN    
#kikaUnit = SLAAPK_CHARGE_TX_UIT    
#kikaUnit = SLAAPK_CHARGE_LIPO_AAN  
#kikaUnit = SLAAPK_CHARGE_LIPO_UIT  
#kikaUnit = SLAAPK_CHARGE_LAPTOP_AAN
#kikaUnit = SLAAPK_CHARGE_LAPTOP_UIT
#kikaUnit = SLAAPK_WD_HDD_AAN       
#kikaUnit = SLAAPK_WD_HDD_UIT       
#kikaUnit = HUISK_LAMP_KAST_AAN     
#kikaUnit = HUISK_LAMP_KAST_UIT     
#kikaUnit = LIANNE_KERST_AAN        
#kikaUnit = LIANNE_KERST_UIT        


for device in locations:  
    try:  
        print "-Trying...",device  
        arduino = serial.Serial(device, 9600)
 	#arduino.setDTR(False)
        arduino.timeout = 0.5   
	#time.sleep(0.5)

        #Print the response from the Arduino - to show the communication has started.
        for x in range(0, 10) :
             print arduino.readline()    
        break 
    except:  
        print "-Serial timout",device     

print '-Received', len(sys.argv), 'arguments.'

if (bool(time.localtime( ).tm_isdst)) :
     print("-Summer")
else :    
     print("-Winter")   
try:  
    if (HUISKAMER == 1) :
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
   
    #while True :
    #   print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    print arduino.readline()
    serial.close()
    exit() 
 
except:
    print "Failed to send!"
 

