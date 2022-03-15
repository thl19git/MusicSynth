#include <U8g2lib.h>
#include <ES_CAN.h>
#include "knob.h"
#include "sound.h"
#include "joystick.h"
#include "main.h"

// Key Array
volatile uint32_t keyArray[7];

// Wave types
std::string waveType[] = {"Saw", "Sin", "Sqr", "Tri"};

// Mutex
SemaphoreHandle_t keyArrayMutex;
SemaphoreHandle_t CAN_TX_Semaphore;

volatile int32_t currentStepSize;

// CAN network
uint8_t RX_Message[8] = {0};
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;
volatile uint8_t receiver = 1;

// Knobs
Knob knob0(0, 0, 10); // Rotation: Echo || Button: Sound wave
Knob knob1(1);
Knob knob2(2, 1, 7);  // Rotation: Octave || Button: Tx/Rx
Knob knob3(3, 0, 16); // Volume

// Joystick
Joystick joystick;

// Sound Gen
SoundGenerator soundGen;

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

/* ####################### */
/* ###### Interupts ###### */
/* ####################### */

void CAN_RX_ISR(void)
/*
 * TODO
 */
{
  uint8_t RX_MESSAGE_ISR[8];
  uint32_t ID;
  CAN_RX(ID, RX_MESSAGE_ISR);
  xQueueSendFromISR(msgInQ, RX_MESSAGE_ISR, NULL);
}

void CAN_TX_ISR(void)
/*
 * TODO
 */
{
  xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

void sampleISR()
/*
 * Function that gets called by an interrupt
 * Updates phase accumulator, sets correct volume and sets the analogue output voltage at each sample interval
 */
{

  int32_t Vout = soundGen.getVout();

  // Setting volume
  Vout = Vout >> (8 - knob3.getRotation() / 2);

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
  const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  volatile uint32_t localKeyArray[7];
  uint8_t prevKnob2Button = 1;
  uint8_t prevKnob0Button = 1;
  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    for (uint8_t i = 0; i < 4; i++)
    {
      setRow(i);
      delayMicroseconds(3);
      uint8_t keys = readCols();
      localKeyArray[i] = keys;
    }
    uint8_t localReceiver = __atomic_load_n(&receiver, __ATOMIC_RELAXED);

    xSemaphoreTake(keyArrayMutex, portMAX_DELAY);

    if (localReceiver)
    {
      uint8_t octave = knob2.getRotation();
      for (uint8_t i = 0; i < 3; i++)
      {
        uint8_t keys = localKeyArray[i];
        uint8_t oldKeys = keyArray[i];
        for (uint8_t j = 0; j < 4; j++)
        {
          uint8_t mask = 1 << j;
          if ((keys & mask) ^ (oldKeys & mask))
          {
            if (keys & mask)
            {
              // Key has been released
              if (localReceiver)
              {
                // soundGen.removeKey(octave, i * 4 + j);
                soundGen.echoKey(octave, i * 4 + j);
              }
              else
              {
                uint8_t TX_Message[8];
                TX_Message[0] = 'R';
                TX_Message[1] = knob2.getRotation();
                TX_Message[2] = i * 4 + j;
                xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
              }
            }
            else
            {
              // Key has been pressed
              if (localReceiver)
              {
                soundGen.addKey(octave, i * 4 + j);
              }
              else
              {
                uint8_t TX_Message[8];
                TX_Message[0] = 'P';
                TX_Message[1] = knob2.getRotation();
                TX_Message[2] = i * 4 + j;
                xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
              }
            }
          }
        }
      }
    }

    xSemaphoreGive(keyArrayMutex);

    cpyKeyArray(localKeyArray);

    // Only update the volume and sound wave if the module is configured to be a receiver
    if (localReceiver)
    {
      knob3.updateRotationValue();
      knob3.updateButtonValue();

      knob0.updateRotationValue();
      soundGen.setGlobalLifeTime(knob0.getRotation());
      knob0.updateButtonValue();
    }

    // Update the octave - user guidance: don't change the octave whilst keys are being pressed!!
    knob2.updateRotationValue();
    knob2.updateButtonValue();

    uint8_t knob2Button = knob2.getButton();

    // Check to see if knob2 (Tx/Rx) has been pressed (i.e. gone from 1 -> 0)
    if (!knob2Button && prevKnob2Button)
    {
      uint8_t localReceiver = __atomic_load_n(&receiver, __ATOMIC_RELAXED);
      if (localReceiver)
      {
        __atomic_store_n(&receiver, 0, __ATOMIC_RELAXED);
      }
      else
      {
        __atomic_store_n(&receiver, 1, __ATOMIC_RELAXED);
      }
    }
    prevKnob2Button = knob2Button;

    uint8_t knob0Button = knob0.getButton();

    // Check to see if knob0 (wave_type) has been pressed (i.e. gone from 1 -> 0)
    if (!knob0Button && prevKnob0Button)
    {
      uint8_t localSoundWave = soundGen.getWaveform();
      if (localSoundWave == 3)
      {
        localSoundWave = 0;
      }
      else
      {
        localSoundWave += 1;
      }
      soundGen.setWaveform(localSoundWave);
    }
    prevKnob0Button = knob0Button;
  }
}

void joystickTask(void *pvParameters)
/*
 * Function to be run on its own thread that:
 *   updates joystick global variables ,
 *   for now: serial prints results
 *
 * :param pvParameters: Thread parameter information
 */
{
  // Initiation interval
  const TickType_t xFrequency = 30 / portTICK_PERIOD_MS;

  // Tick count of last initiation
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    // Updating joystick data
    joystick.updateJoystickPosition();

    // Updating joystick button data
    joystick.updateJoystickButton();

    // for testing
    // double shift = sin(joystick.getX());
    // Serial.println(joystick.getX());
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
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

    u8g2.setCursor(2, 10);
    u8g2.print("Oct: ");
    u8g2.print(knob2.getRotation());

    uint8_t localReceiver = __atomic_load_n(&receiver, __ATOMIC_RELAXED);

    if (localReceiver)
    {
      u8g2.setCursor(110, 10);
      u8g2.print("Rx");

      u8g2.setCursor(60, 20);
      u8g2.print("Vol: ");
      u8g2.print(knob3.getRotation());
      
      u8g2.setCursor(42, 10);
      u8g2.print("Wave: ");
      u8g2.print(waveType[soundGen.getWaveform()].c_str());

      u8g2.setCursor(2, 20);
      u8g2.print("Echo: ");
      u8g2.print(knob0.getRotation());
      u8g2.print("s");

      u8g2.setCursor(2, 30);
      u8g2.print(soundGen.getCurrentNotes().c_str());
    }
    else
    {
      u8g2.setCursor(110, 10);
      u8g2.print("Tx");
    }

    u8g2.sendBuffer(); // transfer internal memory to the display

    // Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}

void CAN_TX_Task(void *pvParameters)
{
  uint8_t msgOut[8];
  while (1)
  {
    xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
    xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
    CAN_TX(0x123, msgOut);
  }
}

void decodeTask(void *pvParameters)
{
  while (1)
  {
    xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
    uint8_t localReceiver = __atomic_load_n(&receiver, __ATOMIC_RELAXED);
    if (localReceiver)
    {
      uint8_t action = RX_Message[0];
      uint8_t octave = RX_Message[1];
      uint8_t note = RX_Message[2];
      if (action == 0x50)
      {
        // Press
        soundGen.addKey(octave, note);
      }
      else
      {
        // Release
        // soundGen.removeKey(octave, note);
        soundGen.echoKey(octave, note);
      }
    }
  }
}

/* --- setup and loop ---*/

void setup()
{
  // put your setup code here, to run once:

  keyArrayMutex = xSemaphoreCreateMutex();
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3, 3);

  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);

  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
      scanKeysTask,     /* Function that implements the task */
      "scanKeys",       /* Text name for the task */
      64,               /* Stack size in words, not bytes */
      NULL,             /* Parameter passed into the task */
      4,                /* Task priority */
      &scanKeysHandle); /* Pointer to store the task handle */

  TaskHandle_t joystickHandle = NULL;
  xTaskCreate(
      joystickTask,     /* Function that implements the task */
      "joystick",       /* Text name for the task */
      64,               /* Stack size in words, not bytes */
      NULL,             /* Parameter passed into the task */
      2,                /* Task priority */
      &joystickHandle); /* Pointer to store the task handle */

  TaskHandle_t displayUpdateTaskHandle = NULL;
  xTaskCreate(
      displayUpdateTask,         /* Function that implements the task */
      "displayUpdate",           /* Text name for the task */
      256,                       /* Stack size in words, not bytes */
      NULL,                      /* Parameter passed into the task */
      1,                         /* Task priority */
      &displayUpdateTaskHandle); /* Pointer to store the task handle */

  TaskHandle_t decodeTaskHandle = NULL;
  xTaskCreate(
      decodeTask,         /* Function that implements the task */
      "decode",           /* Text name for the task */
      64,                 /* Stack size in words, not bytes */
      NULL,               /* Parameter passed into the task */
      3,                  /* Task priority */
      &decodeTaskHandle); /* Pointer to store the task handle */

  TaskHandle_t CAN_TX_TaskHandle = NULL;
  xTaskCreate(
      CAN_TX_Task,         /* Function that implements the task */
      "CAN_TX",            /* Text name for the task */
      256,                 /* Stack size in words, not bytes */
      NULL,                /* Parameter passed into the task */
      2,                   /* Task priority */
      &CAN_TX_TaskHandle); /* Pointer to store the task handle */

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

  msgInQ = xQueueCreate(36, 8);
  msgOutQ = xQueueCreate(36, 8);

  CAN_Init(false);
  setCANFilter(0x123, 0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);

  CAN_Start();

  //Set the initial volume and octave
  knob2.setRotation(4);
  knob3.setRotation(8);

  vTaskStartScheduler();
}

// empty loop function
void loop()
{
}
