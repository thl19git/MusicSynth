# Features

The following diagram and table show the utilisation of the device's hardware:

![image](https://user-images.githubusercontent.com/59955474/159999112-e45b18e2-caa7-4559-83e8-ca13ded137f6.png)

In addition to the core features listed in the specification, a range of advanced features have been implemented to improve the synth's usability.

### Multiple Modules
The east and west handshake signals are used to allow automatic configuration of multiple modules, with support for auto-allocation for 2 static modules, or 2+ dynamic modules. Here a static module is one connected before any of the modules are powered on, whilst modules are dynamic if at least one is powered on before being connected. The handshaking process involves allocation of octaves and determining which modules should be transmitters and receivers. The user is free to change this allocation by pressing knob 2 on the module they wish to be the receiver.

### Polyphony
Multiple keys can be pressed at the same time to make a polyphonic sound as multiple key presses can be detected at once. The scanKeys task was improved to send a message to the SoundGenerator every time a key is pressed or released. The same messages are sent when a receiver module receives messages about key actions from other modules. The SoundGenerator class consists of an array of 12 Voices, where each voice contains all the information related to a key press that is required to calculate the output voltage sent to the speaker. Every execution of the sample ISR includes a call to the getVout method of the SoundGenerator class. This function loops through the Voices array, calculating the Vout for each voice, before summing them to get the final Vout for the sample.

### Changing Octaves
The rotation of Knob2 is used for changing the octave, the octave can vary from 1-7, and it is displayed on the UI.

### Different Waveforms
In order to generate interesting sounds, four types of waveforms are implemented, including sawtooth wave, sine wave, square wave and triangular wave. 

**Sawtooth wave**: The frequency of the note is changed by changing the step size for a phase accumulator.

**Sine wave**: A Look-up table is used for sin() that is much less computationally expensive than the sin() function. Half period of sine wave is divided into intervals, the output is returned based on the corresponding input, and then it is assigned to the phase accumulator.

**Square wave**: The value of the phase accumulator is switched between max and min of int32_t, depending on the frequency of the note.

**Triangle**: The corresponding step size is added or subtracted to the phase accumulator, and the sign is indicated by an ‘upOrDown’ signal.

![image](https://user-images.githubusercontent.com/59955474/159999894-b389ab1e-a7ca-47c6-9e78-61258ded5eb9.png)

The button of knob0 is used for varying the waveform, the default waveform is sawtooth, then it can be changed to sine, square, triangular wave when knob0 is pressed. 

### Joystick
The x-axis (horizontal) of the joystick is used for the frequency shift of tones. Moving the joystick to the left increases the current step size while moving to the right decreases it. It is called from the interrupt function, and therefore global variables can be accessed without worrying about the synchronisation errors.

### Echo
The music synthesiser can be control the length of a notes echo using rotation of knob0 (from 0 to 10 seconds). Once the key is released, the echo effect takes place, with the volume of the note decreasing to 0 during the echo duration. Other keys can be pressed while the echo is decaying.

### Intuitive UI
The UI displays all of the information the user needs, as shown in the diagram at the top of this page. The UI also changes when multiple modules are connected together, showing the state of each module (Tx or Rx), and only showing the relevant settings that can be changed by that particular module. When keys are pressed, both the notes and the octaves of those notes are displayed.

### Default Settings
The module is configured such that on power-up the volume and octaves are set to non-zero defaults (4 for octave and 8 for volume), reducing the need for initial set-up by the user.

### Unit Testing
Although not strictly an advanced feature, one thing we did in addition to the core specification was unit testing.

**Joystick testing**: The joystick button should be initialized to 1 by default whilst the joystick positions (x and y coordinates) are initialized to 0, TEST_ASSERT_EQUAL_INT8 is used to compared the results with the ideal reference, the tests were passed successfully. 

**Knob testing**: All possible combinations of inputs for knobs are tested. Since all the inputs should be 1 or 0, the edge case situation ‘inputs are not 0 or 1’ are also tested, the output should be 0. All the tests were passed.

**Sound testing**: The getWaveform and getGlobalLifeTime functions were tested for their initialization values, by default getWaveform returns the waveform id of 0, corresponding to sawtooth wave, (1 for sine, 2 for triangle and 3 for square wave) and the initial value of the global life time should also be 0 since there is no echo at the start; both reference values are set to 0 for this reason and the TEST_ASSERT_EQUAL_INT8 tests were passed successfully.

**User testing**: 
As well as using test scripts, user testing took place. This included general cases, edge cases and heavy computational load cases. 

**Synchronization and thread testing**:
This was done throughout the development process to ensure all timing issues and variable accesses were safe.

