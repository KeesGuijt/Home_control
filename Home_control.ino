/*
    Control RF 433 Mhz devices, IR, relay and read/write EEprom
    EEprom is used to store the time, reset_suppress and relay states, to survive short power outages. EEprom is also used to store an action list (timed commands)
    Reset on serial connect is disabled by a 50 ohm resistor on i/o pin (12)
    Connect the RF transmitter to digital pin 2, and the IR led to pin 3


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
#define TIME_HOUR_ON    15         //per month a correction is added
#define TIME_HOUR_OFF   23

//Timed devices
#define HUISK_LAMP_KAST_AAN             4478259
#define HUISK_LAMP_KAST_UIT             4478268
#define HUISK_KERSTBOOM_AAN             4478403
#define HUISK_KERSTBOOM_UIT             4478412
#define SLAAPK_BUROLAMP_AAN             830
#define SLAAPK_BUROLAMP_UIT             831
#define SLAAPK_BEELDSCH_AAN             16405
#define SLAAPK_BEELDSCH_UIT             16404
#define SLAAPK_TV_AAN                   200
#define SLAAPK_KACHEL_AAN               4478723
#define SLAAPK_KACHEL_UIT               4478732

unsigned long processMessage(void);

bool bureauLampOnFlag = false;         //remember actual state of this device
bool pcMonitorsOnFlag = false;         //remember actual state of this device
bool kachelOnFlag = false;             //remember actual state of this device
bool tvOnFlag = false;                 //remember actual state of this device
bool bovenViaPirDetected = false;      //set permanently if pir activitySamples >= 45) (meaning "this can only be upstairs unit, it is the only unit with a pir device")
bool pirArmedFlag = false;             //receiving any serial command means pc is on, monitors will be switched with pir; set pirArmedFlag
                                       //reset on SLAAPK_BEELDSCH_UIT || SLAAPK_BUROLAMP_UIT
                                       //pirArmedFlag only checked for "monitor on" actions.
                                       //it's purpose is preventing monitor on if pc is off.
                                       //active (mon on pir on) after upper macro button (>>|)
                                       //inactive after lower macro button (|<<)

//////////////////////////////////////////////////////////////////////////////
// Class definitions


class ClockHandler {        //"pre-declaration"
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
    const int EEpromAddress;                     // pin to which button is connected
    unsigned long pctime;
    const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
    //example: 2016-12-14 10:49:16 = T1481712556
    int timeOn = 0;
    int timeOff = 0;
};

class PinDeviceHandler {        //"pre-declaration"
  public:
    // Constructor
    PinDeviceHandler(int o_pin, int EEpromAddress, unsigned long codeOn, unsigned long codeOff);

    // Initialization done after construction, to permit static instances
    void init();
    //read eeprom, set pin

    //by command, to be called in the loop
    void setByCommand(unsigned long command);

  private:
    const int EEpromAddress;                     // pin to which button is connected
    int outputPin;
    unsigned long codeForOn;
    unsigned long codeForOff;
#define RELAY_ON 0
#define RELAY_OFF 1
};

class RFHandler {        //"pre-declaration"
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

class IR_Handler {        //"pre-declaration"
  public:
    // Constructor
    IR_Handler(int outputPin);

    // Handler, to be called in the loop()
    void sendCommand(unsigned long irControlCode);

  private:
    int outputPin;
};

class EEpromList_Handler {        //"pre-declaration"
  public:
    // Constructor
    EEpromList_Handler(int firstAddr, int lastAddr);

    // Initialization done after construction, to permit static instances
    void init(void);

    // Handler, to be called in the loop()
    void writeCommand(unsigned long controlCodeW, int timeoutHour, int timeoutMinute);
    unsigned long readCommand();         //return event
    void listCommands(void);

  private:
    int firstAddr;
    int lastAddr;
    int timeoutHour;
    int timeoutMinute;
    int seekCommand(unsigned long timeoutCommand, int timeoutHour, int timeoutMinute, bool printIt);
};

class PirHandler {        //"pre-declaration"
  public:
    // Constructor
    PirHandler(int pin);

    // Initialization done after construction, to permit static instances
    void init();

    // suppress, to be called by RF and Pin devices
    void suppressPirActivity();
    // Handler, to be called in the loop 5Hz
    void handlePirActivity();
    // Handler, to be called in the loop 1Hz
    void handlePirActions();

  private:
    const int pin;                     // pin to which pir is connected
    bool pirActivityFlag;
    int activitySamples;
    bool humanPresentFlag;
};


//#endif
//end - This could be in a header file




//start - This could be in a .cpp file
//#include "Arduino.h"
//#include "HomeControl.h"

//member definitions

//////////////////////////////////////////////////////////////////////////////

// Instantiate objects
ClockHandler Myclock(0); //first EEprom address
EEpromList_Handler timeoutList(100, 260); //first/last EEprom address
PinDeviceHandler ResetSuppress(12, sizeof(unsigned long) * 1, 100, 101);
PinDeviceHandler Relay0(4, sizeof(unsigned long) * 2, 800, 801);
PinDeviceHandler Relay1(5, sizeof(unsigned long) * 3, 810, 811);
PinDeviceHandler Relay2(6, sizeof(unsigned long) * 4, 820, 821);
PinDeviceHandler Relay3(7, sizeof(unsigned long) * 5, 830, 831);
PinDeviceHandler Relay4(8, sizeof(unsigned long) * 6, 840, 841);
PinDeviceHandler Relay5(9, sizeof(unsigned long) * 7, 850, 851);
PinDeviceHandler Relay6(10, sizeof(unsigned long) * 8, 860, 861);
PinDeviceHandler Relay7(11, sizeof(unsigned long) * 9, 870, 871);
RFHandler MyRFDevices(2); //pin numbr
IR_Handler MyIRDevices(3);
PirHandler pir1(A0);
//pin 13 free
//a0..a7 free



ClockHandler::ClockHandler(int EEAddress)    //constructor definition
  :    EEpromAddress(EEAddress)
{
}

void ClockHandler::init()
{
  //Get the time data from the EEPROM at position 'EEpromAddress'
  unsigned long pctime;
  EEPROM.get(EEpromAddress, pctime);
  if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
    setTime(pctime); // Sync Arduino clock to the time received on the serial port
    Serial.println("Arduino clock set from Eeprom");
  }
}

unsigned long ClockHandler::handle()
{
  unsigned long event = 0;

  event = 0;
  //every 30 min write to EEPROM to let clock survive resets by PC
  if ( ( (minute() == 0) or (minute() == 30) ) and (second() < 2) )    //only close to beginning of these minutes
  {
    pctime = now();
    //Put the float data from the EEPROM at position 'eeAddress'
    EEPROM.put(EEpromAddress, pctime);
    Serial.println("Timeinfo written to EEPROM.");
    delay(1000);
  }

  //check time, see if RF switch control is planned
  /*    times are calculated:
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
  if (!bovenViaPirDetected)
  {
    timeOn = TIME_HOUR_ON + int(6.5 - abs(month() - 6.5));
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
  }
  if (event == 0 )
  {
    EEpromList_Handler ReadtimeoutList(100, 260); //first/last EEprom address
    event = ReadtimeoutList.readCommand();
    /*
      if (event > 0 )
      {
      Serial.print(" do ");
      Serial.print(event);
      }
    */
  }
  return event;
}

void ClockHandler::set(unsigned long p_time)
{
  if ( p_time >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
    setTime(p_time); // Sync Arduino clock to the time received on the serial port
    //Put the float data from the EEPROM at position 'eeAddress'
    EEPROM.put(EEpromAddress, p_time);
    Serial.println("Timeinfo from PC set and written to EEPROM.");
  }
}


PinDeviceHandler::PinDeviceHandler(int o_pin, int EEAddress, unsigned long codeOn, unsigned long codeOff)    //constructor definition
  :    outputPin(o_pin), EEpromAddress(EEAddress), codeForOn(codeOn), codeForOff(codeOff)
{
}

void PinDeviceHandler::init()
{

  //get state from EEprom, set pin
  unsigned long command = 0;
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
      pinMode(outputPin, INPUT);    //pulling the resetpin down is not a good idea....
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
    pir1.suppressPirActivity();
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
      digitalWrite(outputPin, 0); // set the ResetSuppressPin OFF
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
    pir1.suppressPirActivity();

    if (command == 830)
    {
      bureauLampOnFlag = true;
    }
    if (command == 831)
    {
      bureauLampOnFlag = false;
    }
  }
}



RFHandler::RFHandler(int o_pin)    //constructor definition
  :    outputPin(o_pin)
{
}

void RFHandler::init()
{
  //RcSwitch
  //Transmitter is connected to Arduino Pin #2
  mySwitch.enableTransmit(outputPin);

}

void RFHandler::sendCommand(unsigned long rfControlCode)
{
  //set RF pulselength and send if needed
  if ( (rfControlCode > 881) && (rfControlCode < 1482313385) )
  {
    if ( (rfControlCode >= 4478259 ) && (rfControlCode <= 4478732 ) )
    {
      mySwitch.setPulseLength(177);    //Dynamic energy tx
    }
    else if ( (rfControlCode >= 16404 ) && (rfControlCode <= 16405 ) )
    {
      mySwitch.setPulseLength(360); //Kika
    }
    else
    {
      mySwitch.setPulseLength(324);    //Elro
      //Serial.print("Pl 324. ");
    }
    Serial.print("Sending RF command: ");
    Serial.print(rfControlCode);
    mySwitch.send(rfControlCode, 24);
    Serial.println("");

    pir1.suppressPirActivity();

    //keep tabs on some devices
    if (rfControlCode == SLAAPK_BEELDSCH_AAN)
    {
      pcMonitorsOnFlag = true;
      rfControlCode = SLAAPK_TV_AAN;         //causes a built-in 3s delay. run after SLAAPK_BEELDSCH_AAN
      timeoutList.writeCommand(rfControlCode, -1, -1); // meaning "do immediately"
    }
    if (rfControlCode == SLAAPK_BEELDSCH_UIT)
    {
      pcMonitorsOnFlag = false;
      tvOnFlag = false;
    }
    if (rfControlCode == SLAAPK_KACHEL_AAN)
    {
      kachelOnFlag = true;
    }
    if (rfControlCode == SLAAPK_KACHEL_UIT)
    {
      kachelOnFlag = false;
    }
  }
}

IR_Handler::IR_Handler(int o_pin)    //constructor definition
  :    outputPin(o_pin)
{
}

void IR_Handler::sendCommand(unsigned long irControlCode)
{
  //send IR if needed
  if ( (irControlCode == 200) )
  {
    delay(5000);        //wait for tv to startup after power on
    for (int i = 0; i < 3; i++) {
      irsend.sendNEC(0x10EFA05F, 32); // Telefunken TV power code
      delay(40);
    }
    tvOnFlag = true;
    Serial.print("Written Power button to IR \n");
  }
}

EEpromList_Handler::EEpromList_Handler(int firstAddress, int lastAddress)    //constructor definition
  :    firstAddr(firstAddress), lastAddr(lastAddress)
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
  timeoutListAddress = seekCommand(controlCodeW, 0, 0, false);     //find a controlcode

  // if not found, create a new item in first free slot found
  if (timeoutListAddress == 0)
  {
    //Serial.print("write Item ");
    //Serial.print(controlCodeW);
    //Serial.println(" not found. ");
    //Serial.println(timeoutListAddress);
    timeoutListAddress = seekCommand(0, 0, 0, false);     //find a controlcode
  }
  if (timeoutListAddress > 0)
  {
    //Serial.print("write addr ");
    //Serial.print(timeoutListAddress);
    //Serial.println(" found. ");
  }

  //update or add slot
  if (timeoutListAddress > 0)
  {
    EEPROM.put(timeoutListAddress, controlCodeW);
    EEPROM.put(timeoutListAddress + sizeof(unsigned long), timeoutHour);
    EEPROM.put(timeoutListAddress + sizeof(unsigned long) + sizeof(int), timeoutMinute);
    Serial.print("Item ");
    Serial.print(controlCodeW);
    Serial.print(" written to timeout list on address ");
    Serial.println(timeoutListAddress);
    //listCommands();
  }
}


unsigned long EEpromList_Handler::readCommand()            //return event
{

  unsigned long readEvent = 0;
  int timeoutListAddress = 0;


  readEvent = 0;
  //check list
  timeoutListAddress = seekCommand(999999, -1, -1, false);     //find a controlcode labelled: "do immediately"
  if (timeoutListAddress == 0)
  {
    timeoutListAddress = seekCommand(999999, hour(), minute(), false);     //find a controlcode it is time for
  }
  if (timeoutListAddress > 0)
  {
    EEPROM.get(timeoutListAddress, readEvent);
    //Now erase the item
    const unsigned long fourBytes = 0;
    const int twoBytes = 0;
    EEPROM.put(timeoutListAddress, fourBytes);
    EEPROM.put(timeoutListAddress + sizeof(unsigned long), twoBytes);
    EEPROM.put(timeoutListAddress + sizeof(unsigned long) + sizeof(int), twoBytes);
  }
  /*
    if (readEvent > 0 )
    {
    Serial.print("Found slot : ");
    Serial.print(timeoutListAddress);
    Serial.print("Found action : ");
    Serial.println (readEvent);
    }
  */
  return readEvent;
}


void EEpromList_Handler::listCommands(void)
{
  //dump list
  int dummy = seekCommand(999999, 99, 99, true);     //random numbers, just print
}


int EEpromList_Handler::seekCommand(unsigned long timeoutCommand, int timeoutHour, int timeoutMinute, bool printIt)
{

  //dump list
  int slot = 0;
  int twoBytes = 0;
  int timeoutListAddress = 0;
  int timeoutListItemHour = 0;
  int timeoutListItemMinute = 0;
  unsigned long timeoutListItemCommand = 0;
  // find address of a specific command    -or-     command, 0,0, false
  // find first free address                                        0, 0,0, false
  // scan for commands with current time                0, >0, >0, false
  // list commands:    printIt true        output not used     0, 0, 0, true

  //check for timeout commands
  //if found, use that EEPprom slot
  //else find first empy line

  slot = 0;
  for (timeoutListAddress = firstAddr; timeoutListAddress < lastAddr; timeoutListAddress += 8)
  {
    //int EEpromAddress=timeoutListAddress;
    EEPROM.get(timeoutListAddress, timeoutListItemCommand);
    EEPROM.get(timeoutListAddress + sizeof(unsigned long), timeoutListItemHour);
    EEPROM.get(timeoutListAddress + sizeof(unsigned long) + sizeof(int), timeoutListItemMinute);

    if ( (timeoutHour == 0) && (timeoutMinute == 0) && (!printIt) && (slot == 0) ) //first address not used
    {
      if (timeoutListItemCommand == timeoutCommand ) //first item that matches timeoutCommand
      {
        slot = timeoutListAddress;
      }
    }

    if ( (timeoutCommand == 999999) && (!printIt) && (slot == 0) ) // scan for commands (999999=unknown) with current time
    {
      //Serial.print(timeoutListAddress);
      if ( ( timeoutListItemHour == timeoutHour ) && ( timeoutListItemMinute == timeoutMinute ) && (timeoutListItemCommand > 0) )     //first real item that matches times
      {
        slot = timeoutListAddress;
        /*
          if ( timeoutListItemCommand > 0 )
          {
          Serial.print("Found to do: ");
          Serial.print(timeoutMinute);
          Serial.print(" ");
          Serial.print(timeoutListItemMinute );
          Serial.print(" ");
          Serial.print(timeoutListItemCommand);
          Serial.print(" ");
          }
        */
      }
    }

    if (printIt && timeoutListItemCommand > 0)
    {
      Serial.print(timeoutListAddress);
      Serial.print(" ");
      Serial.print(timeoutListItemCommand);
      Serial.print(" ");
      EEPROM.get(timeoutListAddress + sizeof(unsigned long), twoBytes);
      Serial.print(twoBytes);
      Serial.print(":");
      EEPROM.get(timeoutListAddress + sizeof(unsigned long) + sizeof(int), twoBytes);
      Serial.println(twoBytes);

      /*
        ///Now erase the item     - on time only
        const unsigned long fourBytes=0;
        const int twoBytes=0;
        EEPROM.put(timeoutListAddress, fourBytes);
        EEPROM.put(timeoutListAddress+sizeof(unsigned long), twoBytes);
        EEPROM.put(timeoutListAddress+sizeof(unsigned long)+sizeof(int), twoBytes);
      */
    }
  }
  return slot;     //first match (command or time)
}

PirHandler::PirHandler(int p)    //constructor definition
  : pin(p)
{
}

void PirHandler::init()
{
  //pinMode(pin, INPUT);
}


void PirHandler::suppressPirActivity()   //prevent PIR detection caused by pin or RF actions (interference)
{
  if (!pirActivityFlag)     //only prevent detection when not already detected
  {
    delay(100); // wait for slow responses if any
    for (int i = 0; i < 60; i++)  //hardware creates ~4 sec Active after a trigger, in this case interference
    {
      delay(100);
      Serial.print("A");
      if (analogRead(pin) < 125)
      {
        break;  //exit as soon PIR is silent again
      }
    }
  }
}

void PirHandler::handlePirActivity()
{
  unsigned long pirControlCode = 0;
  const int waitTime = 60;    //1 minute
  unsigned long timeoutTime;
  int timeoutHour;
  int timeoutMinute;

  /*
    if ((analogRead(pin)>125))
    {
    Serial.print("A");
    }
  */
  //Serial.print(analogRead(pin));
  if ((!humanPresentFlag) && (analogRead(pin) > 125))
  {
    activitySamples++;

    //step 1: Burolamp and Kachel - make shure monitor goes last
    if ((!pirActivityFlag) && (activitySamples == 5))     //5 times ~ 1 sec; counts as activity
    {
      Serial.print(" Activity - pirArmedFlag: ");
      Serial.print(pirArmedFlag);
      Serial.print(" pcMonitorsOnFlag: ");
      Serial.println(pcMonitorsOnFlag);
      pirActivityFlag = true;   //step 1
      bovenViaPirDetected = true;
      //assume pc is on
      if (!humanPresentFlag)
      {
        if ( pirArmedFlag )
        {
          //send on command right away
          if (!bureauLampOnFlag)
          {
          /*
		    //first replan off, present or not
            timeoutTime = now() + (waitTime * 2); //minutes
            timeoutHour = hour(timeoutTime);
            timeoutMinute = minute(timeoutTime);
            //timeoutList.listCommands();
            pirControlCode = SLAAPK_BUROLAMP_UIT;
            timeoutList.writeCommand(pirControlCode, timeoutHour, timeoutMinute);
		  */
            //plan immediately on
            pirControlCode = SLAAPK_BUROLAMP_AAN;
            timeoutList.writeCommand(pirControlCode, -1, -1); // "do immediately"
          }
          if ( !kachelOnFlag )
          {
            //plan immediately on
            pirControlCode = SLAAPK_KACHEL_AAN;
            timeoutList.writeCommand(pirControlCode, -1, -1);  // "do immediately"
          }
        }
      }
    }
    else if (pirActivityFlag && (activitySamples >= 7))     //step2: beeldscherm  after two actions
    {
      if (!humanPresentFlag)
      {
        if ( (!pcMonitorsOnFlag) && pirArmedFlag )
        {
        /*
          //first replan off, present or not
          timeoutTime = now() + (waitTime * 3); //minutes
          timeoutHour = hour(timeoutTime);
          timeoutMinute = minute(timeoutTime);
          pirControlCode = SLAAPK_BEELDSCH_UIT;
        */  

          //plan immediately on
          pirControlCode = SLAAPK_BEELDSCH_AAN;
          timeoutList.writeCommand(pirControlCode, -1, -1);

          timeoutList.listCommands();
        }
        humanPresentFlag = true;  //step 2
      }
    }
  }
}

void PirHandler::handlePirActions()
{
  unsigned long pirControlCode = 0;
  const int waitTime = 60;    //1 minute
  unsigned long timeoutTime;
  int timeoutHour;
  int timeoutMinute;

  if (!pirActivityFlag)
  {
    Serial.println("No activity in last minute");
    humanPresentFlag = false;
  }
  else
  {
    Serial.println("Human activity in last minute");

    if ( pirArmedFlag )
    {
      if ( pcMonitorsOnFlag )
      {
        //reschedule off actions
        timeoutTime = now() + (waitTime * 5); //minutes
        timeoutHour = hour(timeoutTime);
        timeoutMinute = minute(timeoutTime);
        pirControlCode = SLAAPK_BEELDSCH_UIT;
  
        //pirControlCode = SLAAPK_BEELDSCH_AAN;
  
        timeoutList.writeCommand(pirControlCode, timeoutHour, timeoutMinute);
      }
      if ( bureauLampOnFlag )
      {
  
        timeoutTime = now() + (waitTime * 4); //minutes
        timeoutHour = hour(timeoutTime);
        timeoutMinute = minute(timeoutTime);
        //timeoutList.listCommands();
        pirControlCode = SLAAPK_BUROLAMP_UIT;
        timeoutList.writeCommand(pirControlCode, timeoutHour, timeoutMinute);
        //timeoutList.listCommands();
      }
      if ( kachelOnFlag )
      {
        timeoutTime = now() + (waitTime * 20); //minutes
        timeoutHour = hour(timeoutTime);
        timeoutMinute = minute(timeoutTime);
        pirControlCode = SLAAPK_KACHEL_UIT;
        timeoutList.writeCommand(pirControlCode, timeoutHour, timeoutMinute);
      }
    }
    activitySamples = 0;
  }
  pirActivityFlag = false;
}

//end - This could be in a .cpp file




void setup()    {
  //Serial.begin(9600);
  Serial.begin(115200);
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

  // init pirs pins; I suppose it's best to do here
  pir1.init();

  digitalClockDisplay();

  //dump timeout list
  timeoutList.listCommands();
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.println(" ");
  Serial.println(" ");
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

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

// return T, W + C or C command
unsigned long processMessage(void) {

  int waitTime = 0;
  unsigned long controlCode = 0;

  const char *timeHeaderUc = "T";// Header tag for serial time sync message
  const char *timeHeaderLc = "t";// Header tag for serial time sync message
  const char *commandHeaderUc = "C";// Header tag for serial device command message
  const char *commandHeaderLc = "c";// Header tag for serial device command message
  const char *waitHeaderUc = "W";// Header tag for serial wait minutes message
  const char *waitHeaderLc = "w";// Header tag for serial wait minutes message

  char *header;
  *header = Serial.read();

  if ( (*header == *timeHeaderUc) || (*header == *timeHeaderLc) )
  {
    Serial.print("Time received from PC: ");
    controlCode = Serial.parseInt();
    Serial.println(controlCode);
  }
  if ( (*header == *waitHeaderUc) || (*header == *waitHeaderLc) )
  {
    waitTime = Serial.parseInt();
    Serial.print("Wait ");
    Serial.print(waitTime);
    unsigned long timeoutTime = now() + (waitTime * 60); //minutes to seconds
    int timeoutHour = hour(timeoutTime);
    int timeoutMinute = minute(timeoutTime);
    Serial.print(" ;planned ");
    Serial.print(timeoutHour);
    Serial.print(":");
    Serial.print(timeoutMinute);

    //W must be followed by C
    for (int i = 0; i < 3; i++)            //process space(s)
    {
      *header = Serial.read();
      if ( (*header == *commandHeaderUc) || (*header == *commandHeaderLc) )
      {
        Serial.print(", command ");
        controlCode =    Serial.parseInt();
        Serial.println (controlCode);
        Serial.println (" ");
      }
    }
    timeoutList.writeCommand(controlCode, timeoutHour, timeoutMinute);

    controlCode = 0; //no command right now
  }
  else
  {
    if ( (*header == *commandHeaderUc) || (*header == *commandHeaderLc) )
    {
      controlCode = Serial.parseInt();
      if ( (controlCode == SLAAPK_BEELDSCH_AAN) || (controlCode == SLAAPK_BUROLAMP_AAN) )    //serial command enabled PIR
      {
         pirArmedFlag = true;                //serial command means pc is on, monitors will be switched with pir
      }
      if ( (controlCode == SLAAPK_BEELDSCH_UIT) || (controlCode == SLAAPK_BUROLAMP_UIT) )    //lamp off may be last, so do both
      {
        pirArmedFlag = false;                //serial command means pc will shut down soon
      }
    }
  }

  return controlCode;
}


void loop() {
  unsigned long controlCode;

  controlCode = 0;    //start blank every loop

  //display some info periodically
  if (timeStatus() != timeNotSet && (second() == 0)) {
    digitalClockDisplay();
    int timeOn = TIME_HOUR_ON + int(6.5 - abs(month() - 6.5));
    int timeOff = TIME_HOUR_OFF;
    Serial.print("On/Off times: ");
    Serial.print(timeOn);
    Serial.print(" & ");
    Serial.println(timeOff);
    //handle pir result
    pir1.handlePirActions();
    timeoutList.listCommands();
    delay(1000); //limit output
  }

  //check for time from pc or serial control command
  if (controlCode == 0)
  {
    if (Serial.available()) {
      controlCode = processMessage();
    }
  }

  Myclock.set(controlCode);    //set if received
  if (controlCode == 0)
  {
    controlCode = Myclock.handle(); //get timed commands
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
  pir1.handlePirActivity();

  delay(100);
  digitalWrite(13, HIGH);    // turn the LED on (HIGH is the voltage level)
  delay(100);
  digitalWrite(13, LOW);     // turn the LED off
}

