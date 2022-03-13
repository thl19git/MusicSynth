#include "main.h"
#include "sound_waves.h"
#include "joystick.h"
#include "knob.h"

#include <math.h>

int32_t getVout()
/*
 * Checks global wave type setting and produces the correct voltage output given the wave type
 */
{
    extern Knob knob0;
    // Serial.println(knob2.getRotation());
    switch (knob0.getRotation())
    {
    case 0: // sawtooth wave
        return sawtooth();
    case 1: // sine wave
        return sine();
    case 2: // square wave
        return square();
    case 3: // traingular wave
        return triangular();
    }
    return 0;
}

int32_t sawtooth()
/*
 * Returns the correct sawtooth output for given note being played
 */
{
    extern int8_t noteIndx;
    static int32_t phaseAcc = 0;
    if (noteIndx != -1)
    {
        // Creating note shift using joystick
        int32_t shift = getShift();
        phaseAcc += shift;
    }

    return phaseAcc;
}

int32_t sine()
/*
 * Returns the correct sine output for given note being played
 */
{
    extern int8_t noteIndx;
    return 0;
    /////// TO DO: very broken, needs fixing and speeding up of computation ///////
    static uint8_t time = 0;
    static int32_t phaseAcc = 0;
    time += 1;
    int32_t x;
    if (noteIndx != 0)
    {
        x = 2 * PI * frequencies[noteIndx] * time;
    }
    else
    {
        return 0;
    }
    x = x / 22;

    // int32_t sinx = x - (pow(x, 3) / 6) + (pow(x, 5) / 120); // - (x ^ 7 / 5040);

    // phaseAcc += stepSizes[noteIndx] * sin(x);

    int32_t Vout = sin(x) * 100000; // phaseAcc >> 24;
    // Serial.println(x);
    return Vout;
}

int32_t square()
/*
 * Returns the correct square output for given note being played
 */
{
    extern int8_t noteIndx;
    return 0;
}
int32_t triangular()
/*
 * Returns the correct triangular output for given note being played
 */

{
    extern int8_t noteIndx;
    static int32_t phaseAcc = 0;
    static int8_t upOrDown = 1;

    if (phaseAcc >= 200000000)
    {
        upOrDown = -1;
    }
    else if (phaseAcc <= -2000000000)
    {
        upOrDown = +1;
    }

    if (noteIndx != -1)
    {
        int32_t shift = getShift();
        phaseAcc += (upOrDown * shift);
    }
    return phaseAcc;
}
