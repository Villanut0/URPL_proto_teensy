#pragma once

#include <Arduino.h>

// HX711 load cell
const int LOADCELL_DOUT_PIN = 28;
const int LOADCELL_SCK_PIN = 29;

// Thermocouples
const int MAX_DO = 12;
const int MAX_CLK = 13;
const int MAX_CS_0 = 6;
const int MAX_CS_1 = 7;
const int MAX_CS_2 = 8;
const int MAX_CS_3 = 9;

// Pressure sensors
const int PRESSURE_0_PIN = A0;
const int PRESSURE_1_PIN = A1;

// Alive LED
const int LED_PIN = 13;

// SD card
#define SD_CONFIG  SdioConfig(FIFO_SDIO)