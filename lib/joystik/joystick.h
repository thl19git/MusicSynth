#include <iostream>

#ifndef JOYSTICK_H
#define JOYSTICK_H

class Joystick
{

    volatile int8_t button;
    volatile uint32_t x;
    volatile uint32_t y;

public:
        void updateJoystickButton();
    /*
     * Gets data from joystic pins and stores it in global variables
     */

    void updateJoystickPosition();
    /*
     * Gets data from joystic pins and stores its position in global variables x and y
     */

    uint32_t getX();
    /*
     * Atomically loads the global variable to prevent synchronisation erros
     */

    uint32_t getY();
    /*
     * Atomically loads the global variable to prevent synchronisation erros
     */

    int8_t getButton();
    /*
     * Atomically loads the global variable to prevent synchronisation erros
     */
};

#endif