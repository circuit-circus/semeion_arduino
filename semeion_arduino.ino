/*
	Multiple Sensors
 
 	This sketch shows how to read data from multiple sensors on a Tact sensor board
 
 	Created 16-10-2014
 	By Philipp Schmitt
 
 	Modified 16-10-2014
 	By Philipp Schmitt
 
 	http:// [github URL]
 */


// Include Tact Library
#include <Tact.h>

// Init new Tact Toolkit
Tact Tact(TACT_MULTI);

#define SPECTRUMSTART 48
#define SPECTRUMSTEPS 32
#define SPECTRUMSTEPSIZE 2

String spectrumSerial = "";


void setup() {

  Serial.begin(9600);

  // Start Tact toolkit
  Tact.begin();

  // Add new Sensor and config
  Tact.addSensor(0, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);
}


void loop() {

  // read Bias for sensor
  int bias = Tact.readBias(0);

  // read Peak for first sensor
  int peak = Tact.readPeak(0);

  // read Spectrum for first sensor
  // pay attention that the spectrum array can hold 
  // as many values as you have defined in Tact.addSensor();
  int spectrum[SPECTRUMSTEPS];
  Tact.readSpectrum(0, spectrum);
  
  spectrumSerial = "";
  for(int i = 0; i < SPECTRUMSTEPS; i++) {
    spectrumSerial = spectrumSerial + spectrum0[i];
    if(i < SPECTRUMSTEPS -1) {
      spectrumSerial = spectrumSerial + ",";
    }
  }
  delay(50);
  Serial.println(spectrumSerial);
}

