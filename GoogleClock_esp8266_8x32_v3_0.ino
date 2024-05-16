/*
v.0.0 - original clock - https://www.hackster.io/mircemk/esp8266-animated-clock-on-8x8-led-matrices-4867ae
v.1.0 - extract data by Nicu FLORICA (niq_ro)
v.1.a - added data on Serial Monitor, extract as in https://github.com/DIYDave/HTTP-DateTime library
v.1.b - added data on display
v.1.c - test real DS18B20
v.1.c1  small updates
v.1.d - test 12-hour format
v.1.e - update info as in DIYDave library for extract date
v.1.f - update info at every 10 minutes (0,10,20,30,40,50) + temperature stay a little
v.1.g - added name of the day and month (romainan and english language) 
v.1.h - added automatic DST (summer/winter time) using info from https://www.timeanddate.com/time/change/germany and in special from https://www.instructables.com/WiFi-NodeMCU-ESP8266-Google-Clock/
v.2.h - ported to RPi Pico W
v.2.h.1 - added status about wifi connexion (internal onboard led), moved to WiFi.h library, reset after 10 times after not read data from net or at changed of the day if no internet connexion
v.3.0 - changed on 8x32 addressable led matrix
*/

#include "Arduino.h"
//#include <WiFi.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <HttpDateTime.h>            // https://github.com/DIYDave
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>  // https://github.com/adafruit/Adafruit_NeoMatrix
#include <Adafruit_NeoPixel.h>  
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#include <OneWire.h>
#include <DallasTemperature.h>
#define SENSOR_RESOLUTION 10
const int oneWireBus =12;  // GPIO14 = D6, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);


//#define DSTpin 12 // GPIO14 = D6, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
//#define PIN 14 // D5 = GPIO14  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
 #define LED_PIN D5 // LED matrix configurations 
 #define LED_BOARD_WIDTH 32 
 #define LED_BOARD_HEIGHT 8 
 #define LED_TILE_COLUMN 1 
 #define LED_TILE_ROW 2 

// MATRIX DECLARATION:
// Parameter 1 = width of the matrix
// Parameter 2 = height of the matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

/*
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);   // https://www.adafruit.com/product/2294
*/
 // Instantiated matrix 
 // https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library // 
 Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix( LED_BOARD_WIDTH, LED_BOARD_HEIGHT, LED_TILE_COLUMN, LED_TILE_ROW,
                              LED_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG +
                              NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS + NEO_TILE_PROGRESSIVE, NEO_GRB + NEO_KHZ800); 
                              
const uint16_t colors[] = {
  matrix.Color(255, 0, 0),    // red
  matrix.Color(0, 255, 0),    // green
  matrix.Color(255, 255, 0),  // yellow
  matrix.Color(0, 0, 255),    // blue 
  matrix.Color(255, 0, 255),  // mouve
  matrix.Color(0, 255, 255),  // bleu (ligh blue)
  matrix.Color(255, 255, 255) // white 
  };

// WS2812b day / night brightness.
#define NIGHTBRIGHTNESS 2      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 10


char result[4];

WiFiClient client;

String date;


// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char* ssid     = "bbk3";              // SSID WiFi nam
const char* password = "internet3";         // WiFi password

float utcOffset = 2;                        // Timezone offset (in hour)
// =======================================================================

int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx=0;
int dy=0;
byte del=0;
int h,m,s;

int ziua,zi,luna,luna2,an;
String luna1,ziua1;
long localEpoc = 0;
long localMillisAtUpdate = 0;

int h1, h0; // hour to print
int h7, h3;

byte preluare = 0;
int lenghdrc;
int lenghdrc1;


//Week Days (english)
String weekDays0[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//Month names (english)
String months0[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
//Week Days (romanian)
String weekDays1[7]={"Duminica", "Luni", "Marti", "Miercuri", "Joi", "Vineri", "Sambata"};
//Month names (romanian)
String months1[12]={"Ianuarie", "Februarie", "Martie", "Aprilie", "Mai", "Iunie", "Iulie", "August", "Septembrie", "Octombrie", "Noiembrie", "Decembrie"};

byte summerTime = 0;
byte contor = 0;
int secundar = 0;
byte culoare = 1;
int x;
int lderece1;
int lderece;


void setup() 
{
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(NIGHTBRIGHTNESS);  // original 255
  matrix.setTextColor(colors[6]);
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.print(F("v.3.0"));
  matrix.setPixelColor(255, 150, 0, 0);
  matrix.show();
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("=");
  Serial.println("=======================================================================");
  sensors.begin();
 
  WiFi.begin(ssid, password);
   digitalWrite(LED_BUILTIN, HIGH);
    delay(500); 
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);   
  while (WiFi.status() != WL_CONNECTED) 
    {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(900); 
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);   
    }
}
// =======================================================================


// =======================================================================
void loop()
{    
    if ((m%10 == 0) and (preluare == 0))
   {
    if (WiFi.status() != WL_CONNECTED) 
    {
    WiFi.begin(ssid, password);             // Connect to the network
    digitalWrite(LED_BUILTIN, HIGH); 
    contor++;
    }
    if (contor > 100)
    {
    Serial.println("Unable to connect to network, rebooting ...");
    ESP.restart();
    }
    getTime();
    clkTime = millis();
    preluare = 1;
    Serial.println("->");
   }
  if ((m%10 >= 9) and (preluare == 1))
   {
    preluare = 0;
    Serial.println("<-");
   } 
   
  if(millis()-clkTime > 30000)
  {     
    String derece1;
     if (m%2 == 0) 
    {
      derece1 += "DST=";
      derece1 += summerTime;
      derece1 += ", ";
    }
    else
    {
      if (summerTime == 0)
      derece1 += "ora standard";
      else
      derece1 += "ora vara";
      derece1 += ", ";
    }
    if (m%2 == 0) 
    {
      derece1 += weekDays0[ziua%7];
    }
    else
    {
      derece1 += weekDays1[ziua%7];
    }
    derece1 += ", "; 
    derece1 += zi/10;
    derece1 += zi%10;
    derece1 += "-";
    /*
    derece1 += luna/10;
    derece1 += luna%10;
    */
    if (m%2 == 0)
    {
      derece1 += months0[luna-1];
    }
    else
    {
      derece1 += months1[luna-1];
    }   
    derece1 += "-"; 
    derece1 += an;

  lderece1 = 6*derece1.length(); // https://reference.arduino.cc/reference/en/language/variables/data-types/string/functions/length/

  culoare++;
  if (culoare >= 6) culoare = 0;   
  if (m/10 == culoare) culoare++;
  if (culoare >= 6) culoare = 0;
  matrix.setTextColor(colors[culoare]);
  
  x = 32;
  for (x; x > -lderece1 ; x--)
  {
  matrix.fillScreen(0); 
  matrix.setCursor(x, 0);
  matrix.print(derece1);  
 // ceasbinar();
  matrix.show();
  delay(75);
  }
    delay(200);
    clkTime = millis();
  }

  updateTime();

  secundar = millis()/1000%2;
  String ceas = "";
  if (h3/10 == 0) 
  ceas = ceas + "0";
  ceas = ceas + h3;
  if (secundar%2 == 0)
    ceas = ceas + ":";
  else
    ceas = ceas + " ";
  if (m/10 == 0) 
  ceas = ceas + "0";
  ceas = ceas + m;
  
  matrix.fillScreen(0); 
  matrix.setTextColor(colors[m/10]);
  matrix.setCursor(1, 0);
  matrix.print(ceas);
  matrix.show();
 
delay(100); 
}  // end main loop
  

// =======================================================================



void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println(".");
    repeatCounter++;
    
  }

  String line;
  client.setNoDelay(false);
  while(client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    //Serial.println(line);
    if (line.startsWith("DATE: ")) {
      //date = "     "+line.substring(6, 17);
      date = ""+line.substring(6, 22); //17);
      ziua1 = line.substring(6, 9);
      luna1 = line.substring(14, 17);
      luna2 = line.substring(14, 17).toInt();
      zi = line.substring(11, 13).toInt();
      an = line.substring(18, 22).toInt();
      h0 = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      zitozi();
      lunatoluna();
      localMillisAtUpdate = millis();
      localEpoc = (h0 * 60 * 60 + m * 60 + s);
      Serial.print(date);
      Serial.print(" -> ");
      Serial.print(ziua1);
      Serial.print(" (");  
      Serial.print(ziua);
      Serial.print("), ");      
      Serial.print(zi/10);
      Serial.print(zi%10);
      Serial.print("-");
      Serial.print(luna1);
      Serial.print(" (");  
      Serial.print(luna);
      Serial.print(") - ");  
      Serial.print(an);      
      Serial.print(", ");
      Serial.print(h0/10);
      Serial.print(h0%10);
      Serial.print(":");
      Serial.print(m/10);
      Serial.print(m%10);
      Serial.print(":");
      Serial.print(s/10);
      Serial.println(s%10);    
    }
  }
  client.stop();

// https://www.instructables.com/WiFi-NodeMCU-ESP8266-Google-Clock/ and https://www.hackster.io/anthias64/wifi-nodemcu-esp8266-google-clock-0804d0
    if (luna>3 && luna<=10) summerTime=1;
    if (luna==3 && zi>=31-(((5*an/4)+4)%7) ) summerTime=1;
    if (luna==10 && zi>=31-(((5*an/4)+1)%7) ) summerTime=0;
    Serial.print("Summertime: ");
    Serial.println(summerTime);


  long epoch1 = fmod(round(localEpoc + 3600 * (utcOffset + summerTime) + 86400L), 86400L);
  h3 = ((epoch1  % 86400L) / 3600) % 24;
  Serial.print(h3); 
  Serial.print(" -> ");
  Serial.println(h0); 

// In case of positive timezone offset 
if (utcOffset > 0)
{  
 //   if (h3  > 23)
    if (h3 < h0) 
    {
    zi++;
    ziua++;
    if (ziua > 7) ziua = 1;
    }
  
    if (zi == 29)
      if (luna == 2 && an % 4 !=0){   // Kein Schaltjahr = 28 Tage
        luna = 3;
        zi = 1;
      }          
    if (zi == 30)
      if (luna == 2 && an % 4 ==0){   // Schaltjahr = 29 Tage
        luna = 3;
        zi = 1;
      } 
     if (zi == 31)
      if (luna == 4 || luna == 6 || luna == 9 || luna == 11){   // 30 tage
        luna++;
        zi = 1;     
      }
     if (zi == 32)
      if (luna == 1 || luna == 3 || luna == 5 || luna == 7 || luna == 8 || luna == 10){   // 31 tage
        luna++;
        zi = 1;   
      }else 
      {           
        if (luna == 12){
          luna = 1;
          zi = 1; 
          an++;
        }                 
      }
  }

// In case of negative timezone offset 
  if (utcOffset < 0)
  {
    zi--;
    ziua--;
    if (ziua < 1) ziua = 7;
  }
  if (zi < 1){
    luna--;
    if (luna < 1)
    {
      luna = 12;    
      an--;           
    }
    
      if (luna == 1)
      {
        zi = 31;
      }
      if (luna == 2)
      {
        if (an % 4 !=0) zi = 28;   //Kein Schaltjahr
        if (an % 4 ==0) zi = 29;   // Schaltjahr
      }
      if (luna == 3)
      {
        zi = 31;
      }
      if (luna == 4)
      {
        zi = 30;
      }
      if (luna == 5)
      {
        zi = 31;
      }
      if (luna == 6)
      {
        zi = 30;
      }
      if (luna == 7)
      {
        zi = 31;
      }
      if (luna == 8)
      {
        zi = 31;
      }
      if (luna == 9)
      {
        zi = 30;
      }
      if (luna == 10)
      {
        zi = 31;
      }
      if (luna == 11)
      {
        zi = 30;
      }
      if (luna == 12)
      {
        zi = 31;
      }
  }  

      Serial.print("Offset = ");
      Serial.print(utcOffset);
      Serial.print(", DST = ");
      Serial.print(summerTime);
      Serial.print(" -> ");
      Serial.print(ziua1);
      Serial.print(" (");  
      Serial.print(ziua);
      Serial.print("), ");      
      Serial.print(zi/10);
      Serial.print(zi%10);
      Serial.print("-");
      Serial.print(luna1);
      Serial.print(" (");  
      Serial.print(luna);
      Serial.print(") - ");  
      Serial.print(an);      
      Serial.print(", ");
      Serial.print(h3/10);
      Serial.print(h3%10);
      Serial.print(":");
      Serial.print(m/10);
      Serial.print(m%10);
      Serial.print(":");
      Serial.print(s/10);
      Serial.println(s%10);  
 
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = fmod(round(curEpoch + 3600 * (utcOffset + summerTime) + 86400L), 86400L);
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60; 
}

// =======================================================================

void zitozi()
{
  if (ziua1 == "MON") ziua = 1;
  else
  if (ziua1 == "TUE") ziua = 2;
  else
  if (ziua1 == "WED") ziua = 3;
  else
  if (ziua1 == "THU") ziua = 4;
  else
  if (ziua1 == "FRI") ziua = 5;
  else
  if (ziua1 == "SAT") ziua = 6;
  else 
  if (ziua1 == "SUN") ziua = 7;
}

void lunatoluna()
{
  if (luna1 == "JAN") luna = 1;
   else
  if (luna1 == "FEB") luna = 2;
   else
  if (luna1 == "MAR") luna = 3;
   else
  if (luna1 == "APR") luna = 4;
   else
  if (luna1 == "MAY") luna = 5;
   else
  if (luna1 == "JUN") luna = 6;
   else
  if (luna1 == "JUL") luna = 7;
   else
  if (luna1 == "AUG") luna = 8;
   else
  if (luna1 == "SEP") luna = 9;
   else
  if (luna1 == "OCT") luna = 10;
   else
  if (luna1 == "NOV") luna = 11; 
   else
  if (luna1 == "DEC") luna = 12;   
}
