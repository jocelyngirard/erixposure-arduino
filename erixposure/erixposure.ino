/*
 ** erixposure.ino for EriXposure Meter
 **
 ** Made by Jocelyn GIRARD
 ** Login   <jocelyn@erioxyde.com>
 **
 ** Started on Sep 28 2014 by Jocelyn GIRARD
 **
 ** Project started from the base initiated by Kevin Kadooka 
 ** http://kadookacameraworks.com/light.html
 **
 ** Reference en.wikipedia.org/wiki/Exposure_value
 ** 
 ** EV : Exposure Value
 ** Lux : Illuminance
 ** A : Aperture (F-number)
 ** t : Exposure time (Shutter speed)
 ** 
 ** Lux = (250 / ISO) * 2^EV
 ** 2^EV = Lux / (250 / ISO)
 ** EV = log2(Lux / 2.5) = log(Lux / 2.5) / log(2)
 ** 
 ** 2^EV = A^2 / t
 ** t = A^2 / 2^EV = A^2 / (Lux / (250 / ISO))
 ** N = sqrt(2^EV * t) = sqrt((Lux / (250 / ISO)) * t)
 */

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#include "erixposure.h"

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_TSL2561_Unified luxMeter = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

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
  { // To remove AdaFruit logo 
    display.display();
    display.clearDisplay();
  }
  apertureIndex = EEPROM.read(APERTURE_MEMORY_ADDR);
  isoIndex = EEPROM.read(ISO_MEMORY_ADDR);

  if (apertureIndex >= length(apertures))
  {
    apertureIndex = 4;
  }
  if (isoIndex >= length(isos))
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

  checkBatteryVoltage();
}

void loop()
{
  handleButtons();
  computeShutterSpeedAndDisplay();
}

// Need to be refactor, the voltage management is wrong
void checkBatteryVoltage()
{
  float rawVoltage = analogRead(BATTERY_PIN);
  float currentVoltage = (rawVoltage * 10 / 1023);

  if (currentVoltage < BATTERY_MIN_VOLTAGE)
  {
    display.setCursor(5, 19);
    display.setTextColor(WHITE);
    display.print("Low Battery! (");
    display.print(currentVoltage, 2);
    display.println("V)");
  }
  else
  {
    display.setCursor(8, 19);
    display.setTextColor(WHITE);
    display.print("Battery OK! (");
    display.print(currentVoltage, 2);
    display.println("V)");
  }

  display.drawRoundRect(2,2,124,28,3,WHITE);

  display.display();
  display.clearDisplay();
  delay(1000);
}

float getLuxValue()
{
  sensors_event_t event;
  luxMeter.getEvent(&event);
  float lux = event.light;
  return lux;
}

void computeShutterSpeedAndDisplay()
{
  float luxValue = getLuxValue();
  int shutterSpeedIndex = 0;
  float exposureValue = log(luxValue / 2.5) / log(2);
  float exposureTimeInSeconds = pow(apertures[apertureIndex], 2) / (luxValue / (250 / isos[isoIndex]));
  
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("f/");
  display.println(apertures[apertureIndex], 1);
  
  // Out of Range management, this happens if the sensor is overloaded or senses no light.
  if (luxValue == 0) 
  {
    display.println("OOR!");
  }
  
  // Minutes management
  else if (exposureTimeInSeconds >= 60.0)
  {
    display.print(exposureTimeInSeconds / 60.0, 1);
    display.println("m");
  }
  
  // Seconds management 
  else if ((exposureTimeInSeconds >= 0.75) && (exposureTimeInSeconds < 60))
  {
    display.print(exposureTimeInSeconds, 1);
    display.println("s");
  }
  
  // Fractional time management, we find the nearest standard shutter speed linked to exposure time value
  else if (exposureTimeInSeconds < 0.75)
  {
    float shortestSpeedGap = 1.0;
    for (int index = 0; index < length(shutterSpeeds); index++)
    {
      float shutterSpeed = shutterSpeeds[index];
      float speedGap = fabs(exposureTimeInSeconds - shutterSpeed);
      if (speedGap < shortestSpeedGap)
      {
        shortestSpeedGap = speedGap;
        shutterSpeedIndex = index;
      }
    }
    display.println(shutterSpeedTexts[shutterSpeedIndex]);
  }

  // We draw the separator
  display.drawLine(73, 0, 73, 32, WHITE);

  // We display the ISO value
  display.setTextSize(1);
  display.setCursor(76, 0);
  display.print("ISO");
  display.println(isos[isoIndex], 0);
  
  // We display the exposure value
  display.setCursor(76,11);
  display.print("EV=");
  display.println(exposureValue, 1);
  
  // We display the Lux value
  display.setCursor(76, 22);
  display.print(luxValue, 1);
  display.println("Lx");

  display.display();
  display.clearDisplay();
  delay(500);
}

// TODO : Refactor the buttons management to use the 'Surface Mount Navigation Switch Breakout'
// (https://www.sparkfun.com/products/8236)
void handleButtons()
{
  //  boolean incrementButton = digitalRead(INC_BUTTON_PIN); //Poll for button presses
  //  boolean decrementButton = digitalRead(DEC_BUTTON_PIN);
  //  boolean meteringButton = digitalRead(ACTION_BUTTON_PIN);
  //
  //  /////////IF BUTTON 2 AND 3 ARE BOTH PRESSED, SAVE EEPROM DATA
  //
  //  if (incrementButton == true & meteringButton == true)
  //    {
  //      save = 1;
  //    }
  //  while (incrementButton == true & meteringButton == true)
  //    {
  //      delay(100);
  //      incrementButton = digitalRead(INC_BUTTON_PIN);
  //      decrementButton = digitalRead(DEC_BUTTON_PIN);
  //      meteringButton = digitalRead(ACTION_BUTTON_PIN);
  //    }
  //
  //  if (save == true) //If + increment and metering button are both pressed, save to EEPROM and display "Saved."
  //    {
  //      EEPROM.write(APERTURE_MEMORY_ADDR, apertureIndex);
  //      EEPROM.write(ISO_MEMORY_ADDR, isoIndex);
  //      Serial.print("Save = ");
  //      Serial.println(save);
  //      //display.clearDisplay();
  //      display.setTextSize(2);
  //      display.setTextColor(WHITE);
  //      display.setCursor(0,0);
  //      display.println("Saved.");
  //      display.display();
  //      delay(500);
  //      display.clearDisplay();
  //      save = 0;
  //      incrementButton = 0;
  //      decrementButton = 0;
  //    }
  //
  //
  //  //READ BUTTON 1 AND INCREMENT APERTURE VALUE
  //  if (incrementButton == true)
  //    {
  //      if (apertureIndex >= (sizeof(apertures) / sizeof(float) - 1))
  //	{
  //	  apertureIndex = 0;
  //	}
  //      else
  //	{
  //	  apertureIndex += 1;
  //	}
  //    }
  //  //READ BUTTON 2 AND INCREMENT APERTURE VALUE
  //  if (decrementButton == true)
  //    {
  //      if (apertureIndex == 0)
  //	{
  //	  apertureIndex = (sizeof(apertures) / sizeof(float) - 1);
  //	}
  //      else
  //	{
  //	  apertureIndex -= 1;
  //	}
  //    }
  //
  //  //////////IF BUTTON 1 AND 2 ARE BOTH PRESSED, ENTER ISO MODE
  //  if (incrementButton == true & decrementButton == true)
  //    {
  //      isoMode = 1;
  //    }
  //  while (incrementButton == true & decrementButton == true)
  //    {
  //      delay(100);
  //      incrementButton = digitalRead(INC_BUTTON_PIN);
  //      decrementButton = digitalRead(DEC_BUTTON_PIN);
  //    }
  //  while (isoMode == true)
  //    {
  //      incrementButton = digitalRead(INC_BUTTON_PIN);
  //      decrementButton = digitalRead(DEC_BUTTON_PIN);
  //      if (incrementButton == true & decrementButton == true)
  //	{
  //	  isoMode = 0;
  //	}
  //
  //      //READ BUTTON 1 AND INCREMENT SENSITIVITY VALUE
  //      if (incrementButton == true)
  //	{
  //	  if (isoIndex >= (sizeof(isos) / sizeof(float) - 1))
  //	    {
  //	      isoIndex = 0;
  //	    }
  //	  else
  //	    {
  //	      isoIndex += 1;
  //	    }
  //	}
  //      //READ BUTTON 2 AND INCREMENT SENSITIVITY VALUE
  //      if (decrementButton == true)
  //	{
  //	  if (isoIndex == 0)
  //	    {
  //	      isoIndex = (sizeof(isos) / sizeof(float) - 1);
  //	    }
  //	  else
  //	    {
  //	      isoIndex -= 1;
  //	    }
  //	}
  //      refresh();
  //      //DELAYS FOR BUTTON HOLD
  //      while(incrementButton == true)
  //	{
  //	  delay(10);
  //	  incrementButton = digitalRead(INC_BUTTON_PIN);
  //	}
  //      while(decrementButton == true)
  //	{
  //	  delay(10);
  //	  decrementButton = digitalRead(DEC_BUTTON_PIN);
  //	}
  //      Serial.print("ISO_m = ");
  //      Serial.println(isoIndex);
  //      Serial.print("ISOmode =");
  //      Serial.println(isoMode);
  //    }
  //
  //  ///////END OF ISOMODE ROUTINE/////
  //
  //  //DELAYS FOR BUTTON HOLD
  //  while(incrementButton == true)
  //    {
  //      delay(10);
  //      incrementButton = digitalRead(INC_BUTTON_PIN);
  //    }
  //  while(decrementButton == true)
  //    {
  //      delay(10);
  //      decrementButton = digitalRead(DEC_BUTTON_PIN);
  //    }
  //  Serial.print("Aperture_m = ");
  //  Serial.println(apertureIndex);
  //  Serial.print("ISOmode = ");
  //  Serial.println(isoMode);
  //  delay(10);
  //
  //  if (meteringButton == true)                        //If the metering button is pressed, get a new illuminance value.
  //    {
  //      refresh();
  //    } 
}



