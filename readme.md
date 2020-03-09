# Rhythm Error

A project that uses Teensy and its audio shield to present an experiment in which participants hear a sequence of taps and then have to reproduce this.


## SD card
You need an SD card, formatted to `fat32`, and put the stimulus wave files (in the `stimulus` directory) on there.

Make sure you use a high quality SD card. We had some issues with cards and the `TDE 8GB Micro SDHC Card` worked well for us. The issues we had is that occasionally sounds would not play or they would play with distortions.


## Wiring design
For wiring design, solder in exactly the same way as TeensyTap (see [https://github.com/florisvanvugt/teensytap](Github)).



## Software
* Install Arduino IDE (make sure you get version 1.8.6). Download [here](https://www.arduino.cc/en/Main/Software)
* Install Teensyduino, which is an add-on for the Arduino software that allows you to use it with the Teensy. Download [here](https://www.pjrc.com/teensy/td_download.html)



## Technicalities

To launch a trial, you send the following to the serial port, in binary format, in this order:
* Number 44, encoded `unsigned char` (`!B`) (`MESSAGE_PLAY_STIMULUS`), i.e. 1 byte.
* 3 bytes representing the ASCII-encoded file name of the wave file that we will play.
* The number of seconds that we should record taps for, encoded as `int` (`i`), i.e. 4 bytes.

So that should give you a total packet size of 1+3+4 = 8 bytes.

The Teensy will then respond by telling you when it started playing something and when it records taps.




## TODO

- [x] Ensure that baudrate is high enough not to be disrupted by tap sending
- [ ] Fix python2 compatibility (encoding ASCII)

