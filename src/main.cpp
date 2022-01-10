#include <Arduino.h>
#include <SPI.h>
#include <TeensyThreads.h>
#include "HX711.h"
#include "Adafruit_MAX31855.h"
#include "SdFat.h"

#include "main.h"
#include "pin.h"
#include "coefs.h"

#define RF Serial1

SdFat sd;
SdFile file;

// HX711 load cell
HX711 load_cell;

// Thermocouples
Adafruit_MAX31855 thermo_0(MAX_CLK, MAX_CS_0, MAX_DO);
Adafruit_MAX31855 thermo_1(MAX_CLK, MAX_CS_0, MAX_DO);
Adafruit_MAX31855 thermo_2(MAX_CLK, MAX_CS_0, MAX_DO);
Adafruit_MAX31855 thermo_3(MAX_CLK, MAX_CS_0, MAX_DO);

volatile double load_cell_value = 0.0;
volatile double temp_0 = 0.0;
volatile double temp_1 = 0.0;
volatile double temp_2 = 0.0;
volatile double temp_3 = 0.0;
volatile double pressure_0 = 0.0;
volatile double pressure_1 = 0.0;

volatile int coef_te_alive = 1;

void setup() {
  Serial.begin(57600);
  RF.begin(57600);

  initSDCard();

  threads.addThread(threadSendData);
  threads.addThread(threadReadData);
  threads.addThread(threadLaunchState);
  threads.addThread(threadAlive);
  threads.addThread(threadLoadCell);
  threads.addThread(threadThermocouples);
  threads.addThread(threadPressureSensors);
}

void loop() {
  threadSDWrite();
}

void threadAlive() {
  pinMode(LED_PIN, OUTPUT);

  while(1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    threads.delay(TE_ALIVE_THREAD / coef_te_alive);
  }
}

void threadSDWrite() {
  String dataString = String(millis());
  dataString += ",";
  dataString += String(load_cell_value);
  dataString += ",";
  dataString += String(temp_0);
  dataString += ",";
  dataString += String(temp_1);
  dataString += ",";
  dataString += String(temp_2);
  dataString += ",";
  dataString += String(temp_3);
  dataString += ",";
  dataString += String(pressure_0);
  dataString += ",";
  dataString += String(pressure_1);

  file.println(dataString);

  // Serial.println(dataString);

  if (!file.sync() || file.getWriteError()) {
    Serial.println("ERROR: write error");
  }

  threads.delay(TE_SD_MS);
}

void threadSendData() {
  while(1) {
    String dataString = String(millis());
    dataString += ",";
    dataString += String(load_cell_value);
    dataString += ",";
    dataString += String(temp_0);
    dataString += ",";
    dataString += String(temp_1);
    dataString += ",";
    dataString += String(temp_2);
    dataString += ",";
    dataString += String(temp_3);
    dataString += ",";
    dataString += String(pressure_0);
    dataString += ",";
    dataString += String(pressure_1);

    RF.println(dataString);
    threads.delay(TE_RF_MS);
  }
}

void threadReadData() {
  while(1) {
    if (RF.available() > 0)
    {
      String str = RF.readStringUntil('\n').trim();

      if (str == "LAUNCH")
      {
        RF.println("ACK_LAUNCH");

        // TODO LAUNCH ACTION
        // Change a state flag
        coef_te_alive = 4;
      }
      else if (str == "ABORT")
      {
        RF.println("ACK_ABORT");

        // TODO ABORT ACTION
        // Change a state flag
        coef_te_alive = 1;
      }
    }
    threads.delay(10);
  }
}

void threadLaunchState() {
  while(1) {
    // TODO LAUNCH ACTION
    // React to state flag change
    // (Open valve and ignition)
    threads.delay(10);
  }
}

void threadLoadCell() {
  initLoadCell();

  while(1) {
    if (load_cell.is_ready()) {
      load_cell_value = load_cell.get_units(1);
    }
    threads.delay(TE_LOAD_CELL);
  }
}

void threadThermocouples() {
  initThermocouples();

  while(1) {
    temp_0 = thermo_0.readInternal() * T0_CELSIUS_J_COEF;
    temp_1 = thermo_1.readInternal() * T1_CELSIUS_J_COEF;
    temp_2 = thermo_2.readInternal() * T2_CELSIUS_N_COEF;
    temp_3 = thermo_3.readInternal() * T3_CELSIUS_N_COEF;
    threads.delay(TE_THERMOCOUPLES);
  }
}

void threadPressureSensors() {
  while(1) {
    pressure_0 = PRESSURE0_BAR_COEF * analogRead(PRESSURE_0_PIN) + PRESSURE0_BAR_OFFSET;
    pressure_1 = PRESSURE1_BAR_COEF * analogRead(PRESSURE_1_PIN) + PRESSURE1_BAR_OFFSET;
    threads.delay(TE_PRESSURE_SENSORS);
  }
}

void initLoadCell() {
  load_cell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.print("Initializing load cell... ");
  if (!load_cell.is_ready()) {
    Serial.println("ERROR.");
    while (1);
  }
  Serial.println("DONE.");
  load_cell.set_scale(1/LOADCELL_NEWTON_COEF);
  load_cell.tare();
}

void initThermocouples() {
  Serial.println("Initializing thermocouples...");
  Serial.print("THERMOCOUPLE 0... ");
  if (!thermo_0.begin()) {
    Serial.println("ERROR.");
    while (1);
  }
  Serial.println("DONE.");
  Serial.print("THERMOCOUPLE 1... ");
  if (!thermo_1.begin()) {
    Serial.println("ERROR.");
    while (1);
  }
  Serial.println("DONE.");
  Serial.print("THERMOCOUPLE 2... ");
  if (!thermo_2.begin()) {
    Serial.println("ERROR.");
    while (1);
  }
  Serial.println("DONE.");
  Serial.print("THERMOCOUPLE 3... ");
  if (!thermo_3.begin()) {
    Serial.println("ERROR.");
    while (1);
  }
  Serial.println("DONE.");
}

void initSDCard() {
  Serial.print("Initializing SD card...");
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt();
  }
  Serial.println("INITIALIZED");

  threads.delay(10);

  Serial.print("Creating file: ");
  char file_path[50];
  int file_num = 0;
  do
  {
    sprintf(file_path, "logs_%d.csv", file_num);
    file_num++;
  } while (sd.exists(file_path));

  Serial.println(file_path);

  if (!file.open(file_path, O_WRONLY | O_CREAT | O_EXCL)) {
    Serial.println("ERROR: file.open");
    return;
  }

  file.println("Time (ms),Load (N),T0 (°C),T1 (°C),T2 (°C),T3 (°C),P0 (bar),P1 (bar)");

  if (!file.sync() || file.getWriteError()) {
    Serial.println("ERROR: write error");
  }
  
  Serial.println("File created");
}