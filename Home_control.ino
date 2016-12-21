/*
* Control RF 433 Mhz devices, IR, relay and read/write EEprom
* ERprom is used to store the time, reset_suppress and relay states, to survive short power outages
* Reset on serial conenct is disabled by a 50 ohm resistor on i/o pin (12) 
* 
* Connect the RF transmitter to digital pin 2, and the IR led to pin 3



// - This could be in a header file "HomeControl.h"

*/

/*-----( Import needed libraries )-----*/
#include <TimeLib.h>
#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();
#include <IRremote.h>
IRsend irsend;
#include <EEPROM.h>

/*-----( Declare Constants )-----*/
#define TIME_HOUR_ON  15     //per month a correction is added
#define TIME_HOUR_OFF  23  

//Timed devices
#define HUISK_LAMP_KAST_AAN       4478259  
#define HUISK_LAMP_KAST_UIT       4478268  
#define HUISK_KERSTBOOM_AAN       4478403  
#define HUISK_KERSTBOOM_UIT       4478412  

unsigned long processMessage(void);

//////////////////////////////////////////////////////////////////////////////
// Class definitions


class ClockHandler {    //"pre-declaration"
  public:
    // Constructor
    ClockHandler(int EEpromAddress); 

    // Initialization done after construction, to permit static instances
    void init();
      //read eeprom, say time
      
    void set(unsigned long p_time);
      //set time, write to eeprom, say time    

    // Handler, to be called in the loop()
    unsigned long handle();

  private:
    const int EEpromAddress;           // pin to which button is connected
    unsigned long pctime; 
    const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
    //2016-12-14 10:49:16  T1481712556
    int timeOn = 0; 
    int timeOff = 0; 
};

class PinDeviceHandler {    //"pre-declaration"
  public:
    // Constructor
    PinDeviceHandler(int o_pin, int EEpromAddress, unsigned long codeOn, unsigned long codeOff); 

    // Initialization done after construction, to permit static instances
    void init();
      //read eeprom, set pin
      
    //by command, to be called in the loop()
    void setByCommand(unsigned long command);

  private:
    const int EEpromAddress;           // pin to which button is connected
    int outputPin; 
    unsigned long codeForOn; 
    unsigned long codeForOff; 
    #define RELAY_ON 0
    #define RELAY_OFF 1
};


class RFHandler {    //"pre-declaration"
  public:
    // Constructor
    RFHandler(int o_pin); 
    
    // Initialization done after construction, to permit static instances
    void init();

    // Handler, to be called in the loop()
    void sendCommand(unsigned long controlCode);

  private:
    int outputPin; 
};

class IR_Handler {    //"pre-declaration"
  public:
    // Constructor
    IR_Handler(int outputPin); 
    
    // Handler, to be called in the loop()
    void sendCommand(unsigned long irControlCode);

  private:
    int outputPin; 
};

class EEpromList_Handler {    //"pre-declaration"
  public:
    // Constructor
    EEpromList_Handler(int firstAddr, int lastAddr); 
    
    // Initialization done after construction, to permit static instances
    void init(void);
    
    // Handler, to be called in the loop()
    void writeCommand(unsigned long controlCodeW, int timeoutHour, int timeoutMinute);
    unsigned long readCommand(int timeoutHour, int timeoutMinute);     //return event
    void listCommands(void);

  private:
    int firstAddr; 
    int lastAddr; 
    int timeoutHour;
    int timeoutMinute;
    int seekCommand(unsigned long timeoutCommand, int timeoutHour, int timeoutMinute, bool printIt);   
};


//#endif
//end - This could be in a header file




//start - This could be in a .cpp file
//#include "Arduino.h"
//#include "HomeControl.h"

//member definitions

ClockHandler::ClockHandler(int EEAddress)  //constructor definition
:  EEpromAddress(EEAddress)
{
}

void ClockHandler::init()
{
      //Get the time data from the EEPROM at position 'EEpromAddress'
      unsigned long pctime; 
      EEPROM.get(EEpromAddress, pctime);
      if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
           setTime(pctime); // Sync Arduino clock to the time received on the serial port
           Serial.println("Arduino clock set from Eeprom");
      }
}

unsigned long ClockHandler::handle()
{
    unsigned long event=0;

    //every 30 min write to EEPROM to let clock survive resets by PC
    if ( ( (minute() == 0) or (minute() == 30) ) and (second() <2) )   //only close to beginning of these minutes
    {  
      pctime=now();
      //Put the float data from the EEPROM at position 'eeAddress'
      EEPROM.put(EEpromAddress, pctime);
      Serial.print("Timeinfo written to EEPROM.");
      delay(1000);
    } 

    //check time, see if RF switch control is planned
    /*  times are calculated:
    int(6.5-abs(month()-6.5)) 
    jan 16
    feb 17
    mrt 18
    apr 19
    mei 20
    jun 21
    jul 21
    aug 20
    sep 19
    okt 18
    nov 17
    dec 16
    */
    timeOn = TIME_HOUR_ON + int(6.5-abs(month()-6.5));
    timeOff = TIME_HOUR_OFF;
    if ( (hour() == timeOn) and (minute() == 0) and (second() < 5) ) 
    {
        event = HUISK_LAMP_KAST_AAN;
    }
    if ( (hour() == timeOff) and (minute() == 0) and (second() < 5) ) 
    {  
        event = HUISK_LAMP_KAST_UIT;
    }
    if ( (hour() == timeOn) and (minute() == 1) and (second() < 5) ) 
    {
        event = HUISK_KERSTBOOM_AAN;
    }
    if ( (hour() == timeOff) and (minute() == 1) and (second() < 5) ) 
    {  
        event = HUISK_KERSTBOOM_UIT;
    }

    if (event == 0 )
    {
        EEpromList_Handler ReadTimeoutList(100,260);  //first/last EEprom address
        event = ReadTimeoutList.readCommand(hour(), minute());
    }
    return event;
}

void ClockHandler::set(unsigned long p_time)
{
    if( p_time >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
        setTime(p_time); // Sync Arduino clock to the time received on the serial port
        //Put the float data from the EEPROM at position 'eeAddress'
        EEPROM.put(EEpromAddress, p_time);
        Serial.println("Timeinfo from PC set and written to EEPROM.");
    }
}


PinDeviceHandler::PinDeviceHandler(int o_pin, int EEAddress, unsigned long codeOn, unsigned long codeOff)  //constructor definition
:  outputPin(o_pin), EEpromAddress(EEAddress), codeForOn(codeOn), codeForOff(codeOff)
{
}

void PinDeviceHandler::init()
{
    //get state from EEprom, set pin
    unsigned long command=0;
    EEPROM.get(EEpromAddress, command);    
    if ( (command >= 100) && (command <= 881) )
    {
        Serial.print("Retrieved control command from EEPROM: ");
        Serial.println(command);
    }
    else
    {
        command = 0;
    }
    
   if ( (command >= 100) && (command <= 101) ) 
    {
        if ( command == codeForOn ) 
        {
            digitalWrite(outputPin, 1);// set the ResetSuppressPin ON
            pinMode(outputPin, OUTPUT);  
            Serial.print("Controlled ResetSuppressPin ");
            Serial.println(command);
        }
        if ( command == codeForOff ) 
        {
            pinMode(outputPin, INPUT);  //pulling the resetpin down is not a good idea....
            Serial.print("Controlled ResetSuppressPin ");
            Serial.println(command);
        }
    }
    if ( (command >= 800) && (command <= 881) ) 
    {
       if ( command == codeForOn ) 
        {
            pinMode(outputPin, OUTPUT);   
            digitalWrite(outputPin, RELAY_ON);// set the Relay ON
            Serial.print("Controlled relay ");
            Serial.println(command);
        }
        if ( command == codeForOff ) 
        {
            pinMode(outputPin, OUTPUT);   
            digitalWrite(outputPin, RELAY_OFF);// set the Relay ON
            Serial.print("Controlled relay ");
            Serial.println(command);
        }
     }  
}

void PinDeviceHandler::setByCommand(unsigned long command)
{
   if ( (command >= 100) && (command <= 101) ) 
   {
        if ( command == codeForOn ) 
        {
             digitalWrite(outputPin, 1);// set the ResetSuppressPin ON
             pinMode(outputPin, OUTPUT);  
             EEPROM.put(EEpromAddress, command);
             Serial.print("Written ResetSuppressPin state to EEPROM: ");
             Serial.println(command);
        }
        if ( command == codeForOff ) 
        {
            pinMode(outputPin, INPUT);  //pulling the resetpin down is not a good idea....
            digitalWrite(outputPin,0);// set the ResetSuppressPin OFF
            Serial.print("Controlled ResetSuppressPin\n");
            EEPROM.put(EEpromAddress, command);
            Serial.print("Written ResetSuppressPin state to EEPROM: ");
            Serial.println(command);
        }
   }
   if ( (command >= 800) && (command <= 881) ) 
   {
        if ( command == codeForOn ) 
        {
            pinMode(outputPin, OUTPUT);   
            digitalWrite(outputPin, RELAY_ON);// set the Relay ON
            Serial.print("Controlled relay\n");
            EEPROM.put(EEpromAddress, command);
            Serial.print("Written relay state to EEPROM ");
            Serial.println(command);
        }
        if ( command == codeForOff ) 
        {
            pinMode(outputPin, OUTPUT);   
            digitalWrite(outputPin, RELAY_OFF);// set the Relay OFF
            Serial.print("Controlled relay\n");
            EEPROM.put(EEpromAddress, command);
            Serial.print("Written relay state to EEPROM ");
            Serial.println(command);
        }
    }  
}

RFHandler::RFHandler(int o_pin)  //constructor definition
:  outputPin(o_pin)
{
}

void RFHandler::init()
{
  //RcSwitch
  //Transmitter is connected to Arduino Pin #2  
  mySwitch.enableTransmit(outputPin);

}

void RFHandler::sendCommand(unsigned long controlCode)
{
  
  //set RF pulselength and send if needed
  if ( (controlCode > 881) && (controlCode < 1482313385) ) 
  {
      if ( (controlCode >= 4478259 ) && (controlCode <= 4478732 ) )
      {
           mySwitch.setPulseLength(177);  //Dynamic energy tx
      }
      else if ( (controlCode >= 16404 ) && (controlCode <= 16405 ) )
      {
           mySwitch.setPulseLength(360); //Kika
      }
      else
      {
          mySwitch.setPulseLength(324);  //Elro
          Serial.print("Pl 324. ");  
      }   
      Serial.print("Sending RF command.");
      Serial.print(controlCode);
      mySwitch.send (controlCode, 24);
  }
}

IR_Handler::IR_Handler(int o_pin)  //constructor definition
:  outputPin(o_pin)  
{
}

void IR_Handler::sendCommand(unsigned long irControlCode)
{ 
  //send IR if needed 
  if ( (irControlCode == 200) ) 
  {
     for (int i = 0; i < 3; i++) {
          irsend.sendNEC(0x10EFA05F, 32); // Telefunken TV power code
          delay(40);            
      }
     Serial.print("Written Power button to IR \n");
  }
}

EEpromList_Handler::EEpromList_Handler(int firstAddress, int lastAddress)  //constructor definition
:  firstAddr(firstAddress), lastAddr(lastAddress)  
{
}

void EEpromList_Handler::writeCommand(unsigned long controlCodeW, int timeoutHour, int timeoutMinute)
{ 
    int itemFound = 0;
    int timeoutListAddress = 0;
    //check for timeout commands
    //if found, use that EEPprom item
    //else find first empy line 

    //check list
    timeoutListAddress = seekCommand(controlCodeW, 0, 0, false);   //find a controlcode
   
    // if not found, create a new item in first free slot found
    if (timeoutListAddress == 0)
    {
        timeoutListAddress = seekCommand(0, 0, 0, false);   //find a controlcode
    }
    
    //update or add slot
    if (timeoutListAddress > 0)
    {
         EEPROM.put(timeoutListAddress, controlCodeW);
         EEPROM.put(timeoutListAddress+sizeof(unsigned long), timeoutHour);
         EEPROM.put(timeoutListAddress+sizeof(unsigned long)+sizeof(int), timeoutMinute);  
         Serial.print("Item ");      
         Serial.print(controlCodeW);      
         Serial.print(" written to timeout list on address ");      
         Serial.println(timeoutListAddress);      
    }
}


unsigned long EEpromList_Handler::readCommand(int timeoutHour, int timeoutMinute)      //return event
{ 
    
    unsigned long event=0;
    int timeoutListAddress=0;

    //check list
    timeoutListAddress = seekCommand(0, timeoutHour, timeoutMinute, false);   //find a controlcode it is time for
    if (timeoutListAddress > 0)
    {
        //Serial.print("slot : ");
        //Serial.print(timeoutListAddress);
        
        EEPROM.get(timeoutListAddress, event); 
        //Now erase the item
        const unsigned long fourBytes=0; 
        const int twoBytes=0;
        EEPROM.put(timeoutListAddress, fourBytes);
        EEPROM.put(timeoutListAddress+sizeof(unsigned long), twoBytes);
        EEPROM.put(timeoutListAddress+sizeof(unsigned long)+sizeof(int), twoBytes); 
    }
    if (event > 0 )
    {
       //Serial.print("command : ");
       //Serial.print(event);
    }
    return event;
}


void EEpromList_Handler::listCommands(void)
{ 
    //dump list
    int dummy = seekCommand(999999, 99, 99, true);   //random numbers, just print
}        


int EEpromList_Handler::seekCommand(unsigned long timeoutCommand, int timeoutHour, int timeoutMinute, bool printIt)   
{ 
  
    //dump list
    int slot = 0;
    int timeoutListAddress = 0;
    int twoBytes = 0;
    unsigned long timeoutListItemCommand=0;
    //check for timeout commands
    //if found, use that EEPprom slot
    //else find first empy line 

    slot = 0;
    for (timeoutListAddress=firstAddr; timeoutListAddress < lastAddr; timeoutListAddress+=8)
    {
      //int EEpromAddress=timeoutListAddress;
      EEPROM.get(timeoutListAddress, timeoutListItemCommand); 
      if (timeoutListItemCommand==timeoutCommand && slot==0)  //first item that matches timeoutCommand
      {
        slot = timeoutListAddress;
      }
 
      if (timeoutListItemCommand > 0)     //slot is in use
      { 
        if (slot==0) 
            { 
            EEPROM.get(timeoutListAddress, timeoutListItemCommand); 
            EEPROM.get(timeoutListAddress+sizeof(unsigned long), timeoutHour);
            EEPROM.get(timeoutListAddress+sizeof(unsigned long)+sizeof(int), timeoutMinute);
            if ( (hour() == timeoutHour) && (minute() == timeoutMinute) && (second() < 5) ) 
            {
              slot = timeoutListAddress;
              if (!printIt)
              {
                  Serial.print("Found to do: ");
                  Serial.println(timeoutListItemCommand);          
              }
            }
        }
        
        if (printIt)
        { 
          Serial.print(timeoutListItemCommand); 
          Serial.print(" "); 
          EEPROM.get(timeoutListAddress+sizeof(unsigned long), twoBytes);
          Serial.print(twoBytes); 
          Serial.print(":"); 
          EEPROM.get(timeoutListAddress+sizeof(unsigned long)+sizeof(int), twoBytes);
          Serial.println(twoBytes); 

          /*
          ///Now erase the item   - on time only
          const unsigned long fourBytes=0; 
          const int twoBytes=0;
          EEPROM.put(timeoutListAddress, fourBytes);
          EEPROM.put(timeoutListAddress+sizeof(unsigned long), twoBytes);
          EEPROM.put(timeoutListAddress+sizeof(unsigned long)+sizeof(int), twoBytes); 
          */      
        }
      }     
    }    
    return slot;   //first match (command or time)
}        
//end - This could be in a .cpp file



//////////////////////////////////////////////////////////////////////////////

// Instantiate objects
ClockHandler Myclock(0); //first EEprom address
EEpromList_Handler TimeoutList(100,260);  //first/last EEprom address
PinDeviceHandler ResetSuppress(12,sizeof(unsigned long)*1,100,101);
PinDeviceHandler Relay0(4,sizeof(unsigned long)*2,800,801);
PinDeviceHandler Relay1(5,sizeof(unsigned long)*3,810,811);
PinDeviceHandler Relay2(6,sizeof(unsigned long)*4,820,821);
PinDeviceHandler Relay3(7,sizeof(unsigned long)*5,830,831);
PinDeviceHandler Relay4(8,sizeof(unsigned long)*6,840,841);
PinDeviceHandler Relay5(9,sizeof(unsigned long)*7,850,851);
PinDeviceHandler Relay6(10,sizeof(unsigned long)*8,860,861);
PinDeviceHandler Relay7(11,sizeof(unsigned long)*9,870,871);
RFHandler MyRFDevices(2); //pin numbr
IR_Handler MyIRDevices(3); 


void setup()  {
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
  pinMode(13, OUTPUT);

  
  Myclock.init();    
 
  ResetSuppress.init();
  
  Relay0.init();
  Relay1.init();
  Relay2.init();
  Relay3.init();
  Relay4.init();
  Relay5.init();
  Relay6.init();
  Relay7.init();
  MyRFDevices.init();

  digitalClockDisplay();

  //dump timeout list
  bool itemFound = 0;
  int timeoutListAddress = 0;

  TimeoutList.listCommands();  
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

// return T, W + C or C command
unsigned long processMessage(void) {

  int waitTime = 0; 
  unsigned long controlCode = 0; 

  const char *timeHeader = "T";// Header tag for serial time sync message
  const char *commandHeader = "C";// Header tag for serial device command message
  const char *waitHeader = "W";// Header tag for serial wait minutes message

  char *header;
  *header = Serial.read(); 

  if(*header == *timeHeader) 
  {
     controlCode = Serial.parseInt();
  }
  if( *header == *waitHeader)  
  {
     waitTime = Serial.parseInt();
     Serial.print("Wait ");   
     Serial.print(waitTime);      
     unsigned long timeoutTime = now()+(waitTime*60); //minutes to seconds
     int timeoutHour= hour(timeoutTime);
     int timeoutMinute= minute(timeoutTime);
     Serial.print(" ;planned "); 
     Serial.print(timeoutHour); 
     Serial.print(":"); 
     Serial.print(timeoutMinute); 
     
     //W must be followed by C
	 for (int i = 0; i < 3; i++)      //process space(s)
     {
        if (Serial.read() == *commandHeader)
        {
           Serial.print(", command ");         
           controlCode =  Serial.parseInt();
           Serial.println (controlCode);
           Serial.println (" ");
        }
     }
     TimeoutList.writeCommand(controlCode,timeoutHour,timeoutMinute);
     
     controlCode = 0; //no command right now 
  }
  else
  {
      if(*header == *commandHeader) 
      {
         controlCode = Serial.parseInt();
      }
  }

  return controlCode;
}


void loop() {
  unsigned long controlCode;
  
  controlCode = 0;  //start blank every loop

  //display some info periodically
  if (timeStatus()!= timeNotSet && (second() == 0)) {
    digitalClockDisplay(); 
    int timeOn = TIME_HOUR_ON + int(6.5-abs(month()-6.5));
    int timeOff = TIME_HOUR_OFF;
    Serial.print("On/Off times: ");
    Serial.print(timeOn);
    Serial.print(" & ");
    Serial.println(timeOff);

    //dump list
    TimeoutList.listCommands();   
    Serial.println(" ");
    
    delay(1000); //limit output
  }
  
  //check for time from pc or serial control command
  if (Serial.available()) {
    controlCode=processMessage();
  }

  Myclock.set(controlCode);  //set if received   
  if (controlCode == 0)
  {
    controlCode=Myclock.handle();   //get timed commands
  }  

  //set pin or send code if needed
  ResetSuppress.setByCommand(controlCode); 
  Relay0.setByCommand(controlCode);
  Relay1.setByCommand(controlCode); 
  Relay2.setByCommand(controlCode);
  Relay3.setByCommand(controlCode); 
  Relay4.setByCommand(controlCode);
  Relay5.setByCommand(controlCode); 
  Relay6.setByCommand(controlCode);
  Relay7.setByCommand(controlCode); 
  MyIRDevices.sendCommand(controlCode);
  MyRFDevices.sendCommand(controlCode);

  // Need interrupts for delay()
  interrupts();
  delay(100);
  digitalWrite(13, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);
  digitalWrite(13, LOW);   // turn the LED off

}

