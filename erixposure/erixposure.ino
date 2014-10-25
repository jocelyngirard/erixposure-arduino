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
 ** Reference http://en.wikipedia.org/wiki/Light_meter#Exposure_meter_calibration
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

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

#include "erixposure.h"

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_TSL2591 luxMeter = Adafruit_TSL2591(2591);

int apertureIndex;
int isoIndex;
int lightMeteringType;

void setup()
{
  pinMode(INC_BUTTON_PIN, INPUT_PULLUP); //Set up pin modes and start serial for debugging
  pinMode(DEC_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ACTION_BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC);
  { // To remove AdaFruit logo 
    display.display();
    display.clearDisplay();
  }

  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //luxMeter.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  luxMeter.setGain(TSL2591_GAIN_MED);      // 25x gain
  //luxMeter.setGain(TSL2591_GAIN_HIGH);   // 428x gain

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  luxMeter.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  //luxMeter.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  //luxMeter.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  //luxMeter.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  //luxMeter.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  //luxMeter.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  checkBatteryVoltage();
}

void loop()
{
  handleButtons();
  // computeShutterSpeedAndDisplay();
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

void readEepromValues()
{
  apertureIndex = EEPROM.read(APERTURE_MEMORY_ADDR);
  if (apertureIndex >= length(apertures))
  {
    apertureIndex = 4;
  }
  isoIndex = EEPROM.read(ISO_MEMORY_ADDR);
  if (isoIndex >= length(isos))
  {
    isoIndex = 2;
  }
  lightMeteringType = EEPROM.read(LIGHT_TYPE_MEMORY_ADDR);
  if (lightMeteringType == 255)
  {
    lightMeteringType = REFLECTED_METERING;
  } 
}

void computeShutterSpeedAndDisplay()
{
  readEepromValues();

  float luxValue = getLuxValue();
  int shutterSpeedIndex = 0;
  boolean isPositive;
  float exposureValue = log(luxValue / 2.5) / log(2);

  float aperture = apertures[apertureIndex];

  display.setTextSize(1);
  display.setCursor(6 * BIG_CHAR_WIDTH + (CHAR_WIDTH / 2) , CHAR_HEIGHT / 2);
  display.setTextColor(WHITE, BLACK);
  float exposureTimeInSeconds;
  switch (lightMeteringType)
  {
  case INCIDENT_METERING:
    exposureTimeInSeconds = pow(aperture, 2) / (luxValue / (INCIDENT_CALIBRATION_CONSTANT / isos[isoIndex]));
    display.write('I');
    break;

  default:
  case REFLECTED_METERING:
    exposureTimeInSeconds = (pow(aperture, 2) * REFLECTED_CALIBRATION_CONSTANT) / (luxValue * isos[isoIndex]);
    display.write('R');
    break;
  }

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("f/");
  display.println(aperture, aperture == (int) aperture ? 0 : 1);

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
      float speedGap = exposureTimeInSeconds - shutterSpeed;
      float absSpeedGap = fabs(speedGap);
      if (absSpeedGap < shortestSpeedGap)
      {
        shortestSpeedGap = absSpeedGap;
        shutterSpeedIndex = index;
        isPositive = speedGap < 0;
      }
    }
    display.println(shutterSpeedTexts[shutterSpeedIndex]);
    display.setTextSize(1);
    display.setCursor(6 * BIG_CHAR_WIDTH + (CHAR_WIDTH / 2) , 1 * BIG_CHAR_WIDTH + CHAR_HEIGHT + 1);
    display.setTextColor(WHITE, BLACK);
    display.write(isPositive ? 24 : 25);
  }

  // We draw the separator
  display.drawLine(73, 0, 73, 32, WHITE);

  // We display the ISO value
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(76, 0);
  display.print(isos[isoIndex], 0);
  display.println(" ISO");

  // We display the exposure value
  display.setCursor(76,11);
  display.print(exposureValue, 1);
  display.println(" Ev");

  // We display the Lux value
  display.setCursor(76, 22);
  display.print(luxValue, 0);
  display.println(" Lx");

  display.display();
  display.clearDisplay();
  delay(500);
}

// TODO : Refactor the buttons management to use the 'Surface Mount Navigation Switch Breakout'
// (https://www.sparkfun.com/products/8236)
void handleButtons()
{  
  boolean incrementButton = digitalRead(INC_BUTTON_PIN);
  boolean decrementButton = digitalRead(DEC_BUTTON_PIN);
  boolean meteringButton = digitalRead(ACTION_BUTTON_PIN);

  if (meteringButton == LOW)
  {
    computeShutterSpeedAndDisplay();
  }
  else if (incrementButton == LOW)
  {
    if (apertureIndex >= (sizeof(apertures) / sizeof(float) - 1))
    {
      apertureIndex = 0;
    }
    else
    {
      apertureIndex += 1;
    }
    EEPROM.write(APERTURE_MEMORY_ADDR, apertureIndex);
    computeShutterSpeedAndDisplay();
  }
  else if (decrementButton == LOW)
  {
    if (isoIndex >= (sizeof(isos) / sizeof(float) - 1))
    {
      isoIndex = 0;
    }
    else
    {
      isoIndex += 1;
    }
    EEPROM.write(ISO_MEMORY_ADDR, isoIndex);
    computeShutterSpeedAndDisplay();
  } 
}




