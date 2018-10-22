/* 
 ––––––––––––––––
|C I R C         |
|U I T ~ ~ ~ ~ ~ |
|                |
|                |
|                |
|~ ~ ~ ~ C I R C |
|            U S |
 ––––––––––––––––

SEMEION ARDUINO
A CIRCUIT CIRCUS PROJECT 
*/

// Include Tact Library
#include <Tact.h>

// Init new Tact Toolkit
Tact Tact(TACT_MULTI);

int SPECTRUMSTART = 73;
const int SENSOR_ID = 0;
const int SPECTRUMSTEPS = 24;
const int SPECTRUMSTEPSIZE = 1;

int spectrum[SPECTRUMSTEPS];

// Average values
const int AVRG_SAMPLE_COUNT = 5;
int avrgCounterSamples[SPECTRUMSTEPS][AVRG_SAMPLE_COUNT];
int avrgSampleIndex = 0;
float movingAvrg = 0.0f;

// Calibration
float baseline = 0.0f;
bool isSensorCalibrated = false;

void setup() {
  Serial.begin(9600);

  // Start Tact toolkit
  Tact.begin();
  // Add new Sensor and config
  Tact.addSensor(SENSOR_ID, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);
}

void loop() {
  // read Spectrum for sensor
  // the spectrum array holds as many values as defined in SPECTRUMSTEPS
  Tact.readSpectrum(SENSOR_ID, spectrum);

  // Keep track of how many samples we've gotten
  avrgSampleIndex = avrgSampleIndex < AVRG_SAMPLE_COUNT-1 ? avrgSampleIndex + 1 : 0;

  FillSamples();
  CalculateMovingAverage();

  if(millis() > 5000 && !isSensorCalibrated) {
    CalibrateSensor();
  }

  Serial.print(F("Variation from baseline: "));
  Serial.println(abs(movingAvrg-baseline));
}

void CalibrateSensor() {
  baseline = movingAvrg;
  isSensorCalibrated = true;
}

void FillSamples() {
  for (int i = 0; i < SPECTRUMSTEPS; i++) {
    avrgCounterSamples[i][avrgSampleIndex] = spectrum[i];
  }
}

void CalculateMovingAverage() {
  int avrgTotalArr[SPECTRUMSTEPS];
  float avrgArr[SPECTRUMSTEPS];
  float tempAverageTotal = 0.0f;

  // Get averages for each section of the spectrum
  for (int i = 0; i < SPECTRUMSTEPS; i++) {
    // Init the arrays to make sure they are 0
    avrgTotalArr[i] = 0;
    avrgArr[i] = 0.0f;

    // Add up the total avrg for this spectrum section
    for (int j = 0; j < AVRG_SAMPLE_COUNT; j++) {
      avrgTotalArr[i] += avrgCounterSamples[i][j];
    }
    // Divide by amount of samples in spectrum section to get average
    avrgArr[i] = avrgTotalArr[i] / AVRG_SAMPLE_COUNT;

    tempAverageTotal += avrgArr[i];
  }

  // Reset moving average
  movingAvrg = 0.0f;
  // Divide our temporary total of the averages with the amount of steps to get moving average
  movingAvrg = tempAverageTotal / SPECTRUMSTEPS;
}