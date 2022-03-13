#include <iostream>
#include <Arduino.h>
#include <string>
#include <STM32FreeRTOS.h>
#include "main.h"
#include "knob.h"

using namespace std;

Knob::Knob(uint8_t id, int8_t minVal, int8_t maxVal)
/*
 * Intiliser for the knob class
 *
 * :param id: knob id from 0 to 3 (left to right)
 *
 * :param minVal: minimum value the knob should be able to produce
 *
 * :param maxVal: maximum value the knob should be able to produce
 */
{
    // Obtains correct row using id argument
    if ((id == 3) | (id == 2))
    {
        rotationRow = 3;
        buttonRow = 5;
    }
    else
    {
        rotationRow = 4;
        buttonRow = 6;
    }

    // Obtains pin address using id argument
    if ((id == 1) | (id == 3))
    {
        aRotationAddr = C0_PIN;
        bRotationAddr = C1_PIN;
        buttonAddr = C1_PIN;
    }
    else
    {
        aRotationAddr = C2_PIN;
        bRotationAddr = C3_PIN;
        buttonAddr = C0_PIN;
    }

    // assigns lower and upper limit
    lowerLimit = minVal * 2;
    upperLimit = maxVal * 2;
}

void Knob::updateRotationValue()
/*
 * Obtains readings from knob and updates global knob rotation variable
 */
{
    // Writing to mux to select certain row
    setRow(rotationRow);

    // Delay to give time for row to be set
    delayMicroseconds(3);

    // Reading correct pins for a given knob
    uint8_t bit0 = digitalRead(aRotationAddr);
    uint8_t bit1 = digitalRead(bRotationAddr);

    // Calculating change caused by knob read
    knobChange = calculateAndAssignval(prevBit0, prevBit1, bit0, bit1, knobChange);

    // Updating previous bits
    prevBit0 = bit0;
    prevBit1 = bit1;

    // Atomically loading global knobRotation to local variable
    int8_t localKnobRotation;
    localKnobRotation = __atomic_load_n(&knobRotation, __ATOMIC_RELAXED);

    // Updating localKnobRotation
    localKnobRotation += knobChange;
    if (localKnobRotation > upperLimit)
    {
        localKnobRotation = upperLimit;
    }
    if (localKnobRotation < lowerLimit)
    {
        localKnobRotation = lowerLimit;
    }

    // Atomically storing to global knobRotation
    __atomic_store_n(&knobRotation, localKnobRotation, __ATOMIC_RELAXED);
}

void Knob::updateButtonValue()
/*
 * Obtains readings from knob button and updates global knob button variable
 */
{
    // Writing to mux to select certain row
    setRow(buttonRow);

    // Delay to give time for row to be set
    delayMicroseconds(3);

    // Reading correct pins for a given knob
    uint8_t bit = digitalRead(buttonAddr);

    // Atomically storing to global knobRotation
    __atomic_store_n(&knobButton, bit, __ATOMIC_RELAXED);
}

int8_t Knob::calculateAndAssignval(uint8_t prevBit0, uint8_t prevBit1, uint8_t bit0, uint8_t bit1, int8_t prevStep)
/*
 * Calculates change caused by knob read
 *
 * :param prevBit0: the previous value of the 0th bit
 *
 * :param prevBit1: the previous value of the 1st bit
 *
 * :param bit0: the current value of the 0th bit
 *
 * :param bit1: the current value of the 1st bit
 *
 * :return: the change caused by the change in bits
 */
{
    if (prevBit0 == bit0 & prevBit1 == bit1)
    {
        return 0;
    }
    else if ((prevBit0 == 0) & (prevBit1 == 0))
    {
        if ((bit0 == 0) & (bit1 == 1))
        {
            return -1;
        }
        else if ((bit0 == 1) & (bit1 == 0))
        {
            return +1;
        }
    }
    else if ((prevBit0 == 0) & (prevBit1 == 1))
    {
        if ((bit0 == 0) & (bit1 == 0))
        {
            return +1;
        }
        else if ((bit0 == 1) & (bit1 == 1))
        {
            return -1;
        }
    }
    else if ((prevBit0 == 1) & (prevBit1 == 0))
    {
        if ((bit0 == 0) & (bit1 == 0))
        {
            return -1;
        }
        else if ((bit0 == 1) & (bit1 == 1))
        {
            return +1;
        }
    }
    else if ((prevBit0 == 1) & (prevBit1 == 1))
    {
        if ((bit0 == 0) & (bit1 == 1))
        {
            return +1;
        }
        else if ((bit0 == 1) & (bit1 == 0))
        {
            return -1;
        }
    }
    else
    {
        return prevStep * 2;
    }
    return 0;
}

int Knob::getRotation()
/*
 * Atomically loads the global knobRotation to prevent synchronisation erros
 *
 * :return: current value of global knobRotation
 */
{
    return __atomic_load_n(&knobRotation, __ATOMIC_RELAXED) / 2;
}

int Knob::getButton()
/*
 * Atomically loads the global knobButton to prevent synchronisation erros
 *
 * :return: current value of global knobButton, 1 is button is not pressed, 0 is button is pressed
 */
{
    return __atomic_load_n(&knobButton, __ATOMIC_RELAXED);
}