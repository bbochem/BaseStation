/**********************************************************************

Sensor.cpp
COPYRIGHT (c) 2013-2015 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino Uno 

**********************************************************************/
/**********************************************************************

DCC++ BASE STATION supports Sensor inputs that can be connected to any Aruidno Pin 
not in use by this program.  Sensors can be of any type (infrared, magentic, mechanical...).
The only requirement is that when "activated" the Sensor must force the specified Arduino
Pin LOW (i.e. to ground), and when not activated, this Pin should remain HIGH (e.g. 5V),
or be allowed to float HIGH if use of the Arduino Pin's internal pull-up resistor is specified.

To ensure proper voltage levels, some part of the Sensor circuitry
MUST be tied back to the same ground as used by the Arduino.

The Sensor code below utilizes exponential smoothing to "de-bounce" spikes generated by
mechanical switches and transistors.  This avoids the need to create smoothing circuitry
for each sensor.  You may need to change these parameters through trial and error for your specific sensors.

UPDATE THIS SECTION !!!!!!!!!!!

All Sensors should be defined in a single global array declared in DCCpp_Uno.ino using the format:

  Sensor(ID, PIN, PULLUP)

  ID: a unique integer ID (0-32767) for this sensor
  PIN: the Arudino Pin used to read the sensor
  PULLUP: HIGH (1) to enable this Pin's internal pull-up resistor, LOW (0) to disable the internal pull-up resistor 

All Sensors defined in the global array are repeatedly and sequentially checked within the main loop of this sketch.
If a Sensor Pin is found to have transitioned from a HIGH state to a LOW state, the following serial message is generated:

  <Q ID>

  where ID is the ID of the Sensor that was triggered

No message is generated when a Sensor Pin transitions back to a HIGH state from a LOW state. 

**********************************************************************/

#include "DCCpp_Uno.h"
#include "Sensor.h"
#include "EEStore.h"
#include <EEPROM.h>

///////////////////////////////////////////////////////////////////////////////
  
void Sensor::check(){    
  Sensor *tt;

  for(tt=firstSensor;tt!=NULL;tt=tt->nextSensor){
    tt->signal=tt->signal*(1.0-SENSOR_DECAY)+digitalRead(tt->data.pin)*SENSOR_DECAY;
    
    if(!tt->active && tt->signal<0.5){
      tt->active=true;
      Serial.print("<Q");
      Serial.print(tt->data.snum);
      Serial.print(">");
    } else if(tt->active && tt->signal>0.99){
      tt->active=false;
    }
  } // loop over all sensors
    
} // Sensor::check

///////////////////////////////////////////////////////////////////////////////

Sensor *Sensor::create(int snum, int pin, int pullUp, int v){
  Sensor *tt;
  
  if(firstSensor==NULL){
    firstSensor=(Sensor *)calloc(1,sizeof(Sensor));
    tt=firstSensor;
  } else if((tt=get(snum))==NULL){
    tt=firstSensor;
    while(tt->nextSensor!=NULL)
      tt=tt->nextSensor;
    tt->nextSensor=(Sensor *)calloc(1,sizeof(Sensor));
    tt=tt->nextSensor;
  }

  if(tt==NULL){       // problem allocating memory
    if(v==1)
      Serial.print("<X>");
    return(tt);
  }
  
  tt->data.snum=snum;
  tt->data.pin=pin;
  tt->data.pullUp=(pullUp==0?LOW:HIGH);
  tt->active=false;
  tt->signal=1;
  pinMode(pin,INPUT);         // set mode to input
  digitalWrite(pin,pullUp);   // don't use Arduino's internal pull-up resistors for external infrared sensors --- each sensor must have its own 1K external pull-up resistor

  if(v==1)
    Serial.print("<O>");
  return(tt);
  
}

///////////////////////////////////////////////////////////////////////////////

Sensor* Sensor::get(int n){
  Sensor *tt;
  for(tt=firstSensor;tt!=NULL && tt->data.snum!=n;tt=tt->nextSensor);
  return(tt); 
}
///////////////////////////////////////////////////////////////////////////////

void Sensor::remove(int n){
  Sensor *tt,*pp;
  
  for(tt=firstSensor;tt!=NULL && tt->data.snum!=n;pp=tt,tt=tt->nextSensor);

  if(tt==NULL){
    Serial.print("<X>");
    return;
  }
  
  if(tt==firstSensor)
    firstSensor=tt->nextSensor;
  else
    pp->nextSensor=tt->nextSensor;

  free(tt);

  Serial.print("<O>");
}

///////////////////////////////////////////////////////////////////////////////

void Sensor::show(){
  Sensor *tt;

  if(firstSensor==NULL){
    Serial.print("<X>");
    return;
  }
    
  for(tt=firstSensor;tt!=NULL;tt=tt->nextSensor){
    Serial.print("<Q");
    Serial.print(tt->data.snum);
    Serial.print(" ");
    Serial.print(tt->data.pin);
    Serial.print(" ");
    Serial.print(tt->data.pullUp);
    Serial.print(">");
  }
}

///////////////////////////////////////////////////////////////////////////////

void Sensor::parse(char *c){
  int n,s,m;
  Sensor *t;
  
  switch(sscanf(c,"%d %d %d",&n,&s,&m)){
    
    case 3:                     // argument is string with id number of sensor followed by a pin number and pullUp indicator (0=LOW/1=HIGH)
      create(n,s,m,1);
    break;

    case 1:                     // argument is a string with id number only
      remove(n);
    break;
    
    case -1:                    // no arguments
      show();
    break;

    case 2:                     // invalid number of arguments
      Serial.print("<X>");
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////

void Sensor::load(){
  struct SensorData data;
  Sensor *tt;

  for(int i=0;i<EEStore::eeStore->data.nSensors;i++){
    EEPROM.get(EEStore::pointer(),data);  
    tt=create(data.snum,data.pin,data.pullUp);
    EEStore::advance(sizeof(tt->data));
  }  
}

///////////////////////////////////////////////////////////////////////////////

void Sensor::store(){
  Sensor *tt;
  
  tt=firstSensor;
  EEStore::eeStore->data.nSensors=0;
  
  while(tt!=NULL){
    EEPROM.put(EEStore::pointer(),tt->data);
    EEStore::advance(sizeof(tt->data));
    tt=tt->nextSensor;
    EEStore::eeStore->data.nSensors++;
  }  
}

///////////////////////////////////////////////////////////////////////////////

Sensor *Sensor::firstSensor=NULL;

