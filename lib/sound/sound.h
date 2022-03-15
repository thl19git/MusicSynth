#ifndef SOUND_H
#define SOUND_H

#include <cstdint>

struct Voice
{

  // State
  uint8_t status;
  /*
   * status = 0: free
   * status = 1: echo (dying)
   * status = 2: not free
   */

  // note varaiables
  uint8_t octave;
  uint8_t note;

  // Sawtooth
  int32_t phaseAcc;

  // Square
  uint16_t cyclesPerHalfPeriod;
  uint8_t squareWaveCount;

  // Echo variables
  uint32_t lifeTime;           // echo life time
  uint8_t intensityRightShift; // echo degrading intensity factor
};

class SoundGenerator
{
private:
  // Array containing all the voices
  Voice voices[12];

  // What waveform to produce - 0 = sawtooth
  volatile uint8_t waveform = 0;

  volatile uint32_t globalLifetime;

public:
  SoundGenerator();
  /*
   * Constructor for the SoundGenerator class
   */

  void addKey(uint8_t octave, uint8_t note);
  /*
   * Adds a key to the voices array, indicating the key has been pressed
   *
   * :param octave: the octave of the key (1-7)
   *
   * :param note: the note of the key (0-11)
   *
   */

  void echoKey(uint8_t octave, uint8_t note);
  /*
   * sets status of key to echo which gives it a limited time span before being removed
   *
   * :param octave: the octave of the key (1-7)
   *
   * :param note: the note of the key (0-11)
   *
   */
  void removeKey(uint8_t octave, uint8_t note);
  /*
   * Removes a key from the voices array, indicating the key has been released
   *
   * :param octave: the octave of the key (1-7)
   *
   * :param note: the note of the key (0-11)
   *
   */

  // Should only be called from an ISR
  int32_t getVout();
  /*
   * Calculates the output voltage for the sound based on the keys pressed and waveform
   *
   * :return: the output voltage (pre volume shifting and dc-offset addition)
   */

  uint8_t getWaveform();
  /*
   * Atomically loads the current waveform type (0 = sawtooth)
   *
   * :return: the waveform id number (0-0)
   */

  void setWaveform(uint8_t wf);
  /*
   * Atomically stores the selected waveform type (0 = sawtooth)
   *
   * :param wf: the waveform id number (0-0)
   */

  uint32_t getGlobalLifeTime();
  /*
   * Atomically loads the current globalLifeTime
   *
   * :return: the adjusted global lifetime in terms of cycles
   */

  void setGlobalLifeTime(uint16_t lifeTime);
  /*
   * Atomically stores the selected globalLifeTime
   *
   * :param wf: the life time in seconds
   */

  void sawtooth(uint8_t voiceIndx);
  /*
   * Produces a sawtooth Vout for a specific note related to a specific voice
   *
   * :param voiceIndx: index of the specific voice that has already been checked if free
   *
   * :return: Vout for that specific voice that needs shifting and volume adjustment
   */

  void sine(uint8_t voiceIndx);
  /*
   * Produces a sine Vout for a specific note related to a specific voice
   *
   * :param voiceIndx: index of the specific voice that has already been checked if free
   *
   * :return: Vout for that specific voice that needs shifting and volume adjustment
   */

  void square(uint8_t voiceIndx);
  /*
   * Produces a square Vout for a specific note related to a specific voice
   *
   * :param voiceIndx: index of the specific voice that has already been checked if free
   *
   * :return: Vout for that specific voice that needs shifting and volume adjustment
   */

  void triangular(uint8_t voiceIndx);
  /*
   * Produces a triangular Vout for a specific note related to a specific voice
   *
   * :param voiceIndx: index of the specific voice that has already been checked if free
   *
   * :return: Vout for that specific voice that needs shifting and volume adjustment
   */

  std::string getCurrentNotes();
  /*
   * Gets the names of the current notes being played
   *
   * :return: string of current notes, space separated
   */
};

int32_t getShift(int32_t currentVoiceStepSize);
/*
 * Gets shift caused by movement in joystick x axis, applies shift to the current step size.
 * Note: function only gets called from a the interupt function sampleISR(), and therefore global variables can be accessed with no worry about synchronisation erros
 *
 * :return: shifted step size.
 */

#endif