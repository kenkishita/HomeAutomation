/*
 ESP32 Arduino IDE
 https://github.com/espressif/arduino-esp32
 Blynk library for Arduino
 https://github.com/blynkkk/blynk-library/releases/tag/v0.4.10
 BME280 I2C library
 https://github.com/mgo-tec/ESP32_BME280_I2C
 
  */
#define BLYNK_PRINT Serial
#define _min(a,b) ((a)<(b)?(a):(b))

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "ESP32_BME280_I2C.h"

ESP32_BME280_I2C bme280i2c(0x76, 26, 25, 400000); //address, SCL, SDA, frequency

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "XXXXXXXX Auth Code XXXXXXXXX";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "WiFi SSID";
char pass[] = "WiFi Pass Code";

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0
// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13
// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000
// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN            5

int ledValue = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by
byte ledState[] = {
   0,   1,   2,   3,   5,   6,   8,   9,
  10,  13,  14,  17,  20,  22,  25,  28,
  32,  36,  41,  49,  56,  64, 74, 80,
 87, 95, 102, 112, 123, 130, 204, 255
};
int i = 0;
int level = sizeof(ledState);
char buff[50];

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * _min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

//Slider range set 0 to 100 in the BlynkApp
BLYNK_WRITE(V1)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();
  Serial.print("V1 Slider value is: ");
  Serial.println(pinValue);
  ledValue = map(pinValue, 0, 100, 0, 31);
  ledcAnalogWrite(LEDC_CHANNEL_0, ledState[ledValue]);
}
//Temerature
BLYNK_READ(V2) 
{ 
  byte tempMostAccurate =  (byte)round(bme280i2c.Read_Temperature());
  char temp_c[10], hum_c[10], pres_c[10];
  sprintf(temp_c, "%2d", tempMostAccurate);
  Serial.println(temp_c);
  Blynk.virtualWrite(V2, temp_c);
}

//Humidity 
BLYNK_READ(V3) 
{ 
  byte humidityMostAccurate = (byte)round(bme280i2c.Read_Humidity());
  char temp_c[10], hum_c[10], pres_c[10];
  sprintf(hum_c, "%2d", humidityMostAccurate);
  Serial.println(hum_c);
  Blynk.virtualWrite(V3, hum_c);
}

//Pressure 
BLYNK_READ(V4) 
{ 
  uint16_t pressureMostAccurate = (uint16_t)round(bme280i2c.Read_Pressure());
  char temp_c[10], hum_c[10], pres_c[10];
  sprintf(pres_c, "%4d", pressureMostAccurate);
  Serial.println(pres_c);
  Blynk.virtualWrite(V4, pres_c);
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
  uint8_t t_sb = 5; //stanby 1000ms
  uint8_t filter = 0; //filter O = off
  uint8_t osrs_t = 4; //OverSampling Temperature x4
  uint8_t osrs_p = 4; //OverSampling Pressure x4
  uint8_t osrs_h = 4; //OverSampling Humidity x4
  uint8_t Mode = 3; //Normal mode
  bme280i2c.ESP32_BME280_I2C_Init(t_sb, filter, osrs_t, osrs_p, osrs_h, Mode);
  delay(1000);
}

void loop()
{
  Blynk.run();
  
}

