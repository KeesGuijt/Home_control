/*
* Control RF 433 Mhz devices, IR, relay and read/write EEprom
* ERprom is used to store the time, reset_suppress and relay states, to survive short power outages
* Reset on serial conenct is disabled by a 50 ohm resistor on i/o pin (12) 
* 
* Connect the RF transmitter to digital pin 2, and the IR led to pin 3
*/


/*-----( Import needed libraries )-----*/
#include <TimeLib.h>
#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();
#include <IRremote.h>
IRsend irsend;
#include <EEPROM.h>

/*-----( Declare Constants )-----*/
#define HUISKAMER 1
#define SLAAPKAMER 0

#define TIME_HEADER  "T"   // Header tag for serial time sync message
const char *timeHeader = "T";// TIME_HEADER;
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

#define TIME_HOUR_ON  15     //per month a correction is added
#define TIME_HOUR_OFF  23  
#define RELAY_ON 0
#define RELAY_OFF 1
#define Relay_0  4  // Arduino Digital I/O pin number
#define Relay_1  5
#define Relay_2  6
#define Relay_3  7
#define Relay_4  8  
#define Relay_5  9
#define Relay_6  10
#define Relay_7  11
#define ResetSuppressPin  12

//Elro  4 button remote  - nieuwe settings    323ms
//Dynamic energy tx, code 1410  177ms
#define SLAAPK_CHARGE_TX_AAN      4478259 // 30 (24Bit) device 1 on   
#define SLAAPK_CHARGE_LAPTOP_UIT  4478732 // 35 (24Bit) device 3 off 
//Klik-aan-klik-uit   (draai schakelaars)
#define HUISK_LAMP_KAST_AAN       16405   // 40 (24Bit) device 1 on         FuzzyLogic   RemoteTransmitter.h  4400     
#define HUISK_LAMP_KAST_UIT       16404   // 41 (24Bit) device 1 off        FuzzyLogic   RemoteTransmitter.h  4398
//Elro  4 button remote zilver, Lianne    323ms
//Relay bar
#define SLAAPK_WD_HDD_AAN         800 // d3 on    
//ResetSuppress
#define RESET_SUPPRESS_AAN         100 // d11 on    
#define RESET_SUPPRESS_UIT         101 // d11 on    

/*-----( Declare Variables )-----*/
const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

//int eeAddress = 0; //EEPROM address to start reading from

int season = 1; //0 summer, 1 winter
int timeOn = 0; 
int timeOff = 0; 

unsigned long pctime;

unsigned long rfControlCode;
unsigned int period;





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
    int handle();

  private:
    const int EEpromAddress;           // pin to which button is connected
    unsigned long pctime; 
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
};


class RFHandler {    //"pre-declaration"
  public:
    // Constructor
    RFHandler(int o_pin); 
    
    // Initialization done after construction, to permit static instances
    void init();

    // Handler, to be called in the loop()
    void sendCommand(unsigned long rfControlCode);

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

//#endif
//end - This could be in a header file


//start - This could be in a .cpp file
//#include "Arduino.h"
//#include "Morse.h"

//member definitions

ClockHandler::ClockHandler(int EEAddress)  //constructor definition
:  EEpromAddress(EEAddress)
{
}

void ClockHandler::init()
{
  //Get the time data from the EEPROM at position 'EEpromAddress'
  unsigned long pctime; 
  //EEpromAddress = 0;
  EEPROM.get(EEpromAddress, pctime);
  if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
       Serial.println("Arduino clock set from Eeprom");
  }
}

int ClockHandler::handle()
{
     int event=0;

    //every 30 min write to EEPROM to let clock survive resets by PC
    if ( ( (minute() == 0) or (minute() == 30) ) and (second() <2) )   //only close to beginning of these minutes
    {  
      pctime=now();
      //Put the float data from the EEPROM at position 'eeAddress'
      EEPROM.put(EEpromAddress, pctime);
      Serial.print("Timeinfo written to EEPROM.");
      delay(2000);
    } 

    if (HUISKAMER == 1)
    {
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
          Serial.print("On/Off times:");
          Serial.print(timeOn);
          Serial.println(timeOff);
          if ( (hour() == timeOn) and (minute() == 0) and (second() < 5) ) 
          {
              event = HUISK_LAMP_KAST_AAN;
          }
          if ( (hour() == timeOff) and (minute() == 0) and (second() < 5) ) 
          {  
              event = HUISK_LAMP_KAST_UIT;
          }
    }
    return event;
}

void ClockHandler::set(unsigned long p_time)
{
     if( p_time >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
        setTime(pctime); // Sync Arduino clock to the time received on the serial port
        //Put the float data from the EEPROM at position 'eeAddress'
        //eeAddress = 0;
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
                 //Serial.println(outputPin);
                 Serial.println(command);
            }
            if ( command == codeForOff ) 
            {
                pinMode(outputPin, INPUT);  //pulling the resetpin down is not a good idea....
                Serial.print("Controlled ResetSuppressPin\n");
                EEPROM.put(EEpromAddress, command);
                Serial.print("Written ResetSuppressPin state to EEPROM: ");
                //Serial.println(outputPin);
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
                //Serial.println(outputPin);
                Serial.println(command);
            }
            if ( command == codeForOff ) 
            {
                pinMode(outputPin, OUTPUT);   
                digitalWrite(outputPin, RELAY_OFF);// set the Relay OFF
                Serial.print("Controlled relay\n");
                EEPROM.put(EEpromAddress, command);
                Serial.print("Written relay state to EEPROM ");
                //Serial.println(outputPin);
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
  // Transmitter is connected to Arduino Pin #2  
  mySwitch.enableTransmit(outputPin);

}

void RFHandler::sendCommand(unsigned long rfControlCode)
{
  
  //set RF pulselength and send if needed
  if ( rfControlCode > 881 ) 
  {
      if ( (rfControlCode >= SLAAPK_CHARGE_TX_AAN ) && (rfControlCode <= SLAAPK_CHARGE_LAPTOP_UIT ) )
      {
           mySwitch.setPulseLength(177);
      }
      else if ( (rfControlCode >= HUISK_LAMP_KAST_UIT ) && (rfControlCode <= HUISK_LAMP_KAST_AAN ) )
      {
           mySwitch.setPulseLength(360);
      }
      else
      {
          mySwitch.setPulseLength(324);
          Serial.print("Pl 324. ");
      }   
      Serial.print("Sending RF command.");
      Serial.print(rfControlCode);
      mySwitch.send (rfControlCode, 24);
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

//end - This could be in a .cpp file



//////////////////////////////////////////////////////////////////////////////

// Instantiate objects
ClockHandler Myclock(0); //first EEprom address
PinDeviceHandler ResetSuppress(12,sizeof(unsigned long)*1,100,101);
PinDeviceHandler Relay0(4,sizeof(unsigned long)*2,800,801);
PinDeviceHandler Relay1(5,sizeof(unsigned long)*3,810,811);
RFHandler MyRFDevices(2); //pin numbr
IR_Handler MyIRDevices(3); 


void setup()  {
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
  pinMode(13, OUTPUT);

  //RcSwitch
  // Transmitter is connected to Arduino Pin #2  
  //mySwitch.enableTransmit(2);

  Myclock.init();
  ResetSuppress.init();
  Relay0.init();
  Relay1.init();
  MyRFDevices.init();

  digitalClockDisplay();
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
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

void processSyncMessage() {
  if(Serial.find(*timeHeader)) {
     pctime = Serial.parseInt();
     Myclock.set(pctime);
     rfControlCode = pctime;
     
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

void loop() {

  rfControlCode = 0; 
  rfControlCode=Myclock.handle();   //get timed commands

  //check for time from pc or serial control command
  if (Serial.available()) {
    processSyncMessage();
  }
  
  if (timeStatus()!= timeNotSet) {
    digitalClockDisplay();  
  }

  //set pin or send code if needed
  ResetSuppress.setByCommand(rfControlCode);
  Relay0.setByCommand(rfControlCode);
  Relay1.setByCommand(rfControlCode); 
  MyIRDevices.sendCommand(rfControlCode);
  MyRFDevices.sendCommand(rfControlCode);
  
  // Need interrupts for delay()
  interrupts();
  delay(500);
  digitalWrite(13, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);
  digitalWrite(13, LOW);   // turn the LED off
}
