#include <STM32FreeRTOS.h>
#include <Arduino.h>
#include "sound.h"
#include "joystick.h"
#include "main.h"

extern SemaphoreHandle_t notesMutex;

SoundGenerator::SoundGenerator()
/*
 * Constructor for the SoundGenerator class
 */
{
  for (uint8_t i = 0; i < 12; i++)
  {
    voices[i].free = true;
    voices[i].note = 0;
    voices[i].octave = 0;
    voices[i].phaseAcc = 0;
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
  xSemaphoreTake(notesMutex, portMAX_DELAY);

  for (uint8_t i = 0; i < 12; i++)
  {
    if (voices[i].free)
    {
      voices[i].free = false;
      voices[i].note = note;
      voices[i].octave = octave;
      break;
    }
  }

  xSemaphoreGive(notesMutex);
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
  xSemaphoreTake(notesMutex, portMAX_DELAY);

  for (uint8_t i = 0; i < 12; i++)
  {
    if (!voices[i].free && voices[i].octave == octave && voices[i].note == note)
    {
      voices[i].free = true;
      voices[i].note = 0;
      voices[i].octave = 0;
      voices[i].phaseAcc = 0;
      break;
    }
  }

  xSemaphoreGive(notesMutex);
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
    if (!voices[i].free)
    {

      switch (wf)
      {
      // Sawtooth wave
      case 0:
        sawtooth(i);
        break;
      }
    }
    Vout += voices[i].phaseAcc >> 24;
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

int32_t getShift(int32_t currentVoiceStepSize)
/*
 * Gets shift caused by movement in joystick x axis, applies shift to the current step size.
 * Note: function only gets called from a the interupt function sampleISR(), and therefore global variables can be accessed with no worry about synchronisation erros
 *
 * :return: shifted step size.
 */
{
  extern Joystick joystick;

  return currentVoiceStepSize + (-(joystick.x - 532) * 10000);
}