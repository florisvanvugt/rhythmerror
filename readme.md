# Rhythm Error

A project that uses Teensy and its audio shield to present an experiment in which participants hear a sequence of taps and then have to reproduce this.


## SD card
You need an SD card, formatted to `fat32`, and put the stimulus wave files (in the `stimulus` directory) on there.

Make sure you use a high quality SD card. We had some issues with cards and the `TDE 8GB Micro SDHC Card` worked well for us. The issues we had is that occasionally sounds would not play or they would play with distortions.


## Wiring design
For wiring design, solder in exactly the same way as TeensyTap (see Github).



## Software
* Install Arduino IDE (make sure you get version 1.8.6). Download [here](https://www.arduino.cc/en/Main/Software)
* Install Teensyduino, which is an add-on for the Arduino software that allows you to use it with the Teensy. Download [here](https://www.pjrc.com/teensy/td_download.html)



## TODO

- [ ] Ensure that baudrate is high enough not to be disrupted by tap sending
- [ ] Fix python2 compatibility (encoding ASCII)

