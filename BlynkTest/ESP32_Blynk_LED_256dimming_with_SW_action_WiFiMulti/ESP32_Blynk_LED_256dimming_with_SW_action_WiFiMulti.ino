/*
 ESP32-DevkitC Blynk LED dimming with momentary push switch action
 
  ledcAnalogWrite : Arduino like analogWrite (PWM) for ESP32

  Mutiple Wi-Fi
  LED button function : every push brighten change 25%, 50% 75%, 100%, and 0%
                        over 2sec long push : 0% -> 100%, else -> 0%
                        Blynk Slider sync. (V1)
                        fade every change brighten
                        4 types of the brighten lookup tables
  button connect GPIO5
  Ken Kishita (kkishita@aynik-tech.jp)
  Aynik Technology Co., Ltd. http://www.aynik-tech.jp
*/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define _min(a,b) ((a)<(b)?(a):(b))
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "blynk auth token here";
char ssid1[] = "SSID1";
char pass1[] = "passphase for SSID1";
char ssid2[] = "SSID2";
char pass2[] = "passphase for SSID2";
char ssid3[] = "SSID3";
char pass3[] = "passphase for SSID3";
WiFiMulti wifiMulti;

#define LEDC_CHANNEL_0     0 // use first channel of 16 channels (started from zero)
#define LEDC_TIMER_13_BIT  13 // use 13 bit precission for LEDC timer
#define LEDC_BASE_FREQ     500 // use 500 Hz as a LEDC base frequency
// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN            2  //default pull-down pin for ESP32

byte ledState[] = {
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,
7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,
21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,34,35,35,36,37,38,38,39,
40,41,41,42,43,44,45,46,46,47,48,49,50,51,52,53,53,54,55,56,57,58,59,60,61,62,63,64,
65,66,67,68,69,70,71,72,73,74,75,77,78,79,80,81,82,83,84,86,87,88,89,90,91,93,94,95,
96,98,99,100,101,103,104,105,106,108,109,110,112,113,114,116,117,118,120,121,122,124,
125,127,128,129,131,132,134,135,137,138,140,141,143,144,146,147,149,150,152,153,155,
156,158,159,161,163,164,166,167,169,171,172,174,176,177,179,181,182,184,186,187,189,
191,193,194,196,198,200,201,203,205,207,208,210,212,214,216,218,219,221,223,225,227,
229,231,233,234,236,238,240,242,244,246,248,250,252,254,255
};
 /*
//gamma = 2.0
*/

const int fadeAmount = 64;    // how many points to fade the LED by
volatile int ledValue = 0;    // how bright the LED is
volatile int lastLedValue = ledValue;
int _tv = 255;
int ledCValue = ledValue;
const int btnPin = 5; // connect physical button 
boolean btnState = false;
int buttonState = LOW;
volatile int state = 0;  //statemachine
uint32_t w1Time = 0;
uint32_t w2Time = 0;
const uint32_t debounceDelay = 20; //20ms
const uint32_t forceMinMax = 2000; //2s
// Arduino like analogWrite for ESP32. value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) 
{  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * _min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void fadeLed(uint8_t channel, uint8_t sidx, uint8_t eidx)
{
  if (sidx != eidx) {
    // fade in
    if (sidx < eidx) {
      for (int i = sidx; i <= eidx; i++) {
      ledcAnalogWrite(LEDC_CHANNEL_0, ledState[i]);
      delayMicroseconds(1000);
      }
    // fade out
    } else if (sidx > eidx) {
      for (int i = sidx; i >= eidx; i--) {
      ledcAnalogWrite(LEDC_CHANNEL_0, ledState[i]);
      delayMicroseconds(1000);    
      }
    }
    delayMicroseconds(2000);
    ledcAnalogWrite(LEDC_CHANNEL_0, ledState[eidx]); //Gleanings
  }
}
// LED
BLYNK_WRITE(V1) //Slider widget set value 0 to 100
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();
  Serial.print("V1 Slider value is: ");
  Serial.print(pinValue);
  ledValue = map(pinValue, 0, 100, 0, 255);
  fadeLed(LEDC_CHANNEL_0, lastLedValue, ledValue);
  Serial.print(", LED : ");
  Serial.print(ledValue);
  Serial.print(", ");
  Serial.print(ledState[ledValue]);
  Serial.print(", ");
  Serial.print(lastLedValue);
  Serial.print(", ");
  Serial.println(ledState[lastLedValue]);
  lastLedValue = ledValue;
}

void setup()
{
  pinMode(btnPin, INPUT_PULLUP);
  // Debug console
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  wifiMulti.addAP(ssid1, pass1);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP(ssid2, pass2);
  wifiMulti.addAP(ssid3, pass3);

  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
  Serial.println('\n');
  
  delay(1000);
  Blynk.config(auth);  // in place of Blynk.begin(auth, ssid, pass);
  Blynk.connect(3333);  // timeout set to 10 seconds and then continue without Blynk
  while (Blynk.connect() == false) {
    // Wait until connected
  }
  Serial.println("Connected to Blynk server");

  int pinValue = map(ledValue, 0, 255, 0, 100);
  Blynk.virtualWrite(V1, pinValue);
  delay(800);
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
  uint8_t t_sb = 5; //stanby 1000ms
  uint8_t filter = 0; //filter O = off
  uint8_t osrs_t = 4; //OverSampling Temperature x4
  uint8_t osrs_p = 4; //OverSampling Pressure x4
  uint8_t osrs_h = 4; //OverSampling Humidity x4
  uint8_t Mode = 3; //Normal mode
  delay(500);
}

void loop()
{
  Blynk.run();
  if ((ledValue != lastLedValue) && (_tv != lastLedValue)) { //Synchronize Slider position
    ledCValue = ((ledValue > 255) ? 0 : ledValue );
    int pinValue = map(ledCValue, 0, 255, 0, 100);
    Blynk.virtualWrite(V1, pinValue);
    _tv = lastLedValue;  //only one time call every change by button
  }

  switch (state) {
    case 0:   //Push
      buttonState = !digitalRead(btnPin);
        if (buttonState == HIGH) {
          if (w1Time == 0) w1Time = millis();
          if ((millis() - w1Time) > debounceDelay) {
            btnState = true;
            Serial.println("Press Button");
            if (ledValue == 0) ledValue = fadeAmount-1;
            else ledValue += fadeAmount;
            w1Time = 0;
            state = 1; 
          }
        }
      break;
    case 1:
      buttonState = !digitalRead(btnPin);
        if (buttonState == LOW) {  // wait release the button after push
          if (w1Time == 0) w1Time = millis();
          if ((millis() - w1Time) > debounceDelay) {
            // release button
            if (ledValue > 255) ledValue = 0;
            Serial.print("Release Button : ");
            Serial.print(ledValue);
            Serial.print(" , ");
            Serial.println(lastLedValue);
            fadeLed(LEDC_CHANNEL_0, lastLedValue, ledValue);
            lastLedValue = ledValue;
            w1Time = 0;
            w2Time = 0;
            state = 0;
          }
        } else { //long time push check
           if (w2Time == 0) w2Time = millis(); 
           if ((millis() - w2Time) > forceMinMax) {
                ledValue = ((lastLedValue == 0) ? 255:0);
                Serial.println("Force ON/OFF");
                fadeLed(LEDC_CHANNEL_0, lastLedValue, ledValue); 
                w1Time = 0;
                w2Time = 0;
                state = 2;
           }
        }
        break;
    case 2:   //Release after Force
      buttonState = !digitalRead(btnPin);
        if (buttonState == LOW) {
          if (w1Time == 0) w1Time = millis();
          if ((millis() - w1Time) > debounceDelay) {
            Serial.println("Release Button after Force");
                int vpinValue = (ledValue ? 100 : 0);
                Blynk.virtualWrite(V1, vpinValue);
                delay(200);
            lastLedValue = ledValue;
            w1Time = 0;
            state = 0;
            delay(800);
          }
        }
      break;

    default:
      state = 0;
      break;
  } //end switch case    
}

