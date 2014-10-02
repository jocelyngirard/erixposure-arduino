/*
 ** erixposure.h for EriXposure Meter
 **
 ** Made by Jocelyn GIRARD
 ** Login   <jocelyn@erioxyde.com>
 **
 ** Started on  Sun Sep 28 Jocelyn GIRARD
 ** Last update Mon Sep 29 19:03:12 2014 Jocelyn GIRARD
 */

#ifndef ERIXPOSURE.H
#define ERIXPOSURE.H

#define length(array)  (sizeof(array) / sizeof(array[0]))

#define BATTERY_PIN                  0
#define BATTERY_CELLS_COUNT          1 // The number of cells on lipo: 1 for 1S, 2 for 2S, ...
#define BATTERY_MIN_VOLTAGE_PER_CELL 3.7 // 3.7v per cell
#define BATTERY_MIN_VOLTAGE          BATTERY_CELLS_COUNT * BATTERY_MIN_VOLTAGE_PER_CELL

#define APERTURE_MEMORY_ADDR         0
#define ISO_MEMORY_ADDR              1

#define INC_BUTTON_PIN               16
#define DEC_BUTTON_PIN               10
#define ACTION_BUTTON_PIN            14

#define OLED_MOSI                    9
#define OLED_CLK                     10
#define OLED_DC                      11
#define OLED_CS                      12
#define OLED_RESET                   13

const float apertures[] = { 1.2, 1.4, 1.8, 2, 2.8, 4, 5.6, 8, 11, 16 };
const float isos[] = { 100, 200, 400, 800, 1600, 3200, 6400 };
const float shutterSpeeds[] = { 1/2.0, 1/4.0, 1/8.0, 1/15.0, 1/30.0, 1/60.0, 1/125.0, 1/250.0, 1/500.0, 1/1000.0, 1/2000.0, 1/4000.0, 1/8000.0 };
const char* shutterSpeedTexts[] = { "1/2", "1/4", "1/8", "1/15", "1/30", "1/60", "1/125", "1/250", "1/500", "1/1000", "1/2000", "1/4000", "1/8000" };

#endif

