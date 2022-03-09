#include <iostream>
#include <Arduino.h>
#include <string>
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>

#ifndef MAIN_H
#define MAIN_H

// Constants
const uint32_t interval = 100; // Display update interval

// Pin definitions
// Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

// Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

// Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

// Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

// Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

void setOutMuxBit(const uint8_t bitIdx, const bool value);

uint8_t readCols();
/*
 * Reads from all columns and returns a single integer value of all bits in form 0b bit0 bit1 bit2 bit3
 *
 * :return: single integer value of columns
 */

void setRow(uint8_t rowIdx);
/*
 * Writes to mux to select certain row that can then be read from using readCols()
 *
 * :param rowIdx: index of row to be read from
 */

uint8_t getIndx(uint8_t key);
/*
 * Given a specific key value from readCols() it will return a indx value for accessing stepSizes array
 *
 * :param key: discrete integer value refereeing to a key from a specific row
 *
 * :return: discrete integer value refereeing to part of an index into the stepSizes array
 */

void cpyKeyArray(volatile uint32_t localKeyArray[7]);
/*
 * Copies a local array of key values to the global keyArray using a mutex to prevent any synchronisation bug
 *
 * :param localKeyArray: array locally created in the scanKeysTask used to temporarlity store key values
 */

void sampleISR();
/*
 * Function that gets called by an interrupt
 * Updates phase accumulator, sets correct volume and sets the analogue output voltage at each sample interval
 */

void scanKeysTask(void *pvParameters);
/*
 * Function to be run on its own thread that:
 *   reads from all keys and knobs,
 *   stores relevant data in global variables
 *   produces correct note at the correct volume
 *
 * :param pvParameters: Thread parameter information
 */

void displayUpdateTask(void *pvParameters);
/*
 * Function to be run on its own thread that:
 *   reads from global variabeles,
 *   prints relevant data on screen
 *
 * :param pvParameters: Thread parameter information
 */

#endif
