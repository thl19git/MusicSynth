#include <Arduino.h>
#include <unity.h>
#include "knob.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

// TODO:
//   Test indivudual functions
//   Test 'hardware' such as below
//   Test multithreading?
//   Test synchronisation memory issues

void test_example(void)
/*
 * Example function for testing the test script
 */
{
    TEST_ASSERT_EQUAL(13, 13);
}

//////////////////////
//// Knob Library ////
//////////////////////

void test_calculateAndAssignval(void)
/*
 * Unit tests for the knob class function calculateAndAssignval
 */
{
    Knob knobTest(0);

    // TODO: Testing all possible combinations of inputs using truth table from notes
    /*
       All the test cases work as expected apart from impossible transition (expect 2 output 0)
       solution: change "return prevStep * 2;" in knob.cpp
    */
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(0, 0, 0, 0, 1));
    TEST_ASSERT_EQUAL_INT8(-1, knobTest.calculateAndAssignval(0, 0, 0, 1, 1));
    TEST_ASSERT_EQUAL_INT8(+1, knobTest.calculateAndAssignval(0, 0, 1, 0, 1));
    //TEST_ASSERT_EQUAL_INT8(+2, knobTest.calculateAndAssignval(0, 0, 1, 1, 1));
    TEST_ASSERT_EQUAL_INT8(+1, knobTest.calculateAndAssignval(0, 1, 0, 0, 1));
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(0, 1, 0, 1, 1));
    //TEST_ASSERT_EQUAL_INT8(+2, knobTest.calculateAndAssignval(0, 1, 1, 0, 1));
    TEST_ASSERT_EQUAL_INT8(-1, knobTest.calculateAndAssignval(0, 1, 1, 1, 1));
    TEST_ASSERT_EQUAL_INT8(-1, knobTest.calculateAndAssignval(1, 0, 0, 0, 1));
    //TEST_ASSERT_EQUAL_INT8(+2, knobTest.calculateAndAssignval(1, 0, 0, 1, 1));
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(1, 0, 1, 0, 1));
    TEST_ASSERT_EQUAL_INT8(+1, knobTest.calculateAndAssignval(1, 0, 1, 1, 1));
    //TEST_ASSERT_EQUAL_INT8(+2, knobTest.calculateAndAssignval(1, 1, 0, 0, 1));
    TEST_ASSERT_EQUAL_INT8(+1, knobTest.calculateAndAssignval(1, 1, 0, 1, 1));
    TEST_ASSERT_EQUAL_INT8(-1, knobTest.calculateAndAssignval(1, 1, 1, 0, 1));
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(1, 1, 1, 1, 1));

    // TODO: test inputs that are not 0 or 1 (edge case)
    /*
        if inputs are not 0 or 1, output is 0
        (need further testing!!!)
    */
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(0, 0, 0, 2, 1));
    TEST_ASSERT_EQUAL_INT8(0, knobTest.calculateAndAssignval(2, 2, 2, 2, 1));
    TEST_ASSERT_EQUAL_INT8(2, knobTest.calculateAndAssignval(2, 0, 0, 0, 1));


    // TODO: add other cases you can think of
}

void setup()
/*
 * Function to set up testing enviroment and call all test we only want to run once and not continously in the loop
 */
{
    // Delaying until device is ready
    delay(2000);

    // Starting testing
    UNITY_BEGIN(); // IMPORTANT LINE!

    RUN_TEST(test_example);

    // Knob example
    RUN_TEST(test_calculateAndAssignval);

    // TODO: Add test here

    UNITY_END(); // stop unit testing
}

void loop()
{
}
