#include <Arduino.h>
#include <unity.h>
#include "joystick.h"
#include "test_joystick.h"

void test_Joystick(void)
/*
 * Tests all joystick testing functions
 */
{
    RUN_TEST(test_updateJoystickButton);
}

void test_updateJoystickButton(void)
/*
 * tests the updateJoystickButton function from the joystick class.
 */
{
    Joystick testJoystick;
    // Testing before assignment, should be initilised as not on (1)
    TEST_ASSERT_EQUAL_INT8(1, testJoystick.getButton());

    // joystick should be 1 as defualt
    testJoystick.updateJoystickButton();
    TEST_ASSERT_EQUAL_INT8(1, testJoystick.getButton());

    // testing button variable is private
    TEST_FAIL(testJoystick.button);
}

void test_updateJoystickPosition(void)
/*
 * tests the updateJoystickPosition function from the joystick class.
 */
{
}

void test_getX(void);
/*
 * tests the getX function from the joystick class.
 */

void test_getY(void);
/*
 * tests the getY function from the joystick class.
 */

void test_getButton(void);
/*
 * tests the getButton function from the joystick class.
 */
