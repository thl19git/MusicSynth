#include <STM32FreeRTOS.h>
#include <Arduino.h>
#include "sound.h"
#include "joystick.h"
#include "knob.h"
#include "main.h"

SoundGenerator::SoundGenerator()
/*
 * Constructor for the SoundGenerator class
 */
{
  for (uint8_t i = 0; i < 12; i++)
  {
    voices[i].status = 0;
    voices[i].lifeTime = 0;
    voices[i].note = 0;
    voices[i].octave = 0;
    voices[i].phaseAcc = 0;
    voices[i].intensityRightShift = 24;
    voices[i].cyclesPerHalfPeriod = 0;
    voices[i].squareWaveCount = 0;
  }
}

void SoundGenerator::addKey(uint8_t octave, uint8_t note)
/*
 * Adds a key to the voices array, indicating the key has been pressed
 *
 * :param octave: the octave of the key (1-7)
 *
 * :param note: the note of the key (0-11)
 *
 */
{
  // Register the key press in the first available voice
  taskENTER_CRITICAL();

  for (uint8_t i = 0; i < 12; i++)
  {
    // Check if voice is free
    if (voices[i].status == 0)
    {
      voices[i].status = 2;
      voices[i].note = note;
      voices[i].octave = octave;
      voices[i].intensityRightShift = 24;
      voices[i].cyclesPerHalfPeriod = 25;

      if (octave > 4)
      {
        voices[i].cyclesPerHalfPeriod = sampleFrequency / ((frequencies[note] << (octave - 4)) * 2);
      }
      else
      {
        voices[i].cyclesPerHalfPeriod = sampleFrequency / ((frequencies[note] >> (4 - octave)) * 2);
      }

      break;
    }
  }

  taskEXIT_CRITICAL();
}

void SoundGenerator::echoKey(uint8_t octave, uint8_t note)
/*
 * sets status of key to echo which gives it a limited time span before being removed
 *
 * :param octave: the octave of the key (1-7)
 *
 * :param note: the note of the key (0-11)
 *
 */
{
  taskENTER_CRITICAL();

  for (uint8_t i = 0; i < 12; i++)
  {
    if ((voices[i].status != 0) && voices[i].octave == octave && voices[i].note == note)
    {
      voices[i].status = 1;
      voices[i].lifeTime = getGlobalLifeTime();
      // break;
    }
  }

  taskEXIT_CRITICAL();
}

void SoundGenerator::removeKey(uint8_t octave, uint8_t note)
/*
 * Removes a key from the voices array, indicating the key has been released
 *
 * :param octave: the octave of the key (1-7)
 *
 * :param note: the note of the key (0-11)
 *
 */
{
  // Remove the first instance of a key press from the voices
  taskENTER_CRITICAL();

  for (uint8_t i = 0; i < 12; i++)
  {
    if ((voices[i].status != 0) && voices[i].octave == octave && voices[i].note == note)
    {
      voices[i].status = 0;
      voices[i].note = 0;
      voices[i].octave = 0;
      voices[i].phaseAcc = 0;
      voices[i].lifeTime = 0;
      voices[i].cyclesPerHalfPeriod = 0;
      break;
    }
  }

  taskEXIT_CRITICAL();
}

int32_t SoundGenerator::getVout()
/*
 * Calculates the output voltage for the sound based on the keys pressed and waveform
 *
 * :return: the output voltage (pre volume shifting and dc-offset addition)
 */
{
  uint8_t wf = __atomic_load_n(&waveform, __ATOMIC_RELAXED);
  int32_t Vout = 0;

  for (uint8_t i = 0; i < 12; i++)
  {
    // Checking not free voice
    if (voices[i].status != 0)
    {

      switch (wf)
      {
      // Sawtooth wave
      case 0:
        sawtooth(i);
        break;

      // sine wave
      case 1:
        sine(i);
        break;

      // square wave
      case 2:
        square(i);
        break;

      // traingular wave
      case 3:
        triangular(i);
        break;
      }

      // Checking if status is echo
      if (voices[i].status == 1)
      {
        uint32_t localLifeTime = getGlobalLifeTime();
        uint32_t scaleFactor = localLifeTime / 6;
        if (voices[i].lifeTime == localLifeTime - scaleFactor || voices[i].lifeTime == localLifeTime - (2 * scaleFactor) || voices[i].lifeTime == localLifeTime - (3 * scaleFactor) || voices[i].lifeTime == localLifeTime - (4 * scaleFactor) || voices[i].lifeTime == localLifeTime - (5 * scaleFactor))
        {
          voices[i].intensityRightShift += 1;
        }
        Vout += voices[i].phaseAcc >> voices[i].intensityRightShift;

        // Checking if lifetime is over
        if (voices[i].lifeTime == 0)
        {
          // removing key
          voices[i].status = 0;
          voices[i].note = 0;
          voices[i].octave = 0;
          voices[i].phaseAcc = 0;
        }
        else
        {
          // counting down lifetime
          voices[i].lifeTime -= 1;
        }
      }
      else
      {
        Vout += voices[i].phaseAcc >> 24;
      }
    }
  }

  return Vout;
}

uint8_t SoundGenerator::getWaveform()
/*
 * Atomically loads the current waveform type (0 = sawtooth)
 *
 * :return: the waveform id number (0-0)
 */
{
  return __atomic_load_n(&waveform, __ATOMIC_RELAXED);
}

void SoundGenerator::setWaveform(uint8_t wf)
/*
 * Atomically stores the selected waveform type (0 = sawtooth)
 *
 * :param wf: the waveform id number (0-0)
 */
{
  __atomic_store_n(&waveform, wf, __ATOMIC_RELAXED);
}

uint32_t SoundGenerator::getGlobalLifeTime()
/*
 * Atomically loads the current globalLifeTime
 *
 * :return: the adjusted global lifetime in terms of cycles
 */
{
  return __atomic_load_n(&globalLifetime, __ATOMIC_RELAXED);
}

void SoundGenerator::setGlobalLifeTime(uint16_t lifeTime)
/*
 * Atomically stores the selected globalLifeTime
 *
 * :param wf: the life time in seconds
 */
{
  __atomic_store_n(&globalLifetime, (lifeTime * 22000), __ATOMIC_RELAXED);
}

void SoundGenerator::sawtooth(uint8_t voiceIndx)
/*
 * Produces a sawtooth Vout for a specific note related to a specific voice
 *
 * :param voiceIndx: index of the specific voice that has already been checked if free
 *
 * :return: Vout for that specific voice that needs shifting and volume adjustment
 */
{
  uint8_t octave = voices[voiceIndx].octave;
  uint8_t note = voices[voiceIndx].note;
  int32_t currentVoiceStepSize;
  if (octave > 4)
  {
    // voices[i].phaseAcc += stepSizes[note] << (octave - 4);
    currentVoiceStepSize = stepSizes[note] << (octave - 4);
  }
  else
  {
    // voices[i].phaseAcc += stepSizes[note] >> (4 - octave);
    currentVoiceStepSize = stepSizes[note] >> (4 - octave);
  }
  // Creating note shift using joystick
  int32_t shift = getShift(currentVoiceStepSize);
  voices[voiceIndx].phaseAcc += shift;
}

void SoundGenerator::sine(uint8_t voiceIndx)
/*
 * Produces a sine Vout for a specific note related to a specific voice
 *
 * :param voiceIndx: index of the specific voice that has already been checked if free
 *
 * :return: Vout for that specific voice that needs shifting and volume adjustment
 */
{
  extern int8_t noteIndx;
  /////// TO DO: very broken, needs fixing and speeding up of computation ///////
  /*
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
  */
}

void SoundGenerator::square(uint8_t voiceIndx)
/*
 * Produces a square Vout for a specific note related to a specific voice
 *
 * :param voiceIndx: index of the specific voice that has already been checked if free
 *
 * :return: Vout for that specific voice that needs shifting and volume adjustment
 */
{

  static int32_t count = 0;
  extern Joystick joystick;

  if (voices[voiceIndx].phaseAcc == 0)
  {
    voices[voiceIndx].phaseAcc = 2147483647;
  }

  int32_t shift = voices[voiceIndx].cyclesPerHalfPeriod + ((joystick.getX() / 100) - 5);

  if (voices[voiceIndx].squareWaveCount == shift)
  {
    voices[voiceIndx].phaseAcc = voices[voiceIndx].phaseAcc * -1;
    voices[voiceIndx].squareWaveCount = 0;
  }
  else
  {
    voices[voiceIndx].squareWaveCount += 1;
  }
}
void SoundGenerator::triangular(uint8_t voiceIndx)
/*
 * Produces a triangular Vout for a specific note related to a specific voice
 *
 * :param voiceIndx: index of the specific voice that has already been checked if free
 *
 * :return: Vout for that specific voice that needs shifting and volume adjustment
 */

{
  uint8_t octave = voices[voiceIndx].octave;
  uint8_t note = voices[voiceIndx].note;
  int32_t currentVoiceStepSize;
  if (octave > 4)
  {
    currentVoiceStepSize = stepSizes[note] << (octave - 4);
  }
  else
  {
    currentVoiceStepSize = stepSizes[note] >> (4 - octave);
  }
  static int8_t upOrDown = 1;

  if (voices[voiceIndx].phaseAcc >= 200000000)
  {
    upOrDown = -1;
  }
  else if (voices[voiceIndx].phaseAcc <= -2000000000)
  {
    upOrDown = +1;
  }

  int32_t shift = getShift(currentVoiceStepSize);
  voices[voiceIndx].phaseAcc += (upOrDown * shift);
}

int32_t getShift(int32_t currentVoiceStepSize)
/*
 * Gets shift caused by movement in joystick x axis, applies shift to the current step size.
 * Note: function only gets called from a the interupt function sampleISR(), and therefore global variables can be accessed with no worry about synchronisation erros
 *
 * :return: shifted step size.
 */
{
  extern Joystick joystick;

  return currentVoiceStepSize + (-(joystick.getX() - 532) * 10000);
}

std::string SoundGenerator::getCurrentNotes()
/*
 * Gets the names of the current notes being played
 *
 * :return: string of current notes, space separated
 */
{
  std::string notesStr = "";

  taskENTER_CRITICAL();

  for (uint8_t i = 0; i < 12; i++)
  {
    if (voices[i].status == 2)
    {
      notesStr += notes[voices[i].note] + std::to_string(voices[i].octave) + " ";
    }
  }

  taskEXIT_CRITICAL();

  return notesStr;
}
