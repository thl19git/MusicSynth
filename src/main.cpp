#include <U8g2lib.h>
#include "knob.h"
#include "main.h"

// Key Array
volatile uint32_t keyArray[7];

// Notes
std::string notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "nan"};
std::string note; // should be volatile

// Step sizes
const int32_t stepSizes[] = {51076056, 54113197, 57330935, 60740010, 64351798, 68178356, 72232452, 76527617, 81078186, 85899345, 91007186, 96418755};
/* const int32_t stepSizes[] = {51076056.67, 54113197.05, 57330935.19, 60740010, 64351798.95, 68178356.04, 72232452.06, 76527617.17, 81078186.09, 85899345.92, 91007186.83, 96418755.77};*/
volatile int32_t currentStepSize;

// Mutex
SemaphoreHandle_t keyArrayMutex;

// Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value)
{
  digitalWrite(REN_PIN, LOW);
  digitalWrite(RA0_PIN, bitIdx & 0x01);
  digitalWrite(RA1_PIN, bitIdx & 0x02);
  digitalWrite(RA2_PIN, bitIdx & 0x04);
  digitalWrite(OUT_PIN, value);
  digitalWrite(REN_PIN, HIGH);
  delayMicroseconds(2);
  digitalWrite(REN_PIN, LOW);
}

/* --- my code --- */

uint8_t readCols()
/*
 * Reads from all columns and returns a single integer value of all bits in form 0b bit0 bit1 bit2 bit3
 *
 * :return: single integer value of columns
 */
{
  uint8_t bit0;
  uint8_t bit1;
  uint8_t bit2;
  uint8_t bit3;
  bit0 = digitalRead(C0_PIN);
  bit1 = digitalRead(C1_PIN);
  bit2 = digitalRead(C2_PIN);
  bit3 = digitalRead(C3_PIN);

  return bit0 + (bit1 << 1) + (bit2 << 2) + (bit3 << 3);
}

void setRow(uint8_t rowIdx)
/*
 * Writes to mux to select certain row that can then be read from using readCols()
 *
 * :param rowIdx: index of row to be read from
 */
{
  // Disabling row select
  digitalWrite(REN_PIN, LOW);

  // Writing to pins
  digitalWrite(RA0_PIN, (rowIdx & 0x01));
  digitalWrite(RA1_PIN, (rowIdx & 0x02) >> 1);
  digitalWrite(RA2_PIN, (rowIdx & 0x04) >> 2);

  // Enabling row select
  digitalWrite(REN_PIN, HIGH);
}

uint8_t getIndx(uint8_t key)
/*
 * Given a specific key value from readCols() it will return a indx value for accessing stepSizes array
 *
 * :param key: discrete integer value refereeing to a key from a specific row
 *
 * :return: discrete integer value refereeing to part of an index into the stepSizes array
 */
{
  switch (key)
  {
  case 0xE:
    return 0;
  case 0xD:
    return 1;
  case 0xB:
    return 2;
  case 0x7:
    return 3;
  }
  return 4;
}

void cpyKeyArray(volatile uint32_t localKeyArray[7])
/*
 * Copies a local array of key values to the global keyArray using a mutex to prevent any synchronisation bug
 *
 * :param localKeyArray: array locally created in the scanKeysTask used to temporarlity store key values
 */
{
  xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
  keyArray[0] = localKeyArray[0];
  keyArray[1] = localKeyArray[1];
  keyArray[2] = localKeyArray[2];
  /*keyArray[3] += localKeyArray[3];
  keyArray[4] = localKeyArray[4];
  keyArray[5] = localKeyArray[5];
  keyArray[6] = localKeyArray[6]; */
  xSemaphoreGive(keyArrayMutex);
}

Knob knob3(3);
Knob knob1(1);
Knob knob2(2);
Knob knob0(0);

void sampleISR()
/*
 * Function that gets called by an interrupt
 * Updates phase accumulator, sets correct volume and sets the analogue output voltage at each sample interval
 */
{
  static int32_t phaseAcc = 0;

  // Updating phase accumulator
  phaseAcc += currentStepSize;
  int32_t Vout = phaseAcc >> 24;

  // Setting volume TODO: adjust volume limits using knob3.max and min
  Vout = Vout >> (8 - knob3.knobRotation / 2);

  // seting analogue output voltage
  analogWrite(OUTR_PIN, Vout + 128);
}

void scanKeysTask(void *pvParameters)
/*
 * Function to be run on its own thread that:
 *   reads from all keys and knobs,
 *   stores relevant data in global variables
 *   produces correct note at the correct volume
 *
 * :param pvParameters: Thread parameter information
 */
{
  // Initiation interval
  const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;

  // Tick count of last initiation
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Local variables for temporary storage
  volatile uint32_t localKeyArray[7];
  static uint32_t localCurrentStepSize;
  uint32_t indx;

  while (1)
  {
    // Blocking call
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    // Looping through keys
    for (uint8_t i = 0; i < 3; i++)
    {
      setRow(i);
      delayMicroseconds(3);
      uint8_t keys = readCols();
      localKeyArray[i] = keys;
      if (keys != 0xF)
      {
        indx = (i * 4) + getIndx(keys);
        localCurrentStepSize = stepSizes[indx];
        note = notes[indx];
        break;
      }
      else
      {
        localCurrentStepSize = 0;
        note = "";
      }
    }

    // atomically storing step size
    __atomic_store_n(&currentStepSize, localCurrentStepSize, __ATOMIC_RELAXED);

    // updating knob values, synchronisation issues delt with within class funciton
    knob3.updateRotationValue();
    knob1.updateRotationValue();
    knob0.updateRotationValue();
    knob2.updateRotationValue();

    // updating global key values
    cpyKeyArray(localKeyArray);
  }
}

void displayUpdateTask(void *pvParameters)
/*
 * Function to be run on its own thread that:
 *   reads from global variabeles,
 *   prints relevant data on screen
 *
 * :param pvParameters: Thread parameter information
 */
{
  // Initiation interval
  const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

  // Tick count of last initiation
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Local variables for temporary storage
  volatile uint32_t localKeyArray[7];

  while (1)
  {
    static uint32_t next = millis();
    static uint32_t count = 0;
    // scanKeysTask(NULL);

    next += interval;

    xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
    localKeyArray[0] = keyArray[0];
    localKeyArray[1] = keyArray[1];
    localKeyArray[2] = keyArray[2];
    localKeyArray[3] = keyArray[3];
    /*localKeyArray[4] = keyArray[4];
    localKeyArray[5] = keyArray[5];
    localKeyArray[6] = keyArray[6];*/
    xSemaphoreGive(keyArrayMutex);

    // Update display
    u8g2.clearBuffer();                   // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);   // choose a suitable font
    u8g2.drawStr(2, 10, "Helllo World!"); // write something to the internal memory
    u8g2.setCursor(2, 20);
    u8g2.print(localKeyArray[0], HEX);
    u8g2.print(localKeyArray[1], HEX);
    u8g2.print(localKeyArray[2], HEX);
    u8g2.print(knob3.getRotation());
    u8g2.print(knob2.getRotation());
    u8g2.print(knob1.getRotation());
    u8g2.print(knob0.getRotation());
    u8g2.drawStr(2, 30, note.c_str()); // write something to the internal memory

    u8g2.sendBuffer(); // transfer internal memory to the display

    // Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}

/* --- setup and loop ---*/

void setup()
{
  ////// put your setup code here, to run once: //////

  // Setting up Mutex
  keyArrayMutex = xSemaphoreCreateMutex();

  // Timer to trigger interupt that will call sampleISR()
  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);

  // Thread initilisation for scanning and updating keys and knobs
  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
      scanKeysTask,     /* Function that implements the task */
      "scanKeys",       /* Text name for the task */
      64,               /* Stack size in words, not bytes */
      NULL,             /* Parameter passed into the task */
      2,                /* Task priority */
      &scanKeysHandle); /* Pointer to store the task handle */

  // Thread initilisation for updating the screen info
  TaskHandle_t displayUpdateTaskHandle = NULL;
  xTaskCreate(
      displayUpdateTask,         /* Function that implements the task */
      "displayUpdate",           /* Text name for the task */
      256,                       /* Stack size in words, not bytes */
      NULL,                      /* Parameter passed into the task */
      1,                         /* Task priority */
      &displayUpdateTaskHandle); /* Pointer to store the task handle */

  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();
  // Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  // Initialise display
  setOutMuxBit(DRST_BIT, LOW); // Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH); // Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH); // Enable display power supply

  // Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  // starting scheduler
  vTaskStartScheduler();
}

// empty loop function
void loop()
{
}
