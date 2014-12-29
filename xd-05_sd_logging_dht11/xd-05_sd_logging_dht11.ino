/*
  SD card datalogger
http://arduino.cc/en/Tutorial/Datalogger

 This example shows how to log data from three analog sensors
 to an SD card using the SD library.

 The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4

 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

#include <SPI.h>
#include <SD.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 53; //4;

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;



//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
  return 1.8 * celsius + 32;
}

// fast integer version with rounding
//int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}


//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
  return celsius + 273.15;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T);
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}


#include <dht11_robtillaart.h>

dht11_robtillaart DHT11;

#define DHT11PIN 2



void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(53 , OUTPUT); //10

  // see if the card is present and can be initialized:
  if (!SD.begin(10, 11, 12, 13)) {  //chipSelect
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  #ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  }
  
  
  Serial.println("DHT11 LOGGING PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();
}

void loop()
{
  DateTime now = rtc.now();

  // make a string for assembling the data to log:
  String dataString = "";

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    
    dataString += String(now.year()) + '/' + String(now.month()) + '/' + String(now.day());
    dataString += ' ' + String(now.hour()) + ':' + String(now.minute()) + ':' + String(now.second());
   
  Serial.println(now.unixtime());
 

  dataString += ","  + String(now.unixtime()) ;

  
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
  case DHTLIB_OK: 
    Serial.println("OK"); 
    break;
  case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error"); 
    break;
  case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error"); 
    break;
  default: 
    Serial.println("Unknown error"); 
    break;
  }

  Serial.print("Humidity (%): ");
  Serial.println((float)DHT11.humidity, 2);
  
  dataString += "," + String(DHT11.humidity);
  
  Serial.print("Temperature (°F): ");
  Serial.println(Fahrenheit(DHT11.temperature), 2);

  dataString += "," + String(Fahrenheit(DHT11.temperature));

  Serial.print("Dew PointFast (°C): ");
  Serial.println(Fahrenheit(dewPointFast(DHT11.temperature, DHT11.humidity)));

  dataString += "," + String(Fahrenheit(dewPointFast(DHT11.temperature, DHT11.humidity)));

  // read three sensors and append to the string:
//  for (int analogPin = 0; analogPin < 3; analogPin++) {
//    int sensor = analogRead(analogPin);
 //   dataString += String(sensor);
//    if (analogPin < 2) {
//      dataString += ",";
//    }
//  }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("dht11.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
  delay(5000);

}
