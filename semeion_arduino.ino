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

// Includes
#include <Tact.h>
#include <Wire.h>

// Tact Inits
Tact Tact(TACT_MULTI);
int SPECTRUMSTART = 73;
const int SENSOR_ID = 0;
const int SPECTRUMSTEPS = 24;
const int SPECTRUMSTEPSIZE = 1;
int spectrum[SPECTRUMSTEPS];

// I2C Inits
#define SLAVE_ADDRESS 0x08

// 10 byte data buffer
int receiveBuffer[9];
// A counter to send back
uint8_t keepCounted = 0;

// Tact - Average values
const int AVRG_SAMPLE_COUNT = 5;
int avrgCounterSamples[SPECTRUMSTEPS][AVRG_SAMPLE_COUNT];
int avrgSampleIndex = 0;
float movingAvrg = 0.0f;

// Tact - Calibration
const int BASELINE_SAMPLE_COUNT = 5;
float baselineSamples[BASELINE_SAMPLE_COUNT];
int baselineSampleIndex = 0;
float baselineTotal = 0.0f;
float baseline = 0.0f;
static float baselineTolerance = 1.75f; // How much "noise" can be tolerate from the baseline?

void setup() {
  Serial.begin(9600);

  // Start Tact toolkit
  Tact.begin();
  Tact.addSensor(SENSOR_ID, SPECTRUMSTART, SPECTRUMSTEPS, SPECTRUMSTEPSIZE);

  // Start I2C
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
}

void loop() {
  RunTactSensor();
}

// Read data in to buffer with the offset in first element, so we can use it in sendData
void receiveData(int byteCount){
  int counter = 0;
  while(Wire.available()) {
    receiveBuffer[counter] = Wire.read();
    counter++;
  }
}

// Use the offset value to select a function
void sendData(){
  float diff = abs(movingAvrg-baseline);
  if(diff >= baselineTolerance) {
    diff = abs(movingAvrg-baseline) - baselineTolerance;
  }
  else {
    diff = 0.0f;
  }

  // If the buffer is set to 99, make some data
  if (receiveBuffer[0] == 99) {
    writeData(diff);
  }
  else if (receiveBuffer[0] == 98) {
    writeData(2.222f);
  }
  else {
    Serial.println("No function for this address");
  }
}

// Write data
void writeData(float newData) {
  char dataString[8];
  // Convert our data to a string/char[] with min length 5
  dtostrf(newData, 5, 3, dataString);
  Wire.write(dataString);
}

void RunTactSensor() {
  // read Spectrum for sensor
  // the spectrum array holds as many values as defined in SPECTRUMSTEPS
  Tact.readSpectrum(SENSOR_ID, spectrum);

  // Keep track of how many samples we've gotten
  avrgSampleIndex = avrgSampleIndex < AVRG_SAMPLE_COUNT-1 ? avrgSampleIndex + 1 : 0;

  FillSamples();
  CalculateMovingAverage();

  // Calibrate in the first 5 seconds
  if(millis() < 3000) {
    CalibrateSensor();
    Serial.println("Calibrating");
  }
}

void CalibrateSensor() {
  // Get next sample
  baselineSamples[baselineSampleIndex] = movingAvrg;

  // Reset baseline total
  baselineTotal = 0.0f;
  for(int i = 0; i < BASELINE_SAMPLE_COUNT; i++) {
    // Add samples to baseline total
    baselineTotal += baselineSamples[i];
  }
  // Baseline is equal to the total of the samples divided by the amount of samples, i.e. the average
  baseline = baselineTotal / BASELINE_SAMPLE_COUNT;

  // Keep track of amount of samples gathered
  baselineSampleIndex = baselineSampleIndex < BASELINE_SAMPLE_COUNT-1 ? baselineSampleIndex + 1 : 0;
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