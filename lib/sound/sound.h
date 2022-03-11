#ifndef SOUND_H
#define SOUND_H

#include <cstdint>

struct Voice
{
  bool free;
  uint8_t octave;
  uint8_t note;
  int32_t phaseAcc;
};

class SoundGenerator
{
private:
  // Array containing all the voices
  Voice voices[12];

  // What waveform to produce - 0 = sawtooth
  volatile uint8_t waveform = 0;

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
};

#endif