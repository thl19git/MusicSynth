#include <iostream>

#ifndef KNOB_H
#define KNOB_H

class Knob
{

public:
    // row to access for knob rotation
    uint8_t rotationRow;
    int8_t lowerLimit;
    int8_t upperLimit;
    uint8_t prevBit0 = 0;
    uint8_t prevBit1 = 0;
    int8_t knobChange = 0;

    int aAddr;
    int bAddr;
    volatile int8_t knobRotation = 0;

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
     * Atomically loads the global knobRotation to prevent synchronisation erros
     *
     * :return: current value of global knobRotation
     */
};

#endif
