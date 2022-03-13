#include <iostream>

#ifndef SOUND_WAVES_H
#define SOUND_WAVES_H

const uint8_t sampleFrequency = 22; // kHz

int32_t getVout();
/*
 * Checks global wave type setting and produces the correct voltage output given the wave type.
 */

int32_t sawtooth();
/*
 * Returns the correct sawtooth output for given note being played.
 *
 * :return: Vout value to be volume adjusted.
 */
int32_t sine();
/*
 * Returns the correct sine output for given note being played.
 *
 * :return: Vout value to be volume adjusted.
 */
int32_t square();
/*
 * Returns the correct square output for given note being played.
 *
 * :return: Vout value to be volume adjusted.
 */
int32_t triangular();
/*
 * Returns the correct triangular output for given note being played.
 *
 * :return: Vout value to be volume adjusted.
 */

int32_t getShift();
/*
 * Gets shift caused by movement in joystick x axis, applies shift to the current step size.
 *
 * :return: shifted step size.
 */
#endif