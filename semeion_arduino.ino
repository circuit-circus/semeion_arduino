/* 
 ––––––––––––––––
|C I R C         |
|U I T ~ ~ ~ ~ ~ |
|                |
|~ ~ ~ ~ C I R C |
|            U S |
 ––––––––––––––––

SEMEION PROCESSING DEBUGGER
A CIRCUIT CIRCUS PROJECT 
*/

// Include Tact Library
#include <Tact.h>

// Init new Tact Toolkit
Tact Tact(TACT_MULTI);

int SPECTRUMSTART = 65;
const int SENSOR_ID = 0;
const int SPECTRUMSTEPS = 32;
const int SPECTRUMSTEPSIZE = 1;

String spectrumSerial = "";

// This shifts the SpectrumStart by one each time the shift timer runs out.
// It can be used to "hunt" for the best part of the spectrum
// This result in "crashing" after 8 iterations, since the Tact library can only handle 8 tactSensor objects, and I haven't figured out how to clear out old tactSensors
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

