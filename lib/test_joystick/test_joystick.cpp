#include <Arduino.h>
#include <unity.h>
#include "joystick.h"
#include "test_joystick.h"
#include "main.h"

void test_Joystick(void)
/*
 * Tests all joystick testing functions
 */
{
    RUN_TEST(test_joystickButton);
    RUN_TEST(test_joystickPosition);
}

void test_joystickButton(void)
/*
 * tests the joystick button.
 */
{
    Joystick testJoystick;
    // Testing before update, should be initilised as not on (1)
    TEST_ASSERT_EQUAL_INT8(1, testJoystick.getButton());

    // joystick should be 1 as defualt
    testJoystick.updateJoystickButton();
    TEST_ASSERT_EQUAL_INT8(1, testJoystick.getButton());
}

void test_joystickPosition(void)
/*
 * tests the joystick Position.
 */
{
    Joystick testJoystick;
    // Testing before update, x and y should be initilised to 0
    TEST_ASSERT_EQUAL_INT8(0, testJoystick.getX());
    TEST_ASSERT_EQUAL_INT8(0, testJoystick.getY());
}
