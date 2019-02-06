
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


// INCLUDES
#include <FastLED.h>
#include <SimpleTimer.h>

// LED

// Params for width and height
const uint8_t kMatrixWidth = 2;
const uint8_t kMatrixHeight = 58;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 255

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds[NUM_LEDS];

uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;

  if ( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if ( kMatrixSerpentineLayout == true) {
    if ( x & 0x01) {
      // Odd rows run backwards
      uint8_t reverseY = (kMatrixHeight - 1) - y;
      i = (x * kMatrixHeight) + reverseY;
    } else {
      // Even rows run forwards
      i = (x * kMatrixHeight) + y;
    }
  }

  return i;
}

// Doppler sensors
#define DOPPLER_PIN1 A0
#define DOPPLER_PIN2 A1

int currentActivity;
const uint8_t deactiveThreshold = 80;
long lastInteractionTime;
const int timeThreshold = 5000;
boolean isActive = false;
boolean isClimaxing = false;
const uint8_t climaxThreshold = 240;
uint8_t buildUp = 0;
const uint8_t reactionThreshold = 180;

//Learning values
int aniSpeed;
int chaos;
int baseHue = 150;

int baseSat = 255;

//Animation values
uint8_t confettiHeight = 25;
const uint8_t maxActiveConfettiLeds = 100;
uint8_t numActiveConfettiLeds = 71;
uint8_t activeConfettiLeds[maxActiveConfettiLeds];
uint8_t activeConfettiLedsT[maxActiveConfettiLeds];

boolean isReacting = false;

uint8_t reactionHeight = 0;
const uint8_t maxActiveReactionLeds = 100;
uint8_t numActiveReactionLeds = 0;
uint8_t activeReactionLeds[maxActiveReactionLeds];
uint8_t activeReactionLedsT[maxActiveReactionLeds];

uint8_t climaxT;
long lastClimaxTime;
int climaxLength = 1000;

//Curves
//float ease[] = {0, 1.5, 1.07, 0, 50}; // ease-in-out
//float hard[] = {1, 1, 1, 0, 20}; // Hard flash ease out

int valToUse;
int dopplerVal1; // Mellem 500-1024, skal smoothes
const int numReadings = 10;
int lastSensorAverage; // det sidste stykke tids læsninger
int lastSensorTotal;
int lastSensorReadings[numReadings];
int readIndex;

// These will be used to determine lower and higher bounds
int lowestReading = 520;
int highestReading = 100;

boolean goingDown = true;

int iterator = 1;
#define MAX_ANI 1;

SimpleTimer timer;

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );

  pinMode(DOPPLER_PIN1, INPUT);

  timer.setInterval(50, readCalculate);

  fill_solid(leds, kMatrixWidth * kMatrixHeight, CRGB::Black);

  for (int i = 0; i < maxActiveConfettiLeds; i++) {
    activeConfettiLeds[i] = random8(kMatrixWidth * kMatrixHeight - 1);
    activeConfettiLedsT[i] = random8(1, 255);
  }
}

void loop() {

  //    Serial.print("H ");
  //    Serial.print(confettiHeight);
//    Serial.print(", A ");
//    Serial.print(currentActivity);
//    Serial.print(", H ");
//    Serial.print(reactionHeight);
//    Serial.print(", BU ");
//    Serial.println(buildUp);
//    Serial.print(", HR ");
//    Serial.print(highestReading);
//    Serial.print(", C ");
//    Serial.println(isClimaxing);
    
  //    Serial.print(", AL ");
  //    Serial.println(numActiveConfettiLeds);

  determineStates();
  setAnimation();
  FastLED.show();

  timer.run();
}

void determineStates() {

  if (!isClimaxing) {

    if (currentActivity > deactiveThreshold) {
      //Become active
      isActive = true;
      confettiHeight = kMatrixHeight;
      lastInteractionTime = millis();
    } else if (currentActivity < deactiveThreshold && millis() - lastInteractionTime  > timeThreshold) {
      //Become idle
      isActive = false;
      buildUp = 0;
      confettiHeight = 25;
    }

    if (isActive && currentActivity > reactionThreshold) {
      isReacting = true;
      buildUp++;
      buildUp = constrain(buildUp, 0, 255);
      reactionHeight = map(currentActivity, reactionThreshold, 255, 0, kMatrixHeight);
    }

    if (buildUp > climaxThreshold) {
      //Climax
      isClimaxing = true;
      lastClimaxTime = millis();
    }
    if (currentActivity < reactionThreshold) {
      isReacting = false;
      reactionHeight = 0;
    }
  } else if (millis() - lastClimaxTime > climaxLength && isClimaxing && climaxT >= 250) {
    isClimaxing = false;
    buildUp = 0;
    climaxT = 0;
  }

}

void setAnimation() {

  if (!isClimaxing) {
    //buildUpAnimation();
    confettiAnimation();
    if (isReacting) {
      reactionAnimation();
    }
  } else {
    climaxAnimation();
  }
}

void readCalculate() {

  dopplerVal1 = analogRead(DOPPLER_PIN1);

  if (dopplerVal1 < lowestReading) {
    lowestReading = dopplerVal1;
  }

  if (dopplerVal1 > highestReading) {
    highestReading = dopplerVal1;
  }

  dopplerVal1 = map(dopplerVal1, lowestReading, highestReading, 0, 255);

  // Calculate last average
  lastSensorTotal = lastSensorTotal - lastSensorReadings[readIndex];
  lastSensorReadings[readIndex] = dopplerVal1;
  lastSensorTotal = lastSensorTotal + lastSensorReadings[readIndex];
  lastSensorAverage = lastSensorTotal / numReadings;
  readIndex += 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  currentActivity = lastSensorAverage;

}

int pick(int x_, int y_) {
  int x = x_;
  int y = y_;
  int p = XY(x, y);
  int checkedPick;
  boolean isTaken = false;
  
    for (int i = 0; i < numActiveConfettiLeds; i++) {
      if (activeConfettiLeds[i] == p) {
        isTaken = true;
      }
      if (isTaken) {
        i = 0;
        isTaken = false;
  
        y++;
        if (y > confettiHeight - 1) {
          y = 0;
          x++;
          if (x > kMatrixWidth - 1) {
            x = 0;
          }
        }
        p = XY(x, y);
      }
    }
  checkedPick = p;

  return checkedPick;
}

void rollUpAnimation() {}
void rollDownAnimation() {}

void buildUpAnimation() {
  uint8_t brightness = round(255 * (buildUp / 255.0));
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i, j)] = CHSV(200, 0, brightness);
    }
  }
}

void confettiAnimation() {
  float curve[] = {0, 1.5, 1.07, 0, 100}; // ease-in-out

  numActiveConfettiLeds = constrain(int(confettiHeight * kMatrixWidth / 0.7), 0, maxActiveConfettiLeds);

  for (int i = 0; i < numActiveConfettiLeds; i++) {
    if (activeConfettiLedsT[i] >= 255) {
      activeConfettiLeds[i] = pick(random(kMatrixWidth), random8(kMatrixHeight - confettiHeight, kMatrixHeight));
      activeConfettiLedsT[i] = 0;
    }
  }

  for (int i = 0; i < maxActiveConfettiLeds; i++) {

    float t = (float) activeConfettiLedsT[i] / 255.0;

    if (t < 0.99){

    float y = 0;
    int duration;

    y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier

    duration = curve[4];

    if (t + (1.0 / duration) >= 1.0) {
      t = 1.0;
    } else {
      t += (1.0 / duration);
    }

    activeConfettiLedsT[i] = constrain((uint8_t)round(t * 255.0),0,255);
    leds[activeConfettiLeds[i]] = CHSV(baseHue + i - int(maxActiveConfettiLeds / 2) , 255,  y * 255);
    }
  }
}

void reactionAnimation() {
  float curve[] = {1, 1, 1, 0, 20}; // Hard flash ease out

  numActiveReactionLeds = constrain(int(reactionHeight * kMatrixWidth / 0.9), 0, maxActiveReactionLeds);

  for (int i = 0; i < numActiveReactionLeds; i++) {
    if (activeReactionLedsT[i] >= 1) {
      activeReactionLeds[i] = pick(random(kMatrixWidth), random8((kMatrixHeight / 2) - (reactionHeight / 2), (kMatrixHeight / 2) + (reactionHeight / 2)));
      activeReactionLedsT[i] = 0;
    }
  }

  for (int i = 0; i < maxActiveReactionLeds; i++) {

    float t = (float) activeReactionLedsT[i] / 255.0;

    if(t < 0.99){

    float y = 0;
    int duration;

    y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier

    duration = curve[4];

    if (t + (1.0 / duration) >= 1.0) {
      t = 1.0;
    } else {
      t += (1.0 / duration);
    }

    activeReactionLedsT[i] = constrain((uint8_t)round(t * 255.0),0,255);
    leds[activeReactionLeds[i]] = CHSV(baseHue + i - int(maxActiveReactionLeds / 2) , 100,  y * 255);
    }
  }
}

void climaxAnimation() {
  float curve[] = {1, 1, 1, 0, 100};

  float t = (float) climaxT / 255.0;

  float y = 0;
  int duration;

  y = (1 - t) * (1 - t) * (1 - t) * curve[0] + 3 * (1 - t) * (1 - t) * t * curve[1] + 3 * (1 - t) * t * t * curve[2] + t * t * t * curve[3]; //Cubic Bezier

  duration = curve[4];

  if (t + (1.0 / duration) >= 1.0) {
    t = 1.0;
  } else {
    t += (1.0 / duration);
  }

  climaxT = (uint8_t) round(t * 255.0);

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i, j)] = CHSV(200 + i - int(maxActiveReactionLeds / 2) , 0, 255 * y);
    }
  }
}

void animate(){}

void animateTime(){}
