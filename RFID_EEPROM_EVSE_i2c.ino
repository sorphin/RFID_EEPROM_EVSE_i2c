/*
 * Open EVSE RFID Interface Code
 * Portions of this are derived from various research online.
 *
 * Copyright (c) 2016+ Daniel Benedict <sorphin@gmail.com>
 * All included libaries are Copyright (c) their respective authors.
 *
 * This Software Module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.

 * This Software Module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this code; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if defined(ARDUINO) && (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h" // shouldn't need this but arduino sometimes messes up and puts inside an #ifdef
#endif // ARDUINO

#include <SoftSerial.h>
#include <NewEEPROM.h>
#include <TinyPinChange.h>

#define I2C_SLAVE_ADDRESS 0x4 // the 7-bit address (remember to change this when adapting this example)
// Get this from https://github.com/rambo/TinyWire
#include <TinyWireS.h>
// The default buffer size, Can't recall the scope of defines right now
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

//External Equipment
#define ENABLE 1               //Pin connected to the Enable pin of RFID
//#define LOCK 3                  //Pin that is connected to NPN transistor that energizes lock

//LED Indicators
//#define OPEN_LIGHT 1              //LED indicator. Indicates the correct key was entered
//#define DENIED_LIGHT 2           //LED indicator. Indicates the wrong key was entered
//#define DETECT_LIGHT 0 //LED indicator. Indicates Programming mode is activated

char rfid_data[10];
char rfid_in[10];
String rfid_master = "";
String comparedkey;
int i, a, b, tmp, command, authorized = 'W';
bool programmingmode = 0, charging = 0;

SoftSerial RFID(3, 255);

void setup()
{
//    Serial.begin(38400);
  RFID.begin(2400);
  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);

//  pinMode(OPEN_LIGHT, OUTPUT);
//  pinMode(DENIED_LIGHT, OUTPUT);
  pinMode(ENABLE, OUTPUT);
//  digitalWrite(OPEN_LIGHT, LOW);
//  digitalWrite(DENIED_LIGHT, LOW);


tws_delay(1500);
}

void loop()
{
  ActiveMode();
  TinyWireS_stop_check();
}

void RegisterCard()
{
//    Serial.print("Registering Card: ");
//    Serial.println(rfid_data);
  // Check if it's in the database
  a = EEPROM.read(0);
  if (a == 0)
  {
    EEPROM.write(0,1);
    for (i = 1; i < 11; i++)
    {
      EEPROM.write(i,rfid_data[i-1]);
    }
//        Serial.println("Card Registered in Database Successfully");
//    digitalWrite(DENIED_LIGHT, LOW);
//    digitalWrite(OPEN_LIGHT, HIGH);
    authorized = 'A';
    programmingmode = 0;
    // REMOVE START
    for (i = 0; i < 11; i++)
    {
      rfid_data[i] = '0';
    }
    rfid_data[11] = '\0';
    // REMOVE END
    tws_delay(2000);
//    digitalWrite(OPEN_LIGHT, LOW);
    authorized = 'W';
    return;
  }
  if (a == 4)
  {
//    digitalWrite(OPEN_LIGHT, LOW);
//    digitalWrite(DENIED_LIGHT, HIGH); // HIGH?
    authorized = 'D';
    tws_delay(2000);
//    digitalWrite(DENIED_LIGHT, LOW);
//        Serial.println("Programming Mode Disabled\n");
    programmingmode = 0;
    authorized = 'W';
    return;
  }

  for (i = 0; i < a; i++)
  {
    for (b = 1; b < 11; b++)
    {
      rfid_in[b-1] = EEPROM.read((i*10)+b);
    }
    rfid_data[10] = '\0';
    b = 1;
    for (int c = 0; c < 10; c++)
    {
      if (rfid_data[c] == rfid_in[c])
      {
        b++;
      }
    }
    if (b == 10)
    {
//      Serial.println("Card Already Exists in Database");
//      digitalWrite(DENIED_LIGHT, HIGH);
//      digitalWrite(OPEN_LIGHT, LOW);
//      Serial.println("Programming Mode Disabled\n");
      authorized = 'D';
      tws_delay(2000);
//      digitalWrite(DENIED_LIGHT, LOW);
      programmingmode = 0;
      authorized = 'W';
      return;
    }
  }
  // Add to database
  tmp = 0;
  for (int d = (a*10)+1; d < (a*10)+11; d++)
  {
    EEPROM.write(d,rfid_data[tmp]);
    tmp++;
  }
  EEPROM.write(0,a+1);
//    Serial.println("Card Registered in Database Successfully");
//    digitalWrite(DENIED_LIGHT, LOW);
//    digitalWrite(OPEN_LIGHT, HIGH);
//    Serial.println("Programming Mode Disabled\n");
  authorized = 'A';
  rfid_data[0] = '\0';
  rfid_in[0] = '\0';
  RFID.flush();
  tws_delay(2000);
//  digitalWrite(OPEN_LIGHT, LOW);
  programmingmode = 0;
  authorized = 'W';
  return;
}


void ListDatabase()
{
  a = EEPROM.read(0);
  if (a == 0)
  {
//        Serial.println("Database Empty");
    return;
  }

  for (i = 0; i < a; i++)
  {
//        Serial.print("Card ");
//        Serial.print(i);
//        Serial.print(": ");
    for (b = 1; b < 11; b++)
    {
//            Serial.write(EEPROM.read((i*10)+b));
    }
//        Serial.println();
  }
//    Serial.println();
//  digitalWrite(OPEN_LIGHT, LOW);
//  digitalWrite(DENIED_LIGHT, LOW);
  authorized = 'P';
  return;
}


void ActiveMode()
{
  // Read a card
  for (int rs = 0; rs < 11; rs++)
  {
    rfid_data[rs] = '0';
  }
  rfid_data[10] = '\0';
  digitalWrite(ENABLE, LOW);
  tws_delay(500);
  do
  {
    tws_delay(200);
  } 
  while (RFID.available() <= 0);
  digitalWrite(ENABLE, HIGH);
  tws_delay(500);
  rfid_data[0] = RFID.read();
  if (int(rfid_data[0]) == 10)
  {
    for (i = 0; i < 10; i++)
    {
      rfid_data[i] = RFID.read();
    }
    rfid_data[10] = '\0';
  }

  RFID.flush();

  // Check if it's in the database

  comparedkey = rfid_data;
  comparedkey = comparedkey.substring(0,10);

  if (comparedkey == rfid_master) {
    if (charging == 1) {
      programmingmode = 0;
      authorized = 'E';
      return;
    } else {
      if (programmingmode == 1) {
//      ListDatabase();
        programmingmode = 0;
        authorized = 'W';
//Serial.println("Programming Mode Disabled\n");
        return;
      }
      else {
//            Serial.println("Programming Mode Enabled\n");
//      digitalWrite(OPEN_LIGHT, HIGH);
//      digitalWrite(DENIED_LIGHT, HIGH);
        authorized = 'P';
        tws_delay(1000);
        digitalWrite(ENABLE, HIGH);
        programmingmode = 1;
        RFID.flush();
        return;
      }
    }
  }

  a = EEPROM.read(0);
  if (a == 0 && programmingmode == 0)
  {
//    digitalWrite(DENIED_LIGHT, HIGH);
    digitalWrite(ENABLE, LOW);
//    Serial.println("Access Denied");
    authorized = 'D';
    tws_delay(2000);
    RFID.flush();
    digitalWrite(ENABLE, HIGH);
//    digitalWrite(DENIED_LIGHT, LOW);
    authorized = 'W';
    return;
  }

  for (i = 0; i < a; i++)
  {
    for (b = 1; b < 11; b++)
    {
      rfid_in[b-1] = EEPROM.read((i*10)+b);
    }
    rfid_data[10] = '\0';
    b = 1;
    for (int c = 0; c < 10; c++)
    {
      if (rfid_data[c] == rfid_in[c])
      {
        b++;
      }
    }
    if (b == 10)
    {
      if (programmingmode == 1) {
        RemoveCard();
      } 
      else {
//        digitalWrite(OPEN_LIGHT, HIGH);
//        Serial.println("Access Approved");
        authorized = 'A';
        tws_delay(2000);
        RFID.flush();
        digitalWrite(ENABLE, LOW);
        tws_delay(500);
        digitalWrite(ENABLE, HIGH);
        authorized = 'W';
//        digitalWrite(OPEN_LIGHT, LOW);
//        digitalWrite(DENIED_LIGHT, LOW);
      }
      return;
    }
  }
  if (programmingmode == 1) {
    RegisterCard();
  } 
  else {
//    digitalWrite(DENIED_LIGHT, HIGH);
//    Serial.println("Access Denied");
    authorized = 'D';
    tws_delay(2000);
    RFID.flush();
    digitalWrite(ENABLE, LOW);
    tws_delay(500);
    digitalWrite(ENABLE, HIGH);
//    digitalWrite(DENIED_LIGHT, LOW);
//    digitalWrite(OPEN_LIGHT, LOW);
      authorized = 'W';
  }
  return;
}

void RemoveCard()
{
//    Serial.print("Removing Card: ");
//    Serial.println(rfid_data);
  // Check if it's in the database
  a = EEPROM.read(0);
  if (a == 0)
  {
//    Serial.println("Not in Database");
    programmingmode = 0;
//    digitalWrite(DENIED_LIGHT, LOW);
//    digitalWrite(OPEN_LIGHT, LOW);
//    Serial.println("Programming Mode Disabled");
    authorized = 'D';
    tws_delay(2000);
    authorized = 'W';
    return;
  }

  for (i = 0; i < a; i++)
  {
    for (b = 1; b < 11; b++)
    {
      rfid_in[b-1] = EEPROM.read((i*10)+b);
    }
    rfid_data[10] = '\0';
    b = 1;
    for (int c = 0; c < 10; c++)
    {
      if (rfid_data[c] == rfid_in[c])
      {
        b++;
      }
    }
    if (b == 10)
    {
      int rmstrt = ((i*10)+b)-9;
      // Last in database?
      if ((a*10) == (rmstrt+9))
      {
        EEPROM.write(0,a-1);
//        Serial.println("Card Removed Successfully!");
//        digitalWrite(OPEN_LIGHT, LOW);
//        digitalWrite(DENIED_LIGHT, HIGH);
        programmingmode = 0;
        authorized = 'D';
//        Serial.println("Programming Mode Disabled\n");
        tws_delay(2000);
//        digitalWrite(DENIED_LIGHT, LOW);
        authorized = 'W';
        return;
      }
      for (tmp = rmstrt+10; tmp < (a*10)+1; tmp++)
      {
        char dta = EEPROM.read(tmp);
        tws_delay(100);
        EEPROM.write(rmstrt,dta);
        rmstrt++;
      }
      tws_delay(100);
      EEPROM.write(0,a-1);
//      Serial.println("Card Removed Successfully!");
//      digitalWrite(OPEN_LIGHT, LOW);
//      digitalWrite(DENIED_LIGHT, HIGH);
      programmingmode = 0;
      authorized = 'D';
//      Serial.println("Programming Mode Disabled\n");
      tws_delay(2000);
//      digitalWrite(DENIED_LIGHT, LOW);
      authorized = 'W';
      return;
    }
  }
//  Serial.println("Card Not in Database");
  programmingmode = 0;
  authorized = 'D';
//  digitalWrite(DENIED_LIGHT, HIGH);
//  digitalWrite(OPEN_LIGHT, LOW);
//    Serial.println("Programming Mode Disabled\n");
  rfid_data[0] = '\0';
  rfid_in[0] = '\0';
  RFID.flush();
  tws_delay(2000);
//  digitalWrite(DENIED_LIGHT, LOW);
  authorized = 'W';
  return;
}

void requestEvent()
{  
  switch(command) {
    case 'a':
    case 'n':
      charging = 0;
      authorized = 'W';
      break;
    case 'c':
      charging = 1;
      break;
    case 's':
      TinyWireS.send(authorized);
      break;
    default:
    ;
//      TinyWireS.send(0x00);
  }
    
}

void receiveEvent(uint8_t howMany)
{
  command = TinyWireS.receive();
}

