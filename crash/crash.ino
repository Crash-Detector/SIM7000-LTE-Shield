#include "Adafruit_FONA.h"
// For botletics SIM7000 shield
#define FONA_PWRKEY 6
#define FONA_RST 7
//#define FONA_DTR 8 // Connect with solder jumper
//#define FONA_RI 9 // Need to enable via AT commands
#define FONA_TX 10 // Microcontroller RX
#define FONA_RX 11 // Microcontroller TX
//#define T_ALERT 12 // Connect with solder jumper

#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

void printMenu(void) {
  Serial.println(F("-------------------------------------"));
  // General
  Serial.println(F("[?] Print this menu"));
  Serial.println(F("[a] Read the ADC; 2.8V max for SIM800/808, 0V-VBAT for SIM7000 shield"));
  Serial.println(F("[b] Read supply voltage")); // Will also give battery % charged for most modules
  Serial.println(F("[n] Get network status"));
  Serial.println(F("[s] Send SMS"));
  Serial.println(F("[O] Turn GPS on (SIM808/5320/7XX0)"));
  Serial.println(F("[o] Turn GPS off (SIM808/5320/7XX0)"));
  Serial.println(F("[L] Query GPS location (SIM808/5320/7XX0)"));
}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}

void setup() {
  //  while (!Serial);

  pinMode(FONA_RST, OUTPUT);
  digitalWrite(FONA_RST, HIGH); // Default state

  pinMode(FONA_PWRKEY, OUTPUT);

  fona.powerOn(FONA_PWRKEY); // Power on the module

  Serial.begin(9600);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take several seconds)"));

  // Software serial:
  fonaSS.begin(115200); // Default SIM7000 shield baud rate

  Serial.println(F("Configuring to 9600 baud"));
  fonaSS.println("AT+IPR=9600"); // Set baud rate
  delay(100); // Short pause to let the command run
  fonaSS.begin(9600);
  if (! fona.begin(fonaSS)) {
    Serial.println(F("Couldn't find FONA"));
    while (1); // Don't proceed if it couldn't find the device
  }
 
  // Set modem to full functionality
  fona.setFunctionality(1); // AT+CFUN=1

  fona.setNetworkSettings(F("hologram")); // For Hologram SIM card


  printMenu();
}

void loop() {
Serial.print(F("FONA> "));
  while (! Serial.available() ) {
    if (fona.available()) {
      Serial.write(fona.read());
    }
  }

char command = Serial.read();
Serial.println(command);

switch (command) {
case '?': {
  printMenu();
  break;
}

case 'a': {
// read the ADC
uint16_t adc;
if (! fona.getADCVoltage(&adc)) {
  Serial.println(F("Failed to read ADC"));
} else {
  Serial.print(F("ADC = ")); Serial.print(adc); Serial.println(F(" mV"));
}
break;
}

case 'b': {
// read the battery voltage and percentage
uint16_t vbat;
if (! fona.getBattVoltage(&vbat)) {
  Serial.println(F("Failed to read Batt"));
} else {
  Serial.print(F("VBat = ")); Serial.print(vbat); Serial.println(F(" mV"));
}

if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
  }
break;
}

case 'n': {
  // read the network/cellular status
  uint8_t n = fona.getNetworkStatus();
  Serial.print(F("Network status "));
  Serial.print(n);
  Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));
  break;
}

case 's': {
  // send an SMS!
  char sendto[21], message[141];
  flushSerial();
  Serial.print(F("Send to #"));
  readline(sendto, 20, 560);
  Serial.println(sendto);
  Serial.print(F("Type out one-line message (140 char): "));
  readline(message, 140, 560);
  Serial.println(message);
  if (!fona.sendSMS(sendto, message)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }

  break;
}

case 'o': {
    // turn GPS off
    if (!fona.enableGPS(false))
      Serial.println(F("Failed to turn off"));
    break;
  }
  
case 'O': {
    // turn GPS on
    Serial.println(F("GPS on"));
    if (!fona.enableGPS(true))
      Serial.println(F("Failed to turn on"));
    break;
  }
  

case 'L': {

    float latitude, longitude, speed_kph, heading, altitude, second;
    uint16_t year;
    uint8_t month, day, hour, minute;

    // Use the top line if you want to parse UTC time data as well, the line below it if you don't care
//        if (fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
    if (fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude)) { // Use this line instead if you don't want UTC time
      Serial.println(F("---------------------"));
      Serial.print(F("Latitude: ")); Serial.println(altitude, 6);
      Serial.print(F("Longitude: ")); Serial.println(longitude, 6);
      
    }

    break;
  }

  default: {
        Serial.println(F("Unknown command"));
        printMenu();
        break;
      }
      

  }
  flushSerial();
  while (fona.available()) {
    Serial.write(fona.read());
  }
}

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while (Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;

        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }

    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}
