/*
** erixposure.ino for EriXposure Meter
**
** Made by Jocelyn GIRARD
** Login   <jocelyn@erioxyde.com>
**
** Started on  Sun Sep 28 2014 Jocelyn GIRARD
** Last update Mon Sep 29 19:02:10 2014 Jocelyn GIRARD
*/

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#include "erixposure.h"

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_TSL2561_Unified luxMeter = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

TimeType timeType; //State of shutter speed value display (fractional, seconds, minutes)

boolean isoMode = 0; //ISO mode state
boolean save = 0; //Save to EEPROM state

int apertureIndex;
int isoIndex;

void setup()
{
  pinMode(INC_BUTTON_PIN, INPUT); //Set up pin modes and start serial for debugging
  pinMode(DEC_BUTTON_PIN, INPUT);
  pinMode(ACTION_BUTTON_PIN, INPUT);
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  display.clearDisplay();

  checkBatteryVoltage();

  apertureIndex = EEPROM.read(APERTURE_MEMORY_ADDR);
  isoIndex = EEPROM.read(ISO_MEMORY_ADDR);

  if (apertureIndex > (sizeof(apertures) / sizeof(float) - 1))
    {
      apertureIndex = 4;
    }
  if (isoIndex > (sizeof(isos) / sizeof(float) - 1))
    {
      isoIndex = 0;
    }

  //luxMeter.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  //luxMeter.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  luxMeter.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  //luxMeter.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);     /* fast but low resolution */
  //luxMeter.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);      /* medium resolution and speed   */
  luxMeter.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);    /* 16-bit data but slowest conversions */

  luxMeter.begin();

  refresh();
}

void loop()
{
  boolean incrementButton = digitalRead(INC_BUTTON_PIN); //Poll for button presses
  boolean decrementButton = digitalRead(DEC_BUTTON_PIN);
  boolean meteringButton = digitalRead(ACTION_BUTTON_PIN);

  /////////IF BUTTON 2 AND 3 ARE BOTH PRESSED, SAVE EEPROM DATA

  if (incrementButton == true & meteringButton == true)
    {
      save = 1;
    }
  while (incrementButton == true & meteringButton == true)
    {
      delay(100);
      incrementButton = digitalRead(INC_BUTTON_PIN);
      decrementButton = digitalRead(DEC_BUTTON_PIN);
      meteringButton = digitalRead(ACTION_BUTTON_PIN);
    }

  if (save == true) //If + increment and metering button are both pressed, save to EEPROM and display "Saved."
    {
      EEPROM.write(APERTURE_MEMORY_ADDR, apertureIndex);
      EEPROM.write(ISO_MEMORY_ADDR, isoIndex);
      Serial.print("Save = ");
      Serial.println(save);
      //display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println("Saved.");
      display.display();
      delay(500);
      display.clearDisplay();
      save = 0;
      incrementButton = 0;
      decrementButton = 0;
    }


  //READ BUTTON 1 AND INCREMENT APERTURE VALUE
  if (incrementButton == true)
    {
      if (apertureIndex >= (sizeof(apertures) / sizeof(float) - 1))
	{
	  apertureIndex = 0;
	}
      else
	{
	  apertureIndex += 1;
	}
    }
  //READ BUTTON 2 AND INCREMENT APERTURE VALUE
  if (decrementButton == true)
    {
      if (apertureIndex == 0)
	{
	  apertureIndex = (sizeof(apertures) / sizeof(float) - 1);
	}
      else
	{
	  apertureIndex -= 1;
	}
    }

  //////////IF BUTTON 1 AND 2 ARE BOTH PRESSED, ENTER ISO MODE
  if (incrementButton == true & decrementButton == true)
    {
      isoMode = 1;
    }
  while (incrementButton == true & decrementButton == true)
    {
      delay(100);
      incrementButton = digitalRead(INC_BUTTON_PIN);
      decrementButton = digitalRead(DEC_BUTTON_PIN);
    }
  while (isoMode == true)
    {
      incrementButton = digitalRead(INC_BUTTON_PIN);
      decrementButton = digitalRead(DEC_BUTTON_PIN);
      if (incrementButton == true & decrementButton == true)
	{
	  isoMode = 0;
	}

      //READ BUTTON 1 AND INCREMENT SENSITIVITY VALUE
      if (incrementButton == true)
	{
	  if (isoIndex >= (sizeof(isos) / sizeof(float) - 1))
	    {
	      isoIndex = 0;
	    }
	  else
	    {
	      isoIndex += 1;
	    }
	}
      //READ BUTTON 2 AND INCREMENT SENSITIVITY VALUE
      if (decrementButton == true)
	{
	  if (isoIndex == 0)
	    {
	      isoIndex = (sizeof(isos) / sizeof(float) - 1);
	    }
	  else
	    {
	      isoIndex -= 1;
	    }
	}
      refresh();
      //DELAYS FOR BUTTON HOLD
      while(incrementButton == true)
	{
	  delay(10);
	  incrementButton = digitalRead(INC_BUTTON_PIN);
	}
      while(decrementButton == true)
	{
	  delay(10);
	  decrementButton = digitalRead(DEC_BUTTON_PIN);
	}
      Serial.print("ISO_m = ");
      Serial.println(isoIndex);
      Serial.print("ISOmode =");
      Serial.println(isoMode);
    }

  ///////END OF ISOMODE ROUTINE/////

  //DELAYS FOR BUTTON HOLD
  while(incrementButton == true)
    {
      delay(10);
      incrementButton = digitalRead(INC_BUTTON_PIN);
    }
  while(decrementButton == true)
    {
      delay(10);
      decrementButton = digitalRead(DEC_BUTTON_PIN);
    }
  Serial.print("Aperture_m = ");
  Serial.println(apertureIndex);
  Serial.print("ISOmode = ");
  Serial.println(isoMode);
  delay(10);

  if (meteringButton == true)                        //If the metering button is pressed, get a new illuminance value.
    {
      refresh();
    }
}

void checkBatteryVoltage()
{
  float rawVoltage = analogRead(BATTERY_PIN);
  float currentVoltage = (rawVoltage * 10 / 1023);

  if (currentVoltage < BATTERY_MIN_VOLTAGE)
    {
      display.setCursor(5, 19);
      display.print("Low Battery! (");
      display.print(currentVoltage, 2);
      display.println("V)");
    }
  else
    {
      display.setCursor(8, 19);
      display.print("Battery OK! (");
      display.print(currentVoltage, 2);
      display.println("V)");
    }

  display.display();
  display.clearDisplay();
  delay(1000);
}

float getLux()
{
  sensors_event_t event;
  luxMeter.getEvent(&event);
  float lux = event.light;
  return lux;
}

void refresh()
{
  float luxValue = getLux();

  int timeInFraction;

  float exposureTimeInSeconds = pow(apertures[apertureIndex], 2) * 64 / (luxValue * isos[isoIndex]);
  if (exposureTimeInSeconds >= 60.0)
    {
      timeType = Minutes;
    }
  else if (exposureTimeInSeconds < 0.75)
    {
      timeType = Fractional;
      if (exposureTimeInSeconds < 0.000125)
	{
	  timeType = OutOfRange;
	}
      if ((exposureTimeInSeconds <= 0.000188) && (exposureTimeInSeconds > 0.000125))
	{
	  timeInFraction = 8000;
	}
      if ((exposureTimeInSeconds <= 0.000375) && (exposureTimeInSeconds > 0.000188))
	{
	  timeInFraction = 4000;
	}
      if ((exposureTimeInSeconds <= 0.00075) && (exposureTimeInSeconds > 0.000375))
	{
	  timeInFraction = 2000;
	}
      if ((exposureTimeInSeconds <= 0.0015) && (exposureTimeInSeconds > 0.00075))
	{
	  timeInFraction = 1000;
	}
      if ((exposureTimeInSeconds <= 0.003) && (exposureTimeInSeconds > 0.0015))
	{
	  timeInFraction = 500;
	}
      if ((exposureTimeInSeconds <= 0.006) && (exposureTimeInSeconds > 0.003))
	{
	  timeInFraction = 250;
	}
      if ((exposureTimeInSeconds <= 0.012333) && (exposureTimeInSeconds > 0.006))
	{
	  timeInFraction = 125;
	}
      if ((exposureTimeInSeconds <= 0.025) && (exposureTimeInSeconds > 0.012333))
	{
	  timeInFraction = 60;
	}
      if ((exposureTimeInSeconds <= 0.05) && (exposureTimeInSeconds > 0.025))
	{
	  timeInFraction = 30;
	}
      if ((exposureTimeInSeconds <= 0.095833) && (exposureTimeInSeconds > 0.05))
	{
	  timeInFraction = 15;
	}
      if ((exposureTimeInSeconds <= 0.1875) && (exposureTimeInSeconds > 0.095833))
	{
	  timeInFraction = 8;
	}
      if ((exposureTimeInSeconds <= 0.375) && (exposureTimeInSeconds > 0.1875))
	{
	  timeInFraction = 4;
	}
      if ((exposureTimeInSeconds <= 0.75) && (exposureTimeInSeconds > 0.375))
	{
	  timeInFraction = 2;
	}
    }
  else if ((exposureTimeInSeconds >= 0.75) && (exposureTimeInSeconds < 60))
    {
      timeType = Seconds;
    }
  if (luxValue == 0) //This happens if the sensor is overloaded or senses no light.
    {
      timeType = OutOfRange;
    }

  Serial.println(timeType);

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("f/");
  display.println(apertures[apertureIndex], 1);

  if (timeType == OutOfRange)
    {
      display.println("OOR!");
    }
  else if (timeType == Minutes)
    {
      display.print(exposureTimeInSeconds / 60.0, 1);
      display.println("m");
    }
  else if (timeType == Seconds)
    {
      display.print(exposureTimeInSeconds, 1);
      display.println("s");
    }
  else if (timeType == Fractional)
    {
      display.print("1/");
      display.println(timeInFraction);
    }

  display.println(exposureTimeInSeconds, 3);

  display.drawLine(73, 0, 73, 32, WHITE);

  display.setTextSize(1);
  display.setCursor(76, 0);
  display.print("ISO");
  display.println(isos[isoIndex], 0);
  display.setCursor(76,11);
  display.print("EV=");
  float exposureValue = log(pow(apertures[apertureIndex], 2)) / log(2) + log(1 / exposureTimeInSeconds) / log(2);
  display.println(floor(exposureValue + 0.5), 1);
  display.setCursor(76, 22);
  display.print(luxValue, 1);
  display.println("Lx");

  display.display();
  display.clearDisplay();
}
