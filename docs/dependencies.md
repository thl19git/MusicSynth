# Dependencies

### Dependency Graph

![dependency_graph_small](https://user-images.githubusercontent.com/59955474/160001696-7baf4ab5-a81f-4f37-ab8c-d9374698742c.png)

### Global Variables
The following is a list of the global variables in main.cpp, along with the tasks that access and modify them, and the steps taken to ensure synchronisation.

**volatile uint32_t keyArray[7]**
- Read in displayUpdateTask
- Updated in scanKeysTask
-	Protected by keyArrayMutex

**volatile uint8_t receiver**
-	Read in scanKeysTask, displayUpdateTask, decodeTask
-	Updated in scanKeysTask, autoMultiSynthTask, decodeTask
-	Uses atomic accesses

**volatile uint8_t connected**
-	Read in scanKeysTask, autoMultiSynthTask, displayUpdateTask, decodeTask,
-	Updated in autoMultiSynthTask, decodeTask
-	Uses atomic accesses

**volatile uint8_t eastConnection**
-	Read in decodeTask
-	Updated in decodeTask, autoMultiSynthTask
-	Uses atomic accesses
-	Protected by connectionMutex

**volatile uint8_t westConnection**
-	Read in autoMultiSynthTask
-	Updated in autoMultiSynthTask
-	Uses atomic accesses
-	Protected by connectionMutex

### Global Objects
In addition to the global variables listed above, there are a number of global objects used which abstract away some of the lower level hardware interactions and calculations.

**Knob knob0(0, 0, 10) - Rotation: Echo | Button: Sound wave**
-	Updated in scanKeysTask
-	Accessed in displayUpdateTask

**Knob knob1(1)**
-	Unused

**Knob knob2(2, 1, 7) - Rotation: Octave | Button: Tx/Rx**
-	Accessed in scanKeysTask, autoMultiSynthTask, displayUpdateTask, decodeTask
-	Updated in decodeTask, scanKeysTask

**Knob knob3(3, 0, 16) - Rotation: Volume**
-	Accessed in sampleISR, displayUpdateTask
-	Updated in scanKeysTask

**Joystick joystick**
-	Accessed in sampleISR
-	Updated in joystickTask

**SoundGenerator soundGen**
-	Accessed in sampleISR, displayUpdateTask
-	Updated in scanKeysTask, decodeTask

### Thread-Safe Classes
The objects above are instances of the following classes, which use a range of methods to ensure data synchronisation.

**Knob**
-	knobRotation and knobButton accessed atomically
-	Updates to knobRotation are performed on a local variable to ensure the global variable is never out of bounds
-	Can’t use a mutex or critical section as the values are accessed in an ISR. Worst case scenario is that for some samples the values are old (would lead to 1 sample at the wrong volume for example), but this is not noticeable to the user.

**Joystick**
-	x, y, and button values accessed atomically

**SoundGenerator**
-	Voices array only updated within critical sections and sampleISR
-	Critical sections ensure the Voices array is always up to date before the execution of sampleISR
-	waveform and globalLifetime accessed atomically
