# Rhythm Error

A project that uses Teensy and its audio shield to present an experiment in which participants hear a sequence of taps and then have to reproduce this.


## SD card
You need an SD card, formatted to `fat32`, and put the stimulus wave files (in the `stimulus` directory) on there.

Make sure you use a high quality SD card. We had some issues with cards and the `TDE 8GB Micro SDHC Card` worked well for us. The issues we had is that occasionally sounds would not play or they would play with distortions.


## Wiring design
For wiring design, solder in exactly the same way as TeensyTap (see [https://github.com/florisvanvugt/teensytap](Github)).





## TODO

- [x] Ensure that baudrate is high enough not to be disrupted by tap sending
- [ ] Fix python2 compatibility (encoding ASCII)

