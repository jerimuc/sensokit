/* sensokit MainBoard
 * 
 * v1.0 - Aug 25
 * 
 * Devices & Parts: 
 * - Arduino MKR NB 1500 (https://www.arduino.cc/en/Guide/MKRNB1500, https://store.arduino.cc/products/arduino-mkr-nb-1500?_gl=1%2Ajmo3e2%2A_ga%2AMTgyMjQxNTg5MC4xNjM3MTQ1NTIw%2A_ga_NEXN8H46L5%2AMTYzNzc0NjM2My4yLjEuMTYzNzc0NjYwOS4w)
 * - Adafruit Mini GPS (https://www.adafruit.com/product/4415)
 * - Adafruit TCA9548A I2C Multiplexer (https://www.adafruit.com/product/2717)
 * - Adafruit Mini Illuminated Momentary Pushbutton - Blue Power Symbol (https://www.adafruit.com/product/3105)
 * - Adafruit NeoPixel Mini PCB (https://www.adafruit.com/product/1612)

 * Sensors: 
 * - Adafruit SCD41 CO2, Temp, Humidity. (I2C Address: 0x62); https://learn.adafruit.com/adafruit-scd-40-and-scd-41
 * - PCB Artists I2C Decibel Sound Level Meter Module (I2C Address: 0x48); https://pcbartists.com/product/i2c-decibel-sound-level-meter-module/?srsltid=AfmBOoobBLvN6rv1Y8pd216K474floij3mWsiDT7VdPprxC0iyXSB69u
      https://pcbartists.com/product-documentation/arduino-decibel-meter/#google_vignette
      https://pcbartists.com/product-documentation/i2c-decibel-meter-programming-manual/
 * - Adafruit ENS 160 Gas Sensor (I2C Address: 0x53); https://learn.adafruit.com/adafruit-ens160-mox-gas-sensor/overview
 * - DF Robot SEN0321 Ozone Sensor (I2C Address: 0x73); https://wiki.dfrobot.com/Gravity_IIC_Ozone_Sensor_%280-10ppm%29%20SKU_SEN0321
 *
 * 
 * Infos & Libraries: 
 * - Arduino MKR NB Library:https://www.arduino.cc/en/Reference/MKRNB
 * - Arduino MKR NB Features: https://docs.arduino.cc/hardware/mkr-nb-1500
 * - Sparkfun_I2C_GPS_Arduino_Library: For using TinyGPS++ Library with I2C
 * - Tiny GPS++ Library: http://arduiniana.org/libraries/tinygpsplus/
 * - I2C Multiplexer Info: https://learn.adafruit.com/adafruit-tca9548a-1-to-8-i2c-multiplexer-breakout/overview
 * - Adafruit NeoPixel Uberguide: https://learn.adafruit.com/adafruit-neopixel-uberguide
 * - Adafruit NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel


* Values & Datatypes, Invalid:
  - Noise:  float,      invalid: NaN
  - Ozone:  int16_t,    invalid: -32768
  - AQI:    int16_t,    invalid: -32768
  - TVOC:   int16_t,    invalid: -32768
  - CO2:    int16_t,    invalid: -32768
  - Temp.:  float,      invalid: NaN
  - Hum.:   float,      invalid: NaN
 
 * STATUS & LED:
 * 0 = While Setup/Startup:          YELLOW (255, 225, 0)
   1 = Running & Everything is OK:   GREEN (0, 255, 0)
   2 = Measuring DATA:               GREEN (200, 255, 50). //TODO
   3 = NO GPS Signal/Position/Fix:   BLUE (0, 0, 255)
   4 = NO Cell Network/Connection:   PINK (255, 0, 255)
   5 = Batterry low:                 RED (255, 0, 0)
 *
 */

#include <MKRNB.h>
#include <TinyGPS++.h>
#include <Adafruit_GPS.h>
#include <SparkFun_I2C_GPS_Arduino_Library.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoLowPower.h>
#include <stdio.h>
#include <string.h>
#include "DFRobot_OzoneSensor.h"
#include "ScioSense_ENS160.h"
#include <SensirionI2cScd4x.h>

// The used commands use up to 48 bytes. On some Arduino's the default buffer
// space is not large enough
#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) && (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

// GPS I2C Address
#define GPS_I2C_ADDR 0x10
// TCA9548A Multiplexer I2C Address
#define TCA9548A_ADDR 0x70
// GPS connected to Channel 0 on the Multiplexer
#define TCA_GPS_CHANNEL 0

#define DBM_ADDR 0x48             //Sound level meter I2C address
// Device registers
#define   DBM_REG_DECIBEL 0x0A

#define SCD41_ADDR 0x62
#define ENS160_ADDR 0x53
#define OZONE_ADDR 0x73
#define NO_SENSOR 0x00


#define GPS_WAKE_PIN 4    // Pin connected to PA1010D WAKE pin

#define LED_PIN 6       // Connect NeoPixel DIN to pin 6
#define NUM_pixel 1    //  NeoPixel count


// Define the threshold for low battery (adjust as needed)
#define LOW_BATTERY_THRESHOLD 15            // % - then the LED is red
#define SHUTDOWN_BATTERY_THRESHOLD 3.38      //mV - the device is shutdown below this value


// Function to select a specific channel on the TCA9548A
void selectTCAChannel(uint8_t channel) {
    if (channel > 7) return; // TCA9548A has 8 channels (0-7)
    Wire.beginTransmission(TCA9548A_ADDR);
    Wire.write(1 << channel); // Enable the desired channel
    Wire.endTransmission();
}

// sensor channels (0 if not connected);
byte scd41channel = 0;
byte ozonechannel = 0;
byte soundchannel = 0;
byte ens160channel = 0;

// The TinyGPS++ object
I2CGPS myI2CGPS;  //Hook object to the library
TinyGPSPlus gps;
boolean gpsfix = false;
Adafruit_GPS GPS(&Wire); //for sending the module to standby


//Connection to DATAhub
//NBClient client(false);
//Otherwise the NBClient.connect() method waits until the internet connection gets ready, if you explicitly prohibit this it will wait forever.
NBClient client(false);
GPRS gprs;
NB nbAccess;
boolean connected = false;

// URL, path and port (for example: example.org
char server[] = "aml.media.tuwien.ac.at";
// IMPORTANT: Place correct source Token here: THIS IS sensokit 3
char pathdata[] = "/api/sensordata/";  //Add a correct source token after /api/sensordata/. The source token can be obtained from the aspern.mobil LAB DATAhub Team (http://datahub.mobillab.wien)
char pathstatus[] = "/api/sensorstatus/"; //Add a correct source token after /api/sensordata/. The source token can be obtained from the aspern.mobil LAB DATAhub Team (http://datahub.mobillab.wien)
int port = 80;  // port 80 is the default for HTTP; Ask for the correct port of the DATAhub at the DATAhub Team.



Adafruit_NeoPixel pixel(NUM_pixel, LED_PIN, NEO_GRB + NEO_KHZ800);


//Timing
unsigned long currentMillis = 0;

uint32_t printTimer = millis();  //Just a timer for printing out values

unsigned long sendToDATAhubInterval = 120000;  //2min; Interval for sending data to webservice = Data sampling interval
unsigned long lastSendToDATAhub = 0;

unsigned long collectDataInBackgroundInterval = 4000; //Get data every four seconds to keep the sensors going
unsigned long lastDataInBackground = 0;


const unsigned long dbSampleDuration = 2000; // milliseconds (2 seconds)


static uint32_t STARTUP = pixel.Color(255, 225, 0);
static uint32_t RUNNING = pixel.Color(0, 255, 0);
static uint32_t OPERATING = pixel.Color(200, 255, 50);
static uint32_t GPSERROR = pixel.Color(0, 0, 255);
static uint32_t NETWORKERROR = pixel.Color(255, 0, 255);
static uint32_t LOWBAT = pixel.Color(255, 0, 0);
static uint32_t OFF = pixel.Color(0, 0, 0);
static uint32_t TESTCOLOR = pixel.Color(3, 252, 255);
uint32_t activecolor = STARTUP;
static int FADESTEPS = 20;
static int FADEDELAY = 50;

byte statusinfo;

// Buffer for printing decibel values over UART - Sound level meter
char buf[64];

//Ozone Sensor
DFRobot_OzoneSensor Ozone;

//MOX Sensor
ScioSense_ENS160      ens160(ENS160_ADDR);

//SCD41 Sensor

// macro definitions
// make sure that we use the proper definition of NO_ERROR
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0
SensirionI2cScd4x scd_sensor;
static char scderrorMessage[64];
static int16_t scderror;

//Sensor values
float noise = -9999;
int16_t ozone = -32768;
int16_t aqi = -32768;
int16_t tvoc = - -32768;
int16_t co2 = -32768;
float temperature = -9999;
float humidity = -9999;

// Function to read a register from decibel meter module
uint8_t dbmeter_readreg (uint8_t regaddr)
{
  Wire.beginTransmission (DBM_ADDR);
  Wire.write (regaddr);
  Wire.endTransmission();
  Wire.requestFrom (DBM_ADDR, 1);
  delay (10);
  return Wire.read();
}

// Function to write a register to decibel meter module
uint8_t dbmeter_writereg (uint8_t regaddr, uint8_t regdata)
{
  Wire.beginTransmission (DBM_ADDR);
  Wire.write (regaddr);
  Wire.write (regdata);
  Wire.endTransmission();
  delay (10);
  return dbmeter_readreg (regaddr);
}

//ON/OFF Button
const int buttonPin = 7;  // Pushbutton connected to D2
const int buttonLedPin = 5; // LED inside the pushbutton (Anode to D3, Cathode to GND)

bool isAwake = true;
bool standbynextcycle = false;

void wakeUp() {
  // Interrupt function - does nothing, just wakes up the board
}

void setup() {

  //Setting up basic things
  pinMode(buttonPin, INPUT_PULLUP); // Internal pull-up for button
  pinMode(buttonLedPin, OUTPUT);    // LED inside the button
  
  Serial.begin(115200);
  Serial1.begin(9600);
  
  Wire.begin();

  pixel.begin();  // Initialize NeoPixel

  while( readBatteryVoltage() < SHUTDOWN_BATTERY_THRESHOLD ) {
    fadePixel(OFF, LOWBAT, FADESTEPS, FADEDELAY);
    fadePixel(LOWBAT, OFF, FADESTEPS, FADEDELAY);
    Serial.println("INFO: in while and below shutdown battery threshold: " + String(readBatteryVoltage()));
  }
  
  
  digitalWrite(buttonLedPin, HIGH); // Button LED also ON

  // Interrupt on button press to wake up from sleep
  LowPower.attachInterruptWakeup(buttonPin, wakeUp, FALLING);

  statusinfo = 0;

  fadePixel(OFF, STARTUP, FADESTEPS, FADEDELAY);
  
  connected = false;

  //Connect GPS to I2C
  // Select Channel 0 for GPS
  pinMode(GPS_WAKE_PIN, OUTPUT);
  digitalWrite(GPS_WAKE_PIN, HIGH); // Ensure GPS is ON at startup
  delay(2000);  // Allow time for the GPS to initialize
  Serial.println(digitalRead(GPS_WAKE_PIN) ? "SETUP: GPS WAKE pin is HIGH" : "SETUP: GPS WAKE pin is LOW");


  selectTCAChannel(TCA_GPS_CHANNEL); // Activate Channel 0
  GPS.begin(TCA_GPS_CHANNEL);
  if (myI2CGPS.begin() == false) {
    Serial.println("SETUP: GPS Module failed to respond. Please check wiring.");
    statusinfo = 3;
    fadePixel(activecolor, GPSERROR, FADESTEPS, FADEDELAY);
    while (1)
      ;  //Freeze!
  }

  encodeGPS();

  if(batteryPercentage() <= LOW_BATTERY_THRESHOLD && statusinfo != 3 && statusinfo != 4) {
        statusinfo = 5;
        fadePixel(activecolor, LOWBAT, FADESTEPS, FADEDELAY);
  } 

  if (statusinfo == 0) {
    statusinfo = 1;
    fadePixel(activecolor, RUNNING, FADESTEPS, FADEDELAY);
  } 

}



void loop() {



  if (isAwake) {
      // When button is pressed, go to sleep
      if (digitalRead(buttonPin) == LOW) {
        delay(200); // Debounce
        digitalWrite(buttonLedPin, LOW);  // Turn OFF button LED
        isAwake = false;
        connected = false;
        
        sendStatusMessage();

        fadePixel(activecolor, OFF, FADESTEPS, FADEDELAY);

        digitalWrite(GPS_WAKE_PIN, LOW); // Put GPS to sleep
        Serial.println(digitalRead(GPS_WAKE_PIN) ? "INFO LOOP 379: GPS WAKE pin is HIGH" : "GPS WAKE pin is LOW");

        nbAccess.shutdown();
        
        LowPower.deepSleep(); // Enter sleep mode

        // Wait for button release after wake-up
        while (digitalRead(buttonPin) == LOW);
        delay(200); // Additional debounce
      }


    //collecting Data in Background if Sensors are connected. Sensors need to be kept activated and running if connected. Data is read to some temp variables.

      currentMillis = millis();
      if ((currentMillis - lastDataInBackground >= collectDataInBackgroundInterval)) {

          lastDataInBackground = currentMillis;

          if (ens160channel !=0 && ens160.available()) {
            uint8_t tempaqi = ens160.getAQI();
            uint16_t temptvoc = ens160.getTVOC();
          }

          if(scd41channel !=0) {
            uint16_t tempco2;
            float temptemp;
            float temphum;
            scderror = scd_sensor.measureAndReadSingleShot(tempco2, temptemp, temphum);
          }

          if(ozonechannel !=0) {
            int16_t tempozone = Ozone.readOzoneData(20);
          }

    }

    //Sending to DATAhub
    currentMillis = millis();

    if ((currentMillis - lastSendToDATAhub >= sendToDATAhubInterval)) {

      lastSendToDATAhub = currentMillis;
      
      //Sensor is operating and collecting data
      statusinfo = 2;
      fadePixel(activecolor, OPERATING, FADESTEPS, FADEDELAY);

      //Encode GPS data
      encodeGPS(); 

      //connect the modem to the network
      connectToNetwork();

      //Check the battery status before collecting data
      int batPercent = batteryPercentage();
      float batVoltage = readBatteryVoltage();

      //Sending status message (mobile network connected and enough battery)
      if(batVoltage > SHUTDOWN_BATTERY_THRESHOLD) {
        Serial.println("INFO LOOP 446: Sending standard status message!");
        sendStatusMessage();
      } else if (batVoltage <= SHUTDOWN_BATTERY_THRESHOLD) {       //(mobile network connected and NOT enough battery)
        
        //Send everything to sleep
        isAwake = false;
        connected = false;
        

        Serial.println("INFO LOOP 459: Sending lowbat shutdown status message!");
        sendStatusMessage();
        
        digitalWrite(buttonLedPin, LOW);  // Turn OFF button LED
        fadePixel(activecolor, OFF, FADESTEPS, FADEDELAY); //Turn OFF the LED

        digitalWrite(GPS_WAKE_PIN, LOW); // Put GPS to sleep

        nbAccess.shutdown();  // Put modem to sleep
        LowPower.deepSleep(); // Enter sleep mode
      }


      delay(4000); //Give the modem some time vefore sending the next message

      //Collecting Data and Sending Data message
      if (statusinfo != 3 && batVoltage > SHUTDOWN_BATTERY_THRESHOLD) {
        
            //Check the multiplexer channels 5, 6 and 7 for attached Sensors and collect data
          for (int i = 5; i<=7; i++) {

            selectTCAChannel(i);
            delay(10); // small delay to allow switching
            scanI2CDevices(i);

          }
        sendDataMessage();
      }

      delay(4000);

      //Shutdown the modem
      Serial.println("INFO LOOP: Shutting down the modem after sending");
      client.stop();
      nbAccess.shutdown();

      //override sensor readings to invalid values. -> detect new readings on the server
      //Sensor values
      noise = -9999;
      ozone = -32768;
      aqi = -32768;
      tvoc = - -32768;
      co2 = -32768;
      temperature = -9999;
      humidity = -9999;

      //Only show lowbat status if no other major error occured
      if(batPercent <= LOW_BATTERY_THRESHOLD && statusinfo != 3 && statusinfo != 4) {
        statusinfo = 5;
      } 

      if (statusinfo == 2) { //Nothing weird happend
        statusinfo = 1;
        fadePixel(activecolor, RUNNING, FADESTEPS, FADEDELAY);
      } else if (statusinfo == 5) { //low battery
        fadePixel(activecolor, LOWBAT, FADESTEPS, FADEDELAY);
      }
    }



  } else {      //Wake up (if not isAwake)
    
    isAwake = true;
    
    

    fadePixel(OFF, STARTUP, FADESTEPS, FADEDELAY);

    // After waking up, turn LEDs back ON
    digitalWrite(buttonLedPin, HIGH);

    connectToNetwork();

    digitalWrite(GPS_WAKE_PIN, HIGH); // Wake up GPS
    encodeGPS();

    if (statusinfo == 0) {
      statusinfo = 1;
      fadePixel(activecolor, RUNNING, FADESTEPS, FADEDELAY);
    } 

    Serial.println("INFO LOOP 551: Sending wakeup status message!");
    sendStatusMessage();

    nbAccess.shutdown();
  }

}

void sendDataMessage() {

  String dataString = getGeoJSONDataString();

  unsigned int len = dataString.length() + 1;
  char data[len];
  dataString.toCharArray(data, len);
  
  // if you get a connection, report back via serial:
  if (client.connect(server, port)) {
    
      // Make a HTTP request:
      client.print("POST ");
      client.print(pathdata);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(server);
      client.println("Content-Type: text/plain");
      client.print("Content-Length: ");
      client.println(strlen(data));
      client.println();
      client.println(data);
      client.println("Connection: close");
      client.println();

    Serial.println("INFO LOOP 594: ...sent data to DATAhub:" + dataString);
    Serial.println();
    

  } else {
    statusinfo = 4;
    fadePixel(activecolor, NETWORKERROR, FADESTEPS, FADEDELAY);
    // if you didn't get a connection to the server:
    Serial.println("INFO LOOP 601: send Data message: Client not connected");
    connected = false;
    Serial.println("INFO LOOP 603: Shutting down the modem");
    nbAccess.shutdown();   //restart the modem
    delay(5000);
    connectToNetwork();
    
    
  }
}

void sendStatusMessage() {

  int batterylevel = batteryPercentage();
  int milliVolts = readBatteryVoltage()*1000;

  bool online = true;
  if(readBatteryVoltage() <= SHUTDOWN_BATTERY_THRESHOLD) online = false;

  String dataString = "{\"battery\": " + String(batterylevel) + ", \"millivolt\": " + String(milliVolts) + ", \"gps_fix\": " + gpsfix + ", \"online\": " + online + "}";

  unsigned int len = dataString.length() + 1;
  char data[len];
  dataString.toCharArray(data, len);
  
  // if you get a connection, report back via serial:
  if (client.connect(server, port)) {
    
      // Make a HTTP request:
      client.print("POST ");
      client.print(pathstatus);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(server);
      client.println("Content-Type: text/plain");
      client.print("Content-Length: ");
      client.println(strlen(data));
      client.println();
      client.println(data);
      client.println("Connection: close");
      client.println();

    Serial.println("INFO LOOP 655: sent status message: " + dataString);
    Serial.println();

  } else {
    statusinfo = 4;
    fadePixel(activecolor, NETWORKERROR, FADESTEPS, FADEDELAY);
    // if you didn't get a connection to the server:
    Serial.println("ERROR LOOP 663: sendStatusMessage: Client not connected");
    connected = false;
    Serial.println("INFO LOOP 665: Shutting down the modem");
    nbAccess.shutdown();   //restart the modem
    delay(5000);
    connectToNetwork();
    
    
  }

}

void connectToNetwork() {
    // After starting the modem with NB.begin()
  // attach to the GPRS network with the APN, login and password
  //&& (gprs.attachGPRS() == GPRS_READY)
  while (!connected) {
    
    
    Serial.println("INFO LOOP 685: Connecting to network!");
    
    if ((nbAccess.begin() == NB_READY)) {
      Serial.println("INFO LOOP 688: Connected to NB-IOT Service (A1)");
      connected = true;
    } else {
      statusinfo = 4;
      fadePixel(activecolor, NETWORKERROR, FADESTEPS, FADEDELAY);
      Serial.println("ERROR LOOP 693: Not connected to NB-IOT Service (A1)");
      delay(1000);
    }
  }
}

/********************************************* getGEOJSONString *********************************************************/
String getGeoJSONDataString() {

          // Convert GPS data to JSON format
  String geoJson = "{";
        geoJson += "\"type\":\"Feature\",";
        geoJson += "\"geometry\":{";
        geoJson += "\"type\":\"Point\",";
        geoJson += "\"coordinates\":[" + String(gps.location.lng(), 6) + "," + String(gps.location.lat(), 6) + "]";
        geoJson += "},";
        geoJson += "\"properties\":{";
        geoJson += "\"Noise\":[" + String(noise) + ", \"dB\"],";
        geoJson += "\"Ozone\":[" + String(ozone) + ", \"ppb\"],";
        geoJson += "\"AQI\":[" + String(aqi) + ", \"AQI\"],";
        geoJson += "\"TVOC\":[" + String(tvoc) + ", \"ppb\"],";
        geoJson += "\"CO2\":[" + String(co2) + ", \"ppm\"],";
        geoJson += "\"Temperature\":[" + String(temperature) + ", \"°C\"],";
        geoJson += "\"Humidity\":[" + String(humidity) + ", \"%\"],";
        geoJson += "\"timestamp\":\"""\"";
        geoJson += "}";
        geoJson += "}";
  
  
  return geoJson;
}

String getGeoJSONFakeGPSString() {

          // Convert GPS data to JSON format
  String geoJson = "{";
        geoJson += "\"type\":\"Feature\",";
        geoJson += "\"geometry\":{";
        geoJson += "\"type\":\"Point\",";
        geoJson += "\"coordinates\":[48.142748,16.391135]";
        geoJson += "},";
        geoJson += "\"properties\":{";
        geoJson += "\"Noise\":[" + String(noise) + ", \"dB\"],";
        geoJson += "\"Ozone\":[" + String(ozone) + ", \"ppm\"],";
        geoJson += "\"AQI\":[" + String(aqi) + ", \"ppm\"],";
        geoJson += "\"TVOC\":[" + String(tvoc) + ", \"ppm\"],";
        geoJson += "\"CO2\":[" + String(co2) + ", \"ppm\"],";
        geoJson += "\"Temperature\":[" + String(temperature) + ", \"°C\"],";
        geoJson += "\"Humidity\":[" + String(humidity) + ", \"%\"],";
        geoJson += "\"timestamp\":\"""\"";
        geoJson += "}";
        geoJson += "}";
  
  
  return geoJson;
}



/********************************************* encodeGPS *********************************************************/
void encodeGPS() {

  // Select GPS channel on TCA9548A
  selectTCAChannel(TCA_GPS_CHANNEL); // Activate Channel 0

  // Send wakeup command
  GPS.sendCommand(" ");

  unsigned long start = millis();
  // For one second we parse GPS data and report some key values
  for (start; millis() - start < 1000;) {
    while (myI2CGPS.available()) {  //available() returns the number of new bytes available from the GPS module
      gps.encode(myI2CGPS.read());  //Feed the GPS parser
    }
  }

  //Debugging
  //if we haven't seen lots of data in 5 seconds, something's wrong.
  if (start > 5000 && gps.charsProcessed() < 10) {
    statusinfo = 3;
    fadePixel(activecolor, GPSERROR, FADESTEPS, FADEDELAY);
    Serial.println("ERROR: Not getting any GPS data! Encoding again");
    gpsfix = false;
    encodeGPS();                         //Encode again
  } else if (!gps.location.isValid()) {  //also add timer for circumstances when there is definately no signal to find
    statusinfo = 3;
    fadePixel(activecolor, GPSERROR, FADESTEPS, FADEDELAY);
    Serial.println("ERROR: GPS Data not valid!");
    gpsfix = false;
    //encodeGPS();  //Encode again
  } else if (gps.satellites.value() < 3) {
    statusinfo = 3;
    fadePixel(activecolor, GPSERROR, FADESTEPS, FADEDELAY);
    Serial.println("ERROR: Not enough GPS satellites:" + String(gps.satellites.value()));
    gpsfix = false;
    //encodeGPS();  //Encode again
  } else {
    Serial.println("GPS encoded");
    gpsfix = true;
  }

  // Send standby command
  GPS.sendCommand("$PMTK161,0*28");
}

/********************************************* PrintGPSDATA *********************************************************/
void printGPSData() {

  //We have new GPS data to deal with!
  Serial.println();

  if (gps.time.isValid()) {
    Serial.print(F("Date: "));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());

    Serial.print((" Time: "));
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());

    Serial.println();  //Done printing time
  } else {
    Serial.println(F("Time not yet valid"));
  }

  if (gps.location.isValid()) {
    Serial.print("Location: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(", "));
    Serial.print(gps.location.lng(), 6);
    Serial.println();
  } else {
    Serial.println(F("Location not yet valid"));
  }
}

/********************************************* I2C Multiplexer *********************************************************/

// Scan I2C devices only on the specified TCA9548A channel
void scanI2CDevices(uint8_t channel) {
  
  bool sensorFound = false;  // Flag to track if any sensor is found on the channel

  for (uint8_t address = 1; address < 127; address++) {

    if (address == TCA9548A_ADDR) continue;  // Skip multiplexer itself
    if (address == GPS_I2C_ADDR) continue;  // Skip GPS
    if (address == 0x60) continue;  // Skip unknown device
    if (address == 0x6B) continue;  // Skip unknown device

    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {  // If the device is found
    
       /* Sensors: 
        * - Adafruit SCD41 CO2, Temp, Humidity. (I2C Address: 0x62);
        * - PCB Artists I2C Decibel Sound Level Meter Module (I2C Address: 0x48);
        * - Adafruit ENS 160 Gas Sensor (I2C Address: 0x53);
        * - DF Robot SEN0321 Ozone Sensor (I2C Address: 0x73); https://wiki.dfrobot.com/Gravity_IIC_Ozone_Sensor_%280-10ppm%29%20SKU_SEN0321
      */

      // Try to identify known sensors
      switch (address) {
        case SCD41_ADDR:
          Serial.println("INFO: SCD41 found at channel: " + String(channel));
          collectSCDData();
          scd41channel = channel;
          break;
        case DBM_ADDR:
          Serial.println("INFO: Sound Level Meter found at channel: " + String(channel));
          collectDbData(); //Measure Sound Level
          soundchannel = channel;
          break;
        case ENS160_ADDR:
          Serial.println("INFO: ENS 160 found at channel: " + String(channel));
          collectMoxData();
          ens160channel = channel;
          break;
        case OZONE_ADDR:
          Serial.println("INFO: Ozone sensor found at channel: " + String(channel));
          collectOzoneData();
          ozonechannel = channel;
          break;
        default:
          Serial.print(" -> Unknown device: ");
          break;
      }
      sensorFound = true;  // Mark that we found a device
    }
  }

  if (!sensorFound) {
    if (channel == scd41channel) scd41channel = 0; //Sensor was previously on this channel but is not anymore
    if (channel == soundchannel) soundchannel = 0; //Sensor was previously on this channel but is not anymore
    if (channel == ens160channel) ens160channel = 0; //Sensor was previously on this channel but is not anymore
    if (channel == ozonechannel) ozonechannel = 0; //Sensor was previously on this channel but is not anymore
  }

}


/********************************************* Sound Level Meter *********************************************************/

void collectDbData() {
  
  float sum = 0.0;
  int count = 0;
  unsigned long startTime = millis();
  float avg = 0;

  while (millis() - startTime < dbSampleDuration) {
    float dB = dbmeter_readreg (DBM_REG_DECIBEL);
    if (dB > 0 && dB !=255) {       //Reading is 255 if sensor is not connected / disconnected
      sum += dB;
      count++;
    } 

  }
  
  if (count > 0) {
    avg = sum / count;
    Serial.print("Average over 3 seconds: ");
    Serial.print(avg);
    Serial.println(" dB");
  } else {
    Serial.println("No valid readings.");
  }

  if(avg == 0) {
    noise = -9999;            //no data collected at all
  } else {
    noise = avg;              //store avg in global variable
  }

}

/********************************************* Ozone Data *********************************************************/

void collectOzoneData() {

  // IMPORTANT: note: it takes time to stable oxygen concentration, about 3 minutes.

  //init sensor
  if(ozonechannel == 0) {
     if(!Ozone.begin(OZONE_ADDR)){
        Serial.println("ERROR: Ozone sensor not initialized! (device number error)");
        delay(1000);
    } else {
    Serial.println("INFO: Ozone Sensor connect successfully!");
    }
    Ozone.setModes(MEASURE_MODE_PASSIVE);

    delay(1000); //Give the sensor some time to set up

  
  }

  // Data collection
  ozone = Ozone.readOzoneData(20);  // COLLECT_NUMBER                 The collection range is 1-100
  Serial.print("Ozone concentration is ");
  Serial.print(ozone);
  Serial.println(" PPB.");
 
}

/********************************************* MOX Data *********************************************************/

void collectMoxData() {
 Serial.println("INFO: Starting to collect ENS160 Data on channel: " + String(ens160channel));
 //init sensor 
 if(ens160channel == 0) {       //If the channel number is still 0 init the sensor
  ens160.begin();
  delay(1000); //Give the sensor some time to init
  if(ens160.available()) {
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "Info: ENS160 set mode: DONE." : "Info: ENS160 set mode: FAILED!");
  }

 }

 if (ens160.available()) {
    ens160.measure(true);
    ens160.measureRaw(true);

    aqi = ens160.getAQI();
    tvoc = ens160.getTVOC();

    Serial.print("AQI: ");Serial.print(aqi);Serial.print("\t");
    Serial.print("TVOC: ");Serial.print(tvoc);Serial.print("ppb\t");
  } else {
    Serial.println("ERROR: ENS160 MOX Sensor not available!");
  }

}
/********************************************* CO2, Temperature, Humidity Data *********************************************************/

void collectSCDData() {

  //Iinitialize the sensor
  if(scd41channel == 0) {

    scd_sensor.begin(Wire, SCD41_ADDR);
    delay(30);

    // Ensure sensor is in clean state
    scderror = scd_sensor.wakeUp();
    if (scderror != NO_ERROR) {
        Serial.print("Error trying to execute wakeUp(): ");
        errorToString(scderror, scderrorMessage, sizeof scderrorMessage);
        Serial.println(scderrorMessage);
    }

    scderror = scd_sensor.reinit();
    if (scderror != NO_ERROR) {
        Serial.print("Error trying to execute reinit(): ");
        errorToString(scderror, scderrorMessage, sizeof scderrorMessage);
        Serial.println(scderrorMessage);
    }

  }
  
  //Read Sensor data

    uint16_t co2Concentration = 0;

    //
    // Ignore first measurement after wake up.
    //
    scderror = scd_sensor.measureSingleShot();
    if (scderror != NO_ERROR) {
        Serial.print("Error trying to execute measureSingleShot(): ");
        errorToString(scderror, scderrorMessage, sizeof scderrorMessage);
        Serial.println(scderrorMessage);
        return;
    }
    //
    // Perform single shot measurement and read data.
    //
    scderror = scd_sensor.measureAndReadSingleShot(co2Concentration, temperature, humidity);
    co2 = co2Concentration;

    if (scderror != NO_ERROR) {
        Serial.print("Error trying to execute measureAndReadSingleShot(): ");
        errorToString(scderror, scderrorMessage, sizeof scderrorMessage);
        Serial.println(scderrorMessage);
        return;
    }
    //
    // Print results in physical units.
    //
    Serial.print("CO2 concentration [ppm]: ");
    Serial.print(co2);
    Serial.println();
    Serial.print("Temperature [°C]: ");
    Serial.print(temperature);
    Serial.println();
    Serial.print("Relative Humidity [RH]: ");
    Serial.print(humidity);
    Serial.println();

}

/********************************************* LED *********************************************************/

// Function to fade between two colors
void fadePixel(uint32_t start, uint32_t target, int steps, int delayTime) {
  // Extract the RGB values of the start and target colors
  uint8_t startR = (start >> 16) & 0xFF;
  uint8_t startG = (start >> 8) & 0xFF;
  uint8_t startB = start & 0xFF;

  uint8_t targetR = (target >> 16) & 0xFF;
  uint8_t targetG = (target >> 8) & 0xFF;
  uint8_t targetB = target & 0xFF;

  // Calculate the difference between the starting and target color
  int deltaR = targetR - startR;
  int deltaG = targetG - startG;
  int deltaB = targetB - startB;

  // Fade in steps
  for (int i = 0; i <= steps; i++) {
    uint8_t r = startR + (deltaR * i) / steps;
    uint8_t g = startG + (deltaG * i) / steps;
    uint8_t b = startB + (deltaB * i) / steps;

    // Set the color to the current fade step
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();

    if(target != OFF) activecolor = target;
    
    delay(delayTime);
  }
}

/********************************************* BATTERY *********************************************************/

// Function to read battery voltage
float readBatteryVoltage() {
  
  // read the input on analog pin 0:
  int sensorValue = analogRead(ADC_BATTERY);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  float voltage = (sensorValue * (4.3 / 1023.0)); //*0.969543; //0.969543 is here for compensating difference between measured and AnalogRead value
  // print out the value you read:

  return voltage;

}

int batteryPercentage() {

    float voltage = readBatteryVoltage();
    if (voltage >= 4.0) return 100;
    else if (voltage >= 3.9) return map(voltage * 100, 390, 400, 85, 100);
    else if (voltage >= 3.8) return map(voltage * 100, 380, 390, 70, 85);
    else if (voltage >= 3.7) return map(voltage * 100, 370, 380, 55, 70);
    else if (voltage >= 3.6) return map(voltage * 100, 360, 370, 40, 55);
    else if (voltage >= 3.5) return map(voltage * 100, 350, 360, 25, 40);
    else if (voltage >= 3.4) return map(voltage * 100, 340, 350, 10, 25);
    else if (voltage >= 3.3) return map(voltage * 100, 330, 340, 1, 10);
    else return 0;
}