#include <iostream>

#ifndef KNOB_H
#define KNOB_H

class Knob
{

public:
    // row to access for knob rotation
    uint8_t rotationRow;
    uint8_t buttonRow;
    int8_t lowerLimit;
    int8_t upperLimit;
    uint8_t prevBit0 = 0;
    uint8_t prevBit1 = 0;
    int8_t knobChange = 0;

    int aRotationAddr;
    int bRotationAddr;

    int buttonAddr;
    volatile int8_t knobRotation = 0;
    volatile int8_t knobButton = 0;

    Knob(uint8_t id, int8_t minVal = 0, int8_t maxVal = 16);
    /*
     * Intiliser for the knob class
     *
     * :param id: knob id from 0 to 3 (left to right)
     *
     * :param minVal: minimum value the knob should be able to produce
     *
     * :param maxVal: maximum value the knob should be able to produce
     */

    int8_t calculateAndAssignval(uint8_t prevBit0, uint8_t prevBit1, uint8_t bit0, uint8_t bit1, int8_t prevStep);
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

    void updateRotationValue();
    /*
     * Obtains readings from knob and updates global knob rotation variable
     */

    int getRotation();
    /*
     * Atomically loads the global knobRotation to prevent synchronisation errors
     *
     * :return: current value of global knobRotation
     */

    void setRotation(int8_t rotation);
    /*
     * Atomically stores the global knobRotation to prevent synchronisation errors
     */

    void updateButtonValue();
    /*
     * Obtains readings from knob button and updates global knob button variable
     */

    int getButton();
    /*
     * Atomically loads the global knobButton to prevent synchronisation errors
     *
     * :return: current value of global knobButton, 1 is button is not pressed, 0 is button is pressed
     */
};

#endif
