#include <iostream>
#include <Arduino.h>
#include <string>
#include <STM32FreeRTOS.h>
#include "main.h"
#include "joystick.h"

using namespace std;

Joystick::Joystick()
/*
 * class constructor to set the joystick button to correct inital value
 */
{
    button = 1;
    x = 0;
    y = 0;
}

void Joystick::updateJoystickButton()
/*
 * Gets data from joystic button and stores it in a global variable
 */
{
    // Writing to mux to select row for detecting button
    setRow(5);

    // Delay to give time for row to be set
    delayMicroseconds(3);

    // Reading correct pin
    int8_t localButton = digitalRead(C2_PIN);

    // Atomically storing to global variable
    __atomic_store_n(&button, localButton, __ATOMIC_RELAXED);
}

void Joystick::updateJoystickPosition()
/*
 * Gets data from joystic pins and stores its position in global variables x and y
 */
{
    // Reading correct pins
    uint32_t localX = analogRead(JOYX_PIN);
    uint32_t localY = analogRead(JOYY_PIN);
    // Serial.println(localX);

    // Atomically storing to global variables
    __atomic_store_n(&x, localX, __ATOMIC_RELAXED);
    __atomic_store_n(&y, localY, __ATOMIC_RELAXED);
}

int8_t Joystick::getButton()
/*
 * Atomically loads the global variable to prevent synchronisation erros
 */
{
    return __atomic_load_n(&button, __ATOMIC_RELAXED);
}

uint32_t Joystick::getX()
/*
 * Atomically loads the global variable to prevent synchronisation erros
 */
{
    return __atomic_load_n(&x, __ATOMIC_RELAXED);
}

uint32_t Joystick::getY()
/*
 * Atomically loads the global variable to prevent synchronisation erros
 */
{
    return __atomic_load_n(&y, __ATOMIC_RELAXED);
}