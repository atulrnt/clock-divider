#include <ShiftDisplay.h>
#include <ResponsiveAnalogRead.h>

ShiftDisplay display(A5, A6, A7, COMMON_ANODE, 3);

#define PROBABILITY 13
#define DURATION A0
#define TEMPO A1
#define TEMPO_RANGE_0 A2
#define TEMPO_RANGE_2 A3
#define CLOCK A4

#define OUTPUT1 2
#define OUTPUT2 3
#define OUTPUT3 4
#define OUTPUT4 5
#define OUTPUT5 6
#define OUTPUT6 7
#define OUTPUT7 8
#define OUTPUT8 9
#define OUTPUT16 10
#define OUTPUT32 11
#define OUTPUT64 12

ResponsiveAnalogRead probabilityPot(PROBABILITY, true);
ResponsiveAnalogRead durationPot(DURATION, true);
ResponsiveAnalogRead tempoPot(TEMPO, true);

const byte dividers[10] = {2,3,4,5,6,7,8,16,32,64};
const byte outputs[10] = {OUTPUT2, OUTPUT3, OUTPUT4, OUTPUT5, OUTPUT6, OUTPUT7, OUTPUT8, OUTPUT16, OUTPUT32, OUTPUT64};
const byte tempoRanges[3][2] = {
  {1, 60},
  {61, 120},
  {121, 240},
};

unsigned long currentTime = 0;
unsigned long lastTrigger = 0;
unsigned long lastClock = 0;

boolean isClockActive = false;

int probability = 0;

byte tempo = 120;
byte tempoRange = 0;
byte clockStep = 0;
byte triggerDuration = 20;

void setup() {
  pinMode(TEMPO, INPUT);
  pinMode(TEMPO_RANGE_0, INPUT_PULLUP);
  pinMode(TEMPO_RANGE_2, INPUT_PULLUP);
  pinMode(DURATION, INPUT);
  pinMode(CLOCK, INPUT_PULLUP);

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
  pinMode(OUTPUT64, OUTPUT);

  display.show("GO", 500, ALIGN_LEFT);
  display.show("GO", 500, ALIGN_RIGHT);
  display.show("GO", 500, ALIGN_LEFT);
  display.show("GO", 500, ALIGN_RIGHT);
  display.show("ATU", 2000);
}

void loop() {
  currentTime = millis();

  probabilityPot.update();
  durationPot.update();
  tempoPot.update();

  if (isExternallyClocked) {
    checkClockInput();
  } else {
    checkTempoSwitch();
    checkTempoPot();
  }

  checkProbabilityPot();
  checkTriggerDuration();
  
  triggerOutput();
  displayTempo();
}

void checkTempoPot() {
  if (tempoPot.hasChanged()) {
    tempo = map(tempoPot.getValue(), 0, 1023, tempoRanges[tempoRange][0], tempoRanges[tempoRange][1]) * 1000;
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

byte getTriggerDuration() {
  if (triggerDuration < 5) {
    return 20;
  }

  if (triggerDuration > 95) {
    return tempo - 20;
  }

  return tempo * (triggerDuration / 100);
}

void checkTempoSwitch() {
  if (digitalRead(TEMPO_RANGE_0)) {
    tempoRange = 0;
  } else if (digitalRead(TEMPO_RANGE_2)) {
    tempoRange = 2;
  } else {
    tempoRange = 1;
  }
}

void checkClockInput() {
  if (digitalRead(CLOCK) == HIGH && !isClockActive) {
    isClockActive = true;
    tempo = currentTime - lastClock;
    lastClock = currentTime;
  }

  if (digitalRead(CLOCK) == LOW && isClockActive) {
    isClockActive = false;
  }
}

void triggerOutput() {
  if (currentTime - lastTrigger >= tempo) {
    lastTrigger = currentTime;
    clockStep++;

    // Base clock is always triggered
    digitalWrite(OUTPUT1, HIGH);

    if (clockStep == 1) {
      return;
    }

    // Start triggers
    for (byte i = 0; i < 10; i++) {
      if (clockStep % dividers[i] == 0 && doTrigger()) {
        digitalWrite(outputs[i], HIGH);
      }
    }

    if (clockStep == 64) {
      clockStep = 1;
    }
  }

  // Stop triggers
  if (currentTime >= lastTrigger + getTriggerDuration()) {
    digitalWrite(OUTPUT1, LOW);
    
    for (byte i = 0; i < 10; i++) {
      digitalWrite(outputs[i], LOW);
    }
  }
}

// Use a voltage divier of 3k - 1k
bool isExternallyClocked() {
  int clockSignal = analogRead(CLOCK);
  return !(clockSignal <= 270 && clockSignal >= 230);
}

boolean doTrigger() {
  return random(2047) > probability;
}

void displayTempo() {
  display.show(round(tempo / 1000));
};
