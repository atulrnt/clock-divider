#include <ShiftDisplay.h>
#include <ResponsiveAnalogRead.h>

#include <ShiftDisplay.h>
ShiftDisplay display(A1, A2, A0, COMMON_ANODE, 3);

#define TEMPO A7
#define DURATION A6
#define PROBABILITY A5
#define CLOCK A4
#define RESET A3

#define OUTPUT1 7
#define OUTPUT2 6
#define OUTPUT3 5
#define OUTPUT4 4
#define OUTPUT5 2
#define OUTPUT6 3
#define OUTPUT7 12
#define OUTPUT8 11
#define OUTPUT16 10
#define OUTPUT32 9
#define OUTPUT48 1
#define OUTPUT64 8

ResponsiveAnalogRead probabilityPot(PROBABILITY, true);
ResponsiveAnalogRead durationPot(DURATION, true);
ResponsiveAnalogRead tempoPot(TEMPO, true);

const byte dividers[11] = {2,3,4,5,6,7,8,16,32,48,64};
const byte outputs[11] = {OUTPUT2, OUTPUT3, OUTPUT4, OUTPUT5, OUTPUT6, OUTPUT7, OUTPUT8, OUTPUT16, OUTPUT32, OUTPUT48, OUTPUT64};
const int tempoRange[2] = {10, 600};

unsigned long currentTime = 0;
unsigned long lastTrigger = 0;
unsigned long lastClock = 0;
unsigned long lastClockCheck = 0;
unsigned long lastTempos = 1200;

boolean isResetActive = false;
boolean isClockActive = false;
byte externalClockCount = 0;

byte skippedDividers = 0;

int probability = 0;

int tempo = 120;
byte clockStep = 0;
byte triggerDuration = 20;

void setup() {
  pinMode(TEMPO, INPUT);
  pinMode(DURATION, INPUT);
  pinMode(CLOCK, INPUT_PULLUP);
  pinMode(RESET, INPUT_PULLUP);

  pinMode(OUTPUT1, OUTPUT);
  pinMode(OUTPUT2, OUTPUT);
  pinMode(OUTPUT3, OUTPUT);
  pinMode(OUTPUT4, OUTPUT);
  pinMode(OUTPUT5, OUTPUT);
  pinMode(OUTPUT6, OUTPUT);
  pinMode(OUTPUT7, OUTPUT);
  pinMode(OUTPUT8, OUTPUT);
  pinMode(OUTPUT16, OUTPUT);
  pinMode(OUTPUT32, OUTPUT);
  pinMode(OUTPUT48, OUTPUT);
  pinMode(OUTPUT64, OUTPUT);
}

void loop() {
  currentTime = millis();

  probabilityPot.update();
  durationPot.update();
  tempoPot.update();

  if (isExternallyClocked()) {
    checkClockInput();
  } else {
    checkTempoPot();
  }

//  lastTempos -= round(lastTempos/10);
//  lastTempos += tempo;
//  tempo = round(lastTempos/10);

  checkReset();

  checkProbabilityPot();
  checkTriggerDuration();
  
  triggerOutput();
  displayTempo();

  if (skippedDividers > 0) {
    display.setDot(2, true);
  }

  if (skippedDividers > 4) {
    display.setDot(1, true);
  }

  if (skippedDividers > 8) {
    display.setDot(0, true);
  }
  
  display.show(10);
}

void checkTempoPot() {    
  if (tempoPot.hasChanged()) {    
    tempo = map(tempoPot.getValue(), 0, 1023, tempoRange[0], tempoRange[1]);
  }
}

void checkProbabilityPot() {
  if (probabilityPot.hasChanged()) {
    probability = probabilityPot.getValue();
  
    if (probability < 10) {
      probability = 0;
    }
  }
}

void checkTriggerDuration() {
  if (durationPot.hasChanged()) {
    triggerDuration = map(durationPot.getValue(), 0, 1023, 0, 100);
  }
}

void checkReset() {
  if (!isResetActive && digitalRead(RESET) == LOW) {
    isResetActive = true;
    clockStep = 0;
  } else if (isResetActive && digitalRead(RESET) == HIGH) {
    isResetActive = false;
  }
}

int getTriggerDuration() {
  if (triggerDuration < 5) {
    return 20;
  }

  if (triggerDuration > 95) {
    return (60000 / tempo) - 20;
  }

  float duration = triggerDuration * 0.01;
  return round((60000 / tempo) * duration);
}



void triggerOutput() {
  skippedDividers = 0;
  
  if (currentTime - lastTrigger >= 60000 / tempo) {
    lastTrigger = currentTime;
    clockStep++;

    bool trigger = false;

    trigger = doTrigger();

    if (!trigger) {
     skippedDividers++;
    } else {
      // Base clock is always triggered
      digitalWrite(OUTPUT1, HIGH);
    }

    if (clockStep == 1) {
      return;
    }
    
    // Start triggers
    for (byte i = 0; i < 11; i++) {
      trigger = doTrigger();
      if (!trigger) {
        skippedDividers++;
      } else if (clockStep % dividers[i] == 0) {
        digitalWrite(outputs[i], HIGH);
      }
    }

    if (clockStep == 64) {
      clockStep = 0;
    }
  }

  // Stop triggers
  if (currentTime >= lastTrigger + getTriggerDuration()) {
    display.setDot(2, false);
    digitalWrite(OUTPUT1, LOW);
    
    for (byte i = 0; i < 11; i++) {
      digitalWrite(outputs[i], LOW);
    }
  }
}

void checkClockInput() {
  if (digitalRead(CLOCK) == LOW && !isClockActive) {
    isClockActive = true;
    lastClock = currentTime;
    tempo = round(60000 / (currentTime - lastClock));
  }

  if (digitalRead(CLOCK) == HIGH && isClockActive) {
    isClockActive = false;
  }
}

// Use a voltage divier of 3k - 1k
bool isExternallyClocked() {
  if (digitalRead(CLOCK) == LOW && !isClockActive) {
    lastClockCheck = currentTime;
  }
  return currentTime - lastClockCheck < 3000;
}

boolean doTrigger() {
  return random(2047) > probability;
}

void displayTempo() {
  display.show(tempo, 10);
}
