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

#define SENSOR_ID 0
int SPECTRUMSTART = 60;
#define SPECTRUMSTEPS 32
#define SPECTRUMSTEPSIZE 1

String spectrumSerial = "";

// This shifts the SpectrumStart by one each time the shift timer runs out.
// This result in "crashing" after 8 iterations, since the Tact library can only handle 8 sensors. 
bool shouldShiftStart = false;
long shiftTimer = 0;
static long shiftTimerDuration = 7500;

void setup() {

  Serial.begin(115200);

  // Start Tact toolkit
  Tact.begin();

  // Add new Sensor and config
  Tact.addSensor(SENSOR_ID, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);
  shiftTimer = millis();
}


void loop() {
  if(millis() > shiftTimer + shiftTimerDuration && shouldShiftStart) {
    SPECTRUMSTART += SPECTRUMSTEPS * SPECTRUMSTEPSIZE;
    Tact.addSensor(SENSOR_ID, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);
    shiftTimer = millis();
  }

  // read Peak and Bias for sensor
  int bias = Tact.readBias(SENSOR_ID);
  int peak = Tact.readPeak(SENSOR_ID);
  // read Spectrum for sensor
  // the spectrum array holds as many values as defined in SPECTRUMSTEPS;
  int spectrum[SPECTRUMSTEPS];
  Tact.readSpectrum(SENSOR_ID, spectrum);

  spectrumSerial = bias;
  spectrumSerial = spectrumSerial + ",";
  spectrumSerial = spectrumSerial + peak;
  spectrumSerial = spectrumSerial + ",";
  spectrumSerial = spectrumSerial + SPECTRUMSTART;
  spectrumSerial = spectrumSerial + ",";
  for(int i = 0; i < SPECTRUMSTEPS; i++) {
    spectrumSerial += spectrum[i];
    if(i < SPECTRUMSTEPS -1) {
      spectrumSerial = spectrumSerial + ",";
    }
  }
  delay(50);
  Serial.println(spectrumSerial);
}

