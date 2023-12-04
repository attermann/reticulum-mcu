#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "config.h"

#ifdef HAS_SDCARD
#include <SD.h>
#include <FS.h>
#endif

#ifdef HAS_DISPLAY
#include <U8g2lib.h>

#ifndef DISPLAY_MODEL
#define DISPLAY_MODEL U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#endif

extern DISPLAY_MODEL *u8g2;
#endif

#ifndef OLED_WIRE_PORT
#define OLED_WIRE_PORT Wire
#endif

#if defined(HAS_PMU)
#include "XPowersLib.h"

extern XPowersLibInterface *PMU;

#ifndef PMU_WIRE_PORT
#define PMU_WIRE_PORT   Wire
#endif

extern bool pmuInterrupt;

void setPmuFlag();
bool initPMU();
void disablePeripherals();
#else
#define initPMU()
#define disablePeripherals()
#endif

//SPIClass SDSPI(HSPI);

void initBoard();
