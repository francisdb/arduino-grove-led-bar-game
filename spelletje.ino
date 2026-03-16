#include <Grove_LED_Bar.h>
#include <TM1637Display.h>

#define BUTTON_PIN 2
#define DISP_CLK 6
#define DISP_DIO 7

Grove_LED_Bar bar(5, 4, false, LED_BAR_10);
TM1637Display display(DISP_CLK, DISP_DIO);

// 5 rounds slow -> fast
const int NUM_ROUNDS = 5;
const int roundDelay[NUM_ROUNDS] = {70, 55, 40, 28, 18};

// base accuracy scoring
const int baseScoreByPos[10] = {1000, 500, 200, 60, 10, 0, 0, 0, 0, 0};

// later rounds worth more
const float roundMultiplier[NUM_ROUNDS] = {0.5, 0.7, 1.0, 1.4, 1.8};

enum GameState {
  WAITING_TO_START,
  ROUND_RUNNING,
  SHOWING_RESULT
};

GameState state = WAITING_TO_START;

bool lastButton = HIGH;
bool startArmed = false;   // only allow start after button has been released once

int roundIndex = 0;
int totalScore = 0;

int pos = 0;
int dir = 1;
int currentDelay = 70;

// 7-segment approximations for PLAY
const uint8_t WORD_PLAY[] = {
  0x73, // P
  0x38, // L
  0x77, // A
  0x6E  // Y-ish
};

void showIdleWord() {
  display.setSegments(WORD_PLAY);
}

void flashBar(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    bar.setBits(0x3FF);
    delay(onMs);
    bar.setBits(0x000);
    delay(offMs);
  }
}

void showRoundNumber(int r) {
  display.clear();
  display.showNumberDec(r + 1, false);
}

void startRound(int idx) {
  roundIndex = idx;
  currentDelay = roundDelay[roundIndex];

  showRoundNumber(roundIndex);

  pos = random(0, 10);
  dir = random(0, 2) ? 1 : -1;

  bar.setBits(1 << pos);
  state = ROUND_RUNNING;
}

int scoreForPosition(int visiblePos) {
  int base = baseScoreByPos[visiblePos];
  float multiplier = roundMultiplier[roundIndex];
  return (int)(base * multiplier);
}

void finishGame() {
  if (totalScore > 9999) totalScore = 9999;

  display.clear();
  display.showNumberDec(totalScore, false);

  state = SHOWING_RESULT;
}

void handleRoundHit() {
  int roundScore = scoreForPosition(pos);
  totalScore += roundScore;

  if (pos == 0) {
    flashBar(2, 60, 60);
    bar.setBits(1 << pos);
  }

  if (roundIndex >= NUM_ROUNDS - 1) {
    finishGame();
  } else {
    startRound(roundIndex + 1);
  }
}

void resetGame() {
  totalScore = 0;
  showIdleWord();
  bar.setBits(0x000);
  state = WAITING_TO_START;
  startArmed = false;   // require a release before a new start
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bar.begin();
  display.setBrightness(7);

  randomSeed(analogRead(A0));

  resetGame();

  // read actual startup state of the button
  lastButton = digitalRead(BUTTON_PIN);
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);

  if (state == WAITING_TO_START) {
    // arm start only after button is not pressed
    if (button == HIGH) {
      startArmed = true;
    }

    if (startArmed && button == LOW && lastButton == HIGH) {
      delay(20);
      totalScore = 0;
      startRound(0);
    }
  }
  else if (state == ROUND_RUNNING) {
    bar.setBits(1 << pos);

    if (button == LOW && lastButton == HIGH) {
      delay(20);
      handleRoundHit();
    } else {
      delay(currentDelay);

      pos += dir;

      if (pos >= 9) {
        pos = 9;
        dir = -1;
      }
      else if (pos <= 0) {
        pos = 0;
        dir = 1;
      }
    }
  }
  else if (state == SHOWING_RESULT) {
    if (button == LOW && lastButton == HIGH) {
      delay(20);
      resetGame();
    }
  }

  lastButton = button;
}