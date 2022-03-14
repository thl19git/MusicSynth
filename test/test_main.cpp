#include <Arduino.h>
#include <unity.h>

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

    // TODO: Add test here

    UNITY_END(); // stop unit testing
}

void loop()
{
}
