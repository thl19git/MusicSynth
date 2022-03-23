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
    voices[i].waveCount = 0;
    voices[i].fOverfs = 0;
    voices[i].stepSize = 0;
    voices[i].upOrDown = 0;
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
      voices[i].upOrDown = 1;
      voices[i].waveCount = 0;

      if (octave > 4)
      {
        voices[i].cyclesPerHalfPeriod = sampleFrequency / ((frequencies[note] << (octave - 4)) * 2);
        voices[i].fOverfs = (frequencies[note] << (octave - 4)) / 22;
      }
      else
      {
        voices[i].cyclesPerHalfPeriod = sampleFrequency / ((frequencies[note] >> (4 - octave)) * 2);
        voices[i].fOverfs = (frequencies[note] >> (4 - octave)) / 22;
      }
      voices[i].stepSize = 4294967 * voices[i].fOverfs;
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
      voices[i].fOverfs = 0;
      voices[i].stepSize = 0;
      voices[i].upOrDown = 1;
      voices[i].waveCount = 0;
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
          voices[i].lifeTime = 0;
          voices[i].cyclesPerHalfPeriod = 0;
          voices[i].fOverfs = 0;
          voices[i].stepSize = 0;
          voices[i].upOrDown = 1;
          voices[i].waveCount = 0;
        }
        else
        {
          // counting down lifetime
          voices[i].lifeTime -= 1;
        }
      }
      else
      {
        if (wf == 3)
        {
          Vout += voices[i].phaseAcc >> 21;
        }
        else
        {
          Vout += voices[i].phaseAcc >> 24;
        }
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

  // Creating note shift using joystick
  int32_t shift = getShift(voices[voiceIndx].stepSize);
  voices[voiceIndx].phaseAcc += shift;
  // Serial.println(voices[voiceIndx].phaseAcc);
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
  extern Joystick joystick;
  // int32_t amplitude = 2147483647; // to be tuned

  int32_t shift = voices[voiceIndx].fOverfs + ((joystick.getX() / 100) - 5);

  float x = voices[voiceIndx].waveCount * shift;

  voices[voiceIndx].phaseAcc = AsinXLookUpTable(x) >> 1;
  // Serial.println(voices[voiceIndx].fOverfs);

  if (voices[voiceIndx].waveCount >= voices[voiceIndx].cyclesPerHalfPeriod * 2)
  {
    voices[voiceIndx].waveCount = 0;
  }
  else
  {
    voices[voiceIndx].waveCount += 1;
  }
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

  extern Joystick joystick;

  if (voices[voiceIndx].phaseAcc == 0)
  {
    voices[voiceIndx].phaseAcc = 2147483647;
  }

  int32_t shift = voices[voiceIndx].cyclesPerHalfPeriod + ((joystick.getX() / 100) - 5);

  if (voices[voiceIndx].waveCount == shift)
  {
    voices[voiceIndx].phaseAcc = voices[voiceIndx].phaseAcc * -1;
    voices[voiceIndx].waveCount = 0;
  }
  else
  {
    voices[voiceIndx].waveCount += 1;
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
  if (voices[voiceIndx].phaseAcc == 0)
  {
    voices[voiceIndx].phaseAcc = -voices[voiceIndx].stepSize * voices[voiceIndx].cyclesPerHalfPeriod / 2;
  }

  if (voices[voiceIndx].waveCount == voices[voiceIndx].cyclesPerHalfPeriod)
  {
    voices[voiceIndx].upOrDown = -1;
  }
  else if (voices[voiceIndx].waveCount == 2 * voices[voiceIndx].cyclesPerHalfPeriod)
  {

    voices[voiceIndx].upOrDown = +1;
    voices[voiceIndx].waveCount = 0;
  }

  voices[voiceIndx].waveCount += 1;

  int32_t shift = getShift(voices[voiceIndx].stepSize);
  voices[voiceIndx].phaseAcc += (voices[voiceIndx].upOrDown * shift);
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

int32_t AsinXLookUpTable(uint16_t x)
/*
 * A lookup table for sin() that is much less computationally expensive than the sin() function
 *
 * :param x: input to lookup in the form of (f/fs * cycleCount)
 *
 * returns the amplitude applied sin result equivilent to Asin((2*PI)*(f/fs)*cycleCount)
 */
{
  int32_t amplitude = 214748360; // to be tuned
  if (isBetween(0, 30, x))
  {
    return amplitude * 10;
  }
  else if (isBetween(30, 60, x))
  {
    return amplitude * 29;
  }
  else if (isBetween(60, 90, x))
  {
    return amplitude * 47;
  }
  else if (isBetween(90, 130, x))
  {
    return amplitude * 63;
  }
  else if (isBetween(130, 160, x))
  {
    return amplitude * 77;
  }
  else if (isBetween(160, 190, x))
  {
    return amplitude * 88;
  }
  else if (isBetween(190, 220, x))
  {
    return amplitude * 95;
  }
  else if (isBetween(220, 250, x))
  {
    return amplitude * 99;
  }
  else if (isBetween(250, 280, x))
  {
    return amplitude * 99;
  }
  else if (isBetween(280, 310, x))
  {
    return amplitude * 95;
  }
  else if (isBetween(310, 340, x))
  {
    return amplitude * 88;
  }
  else if (isBetween(340, 380, x))
  {
    return amplitude * 77;
  }
  else if (isBetween(380, 410, x))
  {
    return amplitude * 63;
  }
  else if (isBetween(410, 440, x))
  {
    return amplitude * 47;
  }
  else if (isBetween(440, 470, x))
  {
    return amplitude * 29;
  }
  else if (isBetween(470, 500, x))
  {
    return amplitude * 10;
  }
  else if (isBetween(500, 530, x))
  {
    return amplitude * -10;
  }
  else if (isBetween(530, 560, x))
  {
    return amplitude * -29;
  }
  else if (isBetween(560, 590, x))
  {
    return amplitude * -47;
  }
  else if (isBetween(590, 630, x))
  {
    return amplitude * -63;
  }
  else if (isBetween(630, 660, x))
  {
    return amplitude * -77;
  }
  else if (isBetween(660, 690, x))
  {
    return amplitude * -88;
  }
  else if (isBetween(690, 720, x))
  {
    return amplitude * -95;
  }
  else if (isBetween(720, 750, x))
  {
    return amplitude * -99;
  }
  else if (isBetween(750, 780, x))
  {
    return amplitude * -99;
  }
  else if (isBetween(780, 810, x))
  {
    return amplitude * -95;
  }
  else if (isBetween(810, 840, x))
  {
    return amplitude * -88;
  }
  else if (isBetween(840, 880, x))
  {
    return amplitude * -77;
  }
  else if (isBetween(880, 910, x))
  {
    return amplitude * -63;
  }
  else if (isBetween(910, 940, x))
  {
    return amplitude * -47;
  }
  else if (isBetween(940, 970, x))
  {
    return amplitude * -29;
  }
  else if (isBetween(970, 1000, x))
  {
    return amplitude * -10;
  }
  else
  {
    return 0;
  }
}

bool isBetween(float lowBound, float upBound, float x)
/*
 * Function used in AsinXLookUpTable() to check if x is: lowBound <= x < upBound
 *
 * :param lowBound: lower bound that x can be greater than or equal to
 *
 * :param upBound: upper bound that x can be less than
 *
 * :return: bool value to wether x is between value or not (true is it is)
 */
{
  if ((x >= lowBound) & (x < upBound))
  {
    return true;
  }
  else
  {
    return false;
  }
}