
/*


  Rhythm Error Presentation

  A Teensy 3.2 interface that is designed to
  - capture finger taps from a FSR
  - communicate tap timings to a PC through USB
  - play a WAVE file from the SD card.


  Based on TeensyTap

  This is code designed to work with Teensyduino
  (i.e. the Arduino IDE software used to generate code you can run on Teensy)

 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


// Load the samples that we will play for taps and metronome clicks, respectively
#include "DeviceID.h"


/*
  Setting up infrastructure for capturing taps (from a connected FSR)
*/

boolean active = false; // Whether the tap capturing & metronome and all is currently active

boolean prev_active = false; // Whether we were active on the previous loop iteration

int fsrAnalogPin = 3; // FSR is connected to analog 3 (A3)
int fsrReading;      // the analog reading from the FSR resistor divider

// For interpreting taps
int tap_onset_threshold    = 20; // the FSR reading threshold necessary to flag a tap onset
int tap_offset_threshold   = 10; // the FSR reading threshold necessary to flag a tap offset
int min_tap_on_duration    = 20; // the minimum duration of a tap (in ms), this prevents double taps
int min_tap_off_duration   = 40; // the minimum time between offset of one tap and the onset of the next taps, again this prevents double taps


int tap_phase = 0; // The current tap phase, 0 = tap off (i.e. we are in the inter-tap-interval), 1 = tap on (we are within a tap)


unsigned long current_t            = 0; // the current time (in ms)
unsigned long prev_t               = 0; // the time stamp at the previous iteration (used to ensure correct loop time)
unsigned long next_event_embargo_t = 0; // the time when the next event is allowed to happen
unsigned long trial_end_t          = 0; // the time that this trial will end
unsigned long record_duration_t    = 0; // how long to record for


unsigned long tap_onset_t = 0;  // the onset time of the current tap
unsigned long tap_offset_t = 0; // the offset time of the current tap
int           tap_max_force = 0; // the maximum force reading during the current tap
unsigned long tap_max_force_t = 0; // the time at which the maximum tap force was experienced



boolean void_sound = false; // whether there is a sound to play
unsigned long start_play_t = 0; // the time when we started playing the stimulus
boolean sound_playing = false; // whether currently a sound is playing




int missed_frames = 0; // ideally our script should read the FSR every millisecond. we use this variable to check whether it may have skipped a millisecond


boolean running_trial = false; // whether we are currently running the trial



/*
  Setting up the audio
*/


AudioPlaySdWav           playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S           audioOutput;
//AudioOutputSPDIF       audioOutput;
//AudioOutputAnalog      audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14





/*
  Serial communication stuff
*/


int msg_number = 0; // keep track of how many messages we have sent over the serial interface (to be able to track down possible missing messages)

long baudrate = 9600; // the serial communication baudrate; not sure whether this actually does anything because Teensy documentation suggests that USB communication is always the highest possible.





const int MESSAGE_PLAY_STIMULUS       = 44; // Teensy should start playing a stimulus file
//const int MESSAGE_RECORD_TAPS         = 55; // Teensy should record the subject's taps
const int MESSAGE_STOP                = 66;   // Signal to the Teensy to stop whatever it is doing


const int PLAY_INSTRUCTION_LENGTH        = 7; // Defines the length of the instruction packet to play
//const int RECORD_INSTRUCTION_LENGTH      = 1*4; // Defines the length of the instruction packet to record taps


char PLAY_FILENAME[PLAY_INSTRUCTION_LENGTH+1] = {'\0'}; // The filename that we should play




void setup(void) {
  /* This function will be executed once when we power up the Teensy */
  
  Serial.begin(baudrate);  // Initiate serial communication
  Serial.print("TeensyTap starting...\n");

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(8);

  // Comment these out if not using the audio adaptor board.
  // This may wait forever if the SDA & SCL pins lack
  // pullup resistors
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }


  Serial.print("TeensyTap ready.\n");

  active = false;
}






void do_activity() {
  /* This is the usual activity loop */
  
  /* If this is our first loop ever, initialise the time points at which we should start taking action */
  if (prev_t == 0)           { prev_t = current_t; } // To prevent seeming "lost frames"
  
  if (current_t > prev_t) {
    // Main loop tick (one ms has passed)
    
    
    if ((prev_active) && (current_t-prev_t > 1)) {
      // We missed a frame (or more)
      missed_frames += (current_t-prev_t);
    }
    
    
    /*
     * Collect data
     */
    fsrReading = analogRead(fsrAnalogPin);
    

    /* 
     * Check the status of the playing sound
     */
    if (sound_playing && (start_play_t - current_t > 5)) { // we leave a little buffer
      if (!playWav1.isPlaying()) {
	sound_playing = false;
	Serial.print("# t=");
	Serial.print(current_t);
	Serial.println(" SOUND END");
      }
      
    }
    

    /*
     * Process data: has a new tap onset or tap offset occurred?
     */

    if (tap_phase==0) {
      // Currently we are in the tap-off phase (nothing was thouching the FSR)
      
      /* First, check whether actually anything is allowed to happen.
	 For example, if a tap just happened then we don't allow another event,
	 for example we don't want taps to occur impossibly close (e.g. within a few milliseconds
	 we can't realistically have a tap onset and offset).
	 Second, check whether this a new tap onset
      */
      if ( (current_t > next_event_embargo_t) && (fsrReading>tap_onset_threshold)) {

	// New Tap Onset
	tap_phase = 1; // currently we are in the tap "ON" phase
	tap_onset_t = current_t;
	// don't allow an offset immediately; freeze the phase for a little while
	next_event_embargo_t = current_t + min_tap_on_duration;

      }
      
    } else if (tap_phase==1) {
      // Currently we are in the tap-on phase (the subject was touching the FSR)
      
      // Check whether the force we are currently reading is greater than the maximum force; if so, update the maximum
      if (fsrReading>tap_max_force) {
	tap_max_force_t = current_t;
	tap_max_force   = fsrReading;
      }
      
      // Check whether this may be a tap offset
      if ( (current_t > next_event_embargo_t) && (fsrReading<tap_offset_threshold)) {

	// New Tap Offset
	
	tap_phase = 0; // currently we are in the tap "OFF" phase
	tap_offset_t = current_t;
	
	// don't allow an offset immediately; freeze the phase for a little while
	next_event_embargo_t = current_t + min_tap_off_duration;

	// Send data to the computer!
	send_tap_to_serial();

	// Clear information about the tap so that we are ready for the next tap to occur
	tap_onset_t     = 0;
	tap_offset_t    = 0;
	tap_max_force   = 0;
	tap_max_force_t = 0;
	
      }
      
    }

    // Update the "previous" state of variables
    prev_t = current_t;
  }

}




void play_stimulus() {
  /* This plays the stimulus that we are supposed play at this point. */
  // Start playing the file.  This sketch continues to
  // run while the file plays.

  if (!void_sound) {
  
    char filename[] = "XXX.WAV"; // = "WM5.WAV";
    sprintf(filename,"%s.WAV",PLAY_FILENAME);
    
    Serial.print("PLAY ");
    Serial.println(filename);
    playWav1.play(filename);
  }
  
  start_play_t = current_t; // This is when we started playing the sound
  sound_playing = true;
    
  // A brief delay for the library read WAV info
  //delay(5);

  // Simply wait for the file to finish playing.
  //while (playWav1.isPlaying()) {
  //}
  
  //Serial.println("DONE");
}






void loop(void) {
  /* This is the main loop function which will be executed ad infinitum */

  //playFile("WM5.WAV");
  
  current_t = millis(); // get current time (in ms)

  if (active) {
    do_activity();
  }
  // Signal for the next loop iteration whether we were active previously.
  // For example, if we weren't active previously then we don't want to count lost frames.
  prev_active = active;


  if (active && running_trial && (current_t > trial_end_t)) {
    // Trial has ended (we have completed the number of metronome clicks and continuation clicks)

    // Play another sound to signal to the subject that the trial has ended.
    //sound1.play(AudioSampleEndsignal);

    // Communicate to the computer
    Serial.print("FINISH\n");
    Serial.print("# Trial completed at t=");
    Serial.print(current_t);
    Serial.print("\n");
    
    running_trial = false;
  }


  
  /* 
     Read the serial port, see if some message is available for us.
  */
  if (Serial.available()) {
    int inByte = Serial.read();

    if (inByte==MESSAGE_PLAY_STIMULUS) { // We are going to receive config information from the PC
      read_play_config_from_serial();
      play_stimulus();

      // Compute when this trial will end
      trial_end_t = current_t + ( record_duration_t*1000 ) ; // Determine until when we will be recording
      
      active        = true;
      running_trial = true;
      //ready_to_send = false;
    
      tap_phase        = 0;
      
      /* And that's it, we're live! */

    //if (inByte==MESSAGE_RECORD_TAPS) { // We are going to receive config information from the PC
    //read_record_config_from_serial();


    }

    
    if (inByte==MESSAGE_STOP) {   // Switch to inactive mode
      Serial.print("# Stop signal received at t=");
      Serial.print(current_t);
      Serial.print("\n");
      active = false; // Put our activity on hold
    }
    
  }

}





long readint() {
  /* Reads an int (well, really a long int in Arduino land) from the Serial interface */
  union {
    byte asBytes[4];
    long asLong;
  } reading;
  
  for (int i=0;i<4;i++){
    reading.asBytes[i] = (byte)Serial.read();
  }
  return reading.asLong;

}



void read_play_config_from_serial() {
  /* 
     This function runs when we are receiving instructions
     about a stimulus file we need to play.
  */
  active = false; // Ensure we are not active while receiving configuration (this can have unpredictable results)
  Serial.print("# Receiving play configuration...\n");

  while (!(Serial.available()>=PLAY_INSTRUCTION_LENGTH)) {
    // Wait until we have enough info
  }
  Serial.print("# ... starting to read...\n");
  // Read the config
  //char PLAY_FILENAME[PLAY_INSTRUCTION_LENGTH+1] = {'\0'};
  //PLAY_FILENAME = {'\0'}; // initialise to zero
  void_sound = true; // by default, let's assume there is no sound specified
  for (byte i=0; i<PLAY_INSTRUCTION_LENGTH; i++){
    PLAY_FILENAME[i] = Serial.read();
    if (PLAY_FILENAME[i]!='-') void_sound = false;
  }
  PLAY_FILENAME[3] = '\0'; // end with a proper string ending -- otherwise can't ping this back.
  //Serial.println(PLAY_FILENAME);

  // Read the record duration (in sec)
  record_duration_t     = readint();
  
  Serial.print("# Config received...\n");
  send_config_to_serial();
  send_header();

  // Reset some of the other configuration parameters
  missed_frames           = 0;
  msg_number              = 0; // start messages from zero again
  
}





void send_config_to_serial() {
  /* Sends a dump of the current config to the serial. */

  Serial.print  ("# Device installed ");
  Serial.println(DEVICE_ID);

}






void send_header() {
  /* Sends information about the current tap to the PC through the serial interface */
  Serial.print("message_number type play_start_t onset_t offset_t max_force_t max_force n_missed_frames\n");
}  


void send_tap_to_serial() {
  /* Sends information about the current tap to the PC through the serial interface */
  char msg[100];
  msg_number += 1; // This is the next message
  sprintf(msg, "%d tap %lu %lu %lu %lu %d %d\n",
	  msg_number,
	  start_play_t,
	  tap_onset_t,
	  tap_offset_t,
	  tap_max_force_t,
	  tap_max_force,
	  missed_frames);
  Serial.print(msg);
  
}





