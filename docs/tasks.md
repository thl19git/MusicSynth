# Tasks

## Threads

### scanKeys
- Sets and reads from knob and key addresses.
- Detects any knob changes and indicates which key is being pressed.
- Safely updates sound generation objects and other global variables.

### joystick
- Sets and reads data from the joystick button, x and y coordinates.
- Safely stores them in the corresponding global variables

### autoMultiSynth
- Detects other keyboards and puts the appropriate messages on the queue 

### displayUpdate
- Safely reads from the global variables
- Prints important, relevant and up-to-date information on the module display

### decode
- Interprets messages received over the CAN bus
- Handles key press and release events from external modules
- Involved in establishing a connection with other modules, for example auto-setting the octave

### CAN_TX
- Obtains a message from the queue
- Takes the CAN_TX semaphore. 
-	Sends the message over the CAN bus.

## Interrupts

### CAN_TX_ISR
- Releases the CAN_TX semaphore

### CAN_RX_ISR
- Receives a message from the CAN bus and places it on the incoming messages queue

### sampleISR
- Calls the getVout() method of the SoundGenerator class to generate the output voltage. This takes into consideration all of the notes being played, the octaves, any echo, and the wave type.
- Sets the volume
- Applies analogue output voltage at each sample interval
