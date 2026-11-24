#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Output pins
const int morningLED = 6;
const int afternoonLED = 7;
const int nightLED = 8;
const int buzzer = 9;

// Button pins
const int buttons[5] = {A0, A1, A2, A3, A4};

// Button indexes
const int SET_BTN  = 0;
const int MODE_BTN = 1;
const int UP_BTN   = 2;
const int DOWN_BTN = 3;
const int OK_BTN   = 4;

// Simulated real-time clock
int currentHour = 7;
int currentMinute = 55;
int currentSecond = 0;

unsigned long previousSecond = 0;
unsigned long previousDisplay = 0;
int simulatedDay = 0;

// Medicine schedules
// Morning, Afternoon, Night
int pillHour[3] = {8, 14, 20};
int pillMinute[3] = {0, 0, 0};

const char* pillNames[3] = {
  "MORNING PILL",
  "AFTERNOON PILL",
  "NIGHT PILL"
};

const int ledPins[3] = {
  morningLED,
  afternoonLED,
  nightLED
};

// Alarm control
bool alarmActive = false;
int activePill = -1;
int lastAlarmDay[3] = {-1, -1, -1};

// Menu control
bool settingMode = false;
int selectedField = 0;

// Fields:
// 0 = clock hour
// 1 = clock minute
// 2 = morning hour
// 3 = morning minute
// 4 = afternoon hour
// 5 = afternoon minute
// 6 = night hour
// 7 = night minute

// Button debounce
bool lastReading[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
bool stableState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounce[5] = {0, 0, 0, 0, 0};

bool justPressed(int index) {
  bool reading = digitalRead(buttons[index]);

  if (reading != lastReading[index]) {
    lastDebounce[index] = millis();
    lastReading[index] = reading;
  }

  if (millis() - lastDebounce[index] > 35) {
    if (reading != stableState[index]) {
      stableState[index] = reading;

      if (stableState[index] == LOW) {
        return true;
      }
    }
  }

  return false;
}

void updateClock() {
  unsigned long now = millis();

  while (now - previousSecond >= 1000) {
    previousSecond += 1000;

    currentSecond++;

    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
    }

    if (currentMinute >= 60) {
      currentMinute = 0;
      currentHour++;
    }

    if (currentHour >= 24) {
      currentHour = 0;
      simulatedDay++;
    }
  }
}

void increaseField(int amount) {
  switch (selectedField) {

    case 0:
      currentHour = (currentHour + amount + 24) % 24;
      break;

    case 1:
      currentMinute = (currentMinute + amount + 60) % 60;
      currentSecond = 0;
      break;

    default: {
      int index = (selectedField - 2) / 2;

      if ((selectedField - 2) % 2 == 0) {
        pillHour[index] =
          (pillHour[index] + amount + 24) % 24;
      } else {
        pillMinute[index] =
          (pillMinute[index] + amount + 60) % 60;
      }
      break;
    }
  }
}

void showSettings() {
  lcd.clear();

  lcd.setCursor(0, 0);

  switch (selectedField) {
    case 0: lcd.print("SET CLOCK HOUR"); break;
    case 1: lcd.print("SET CLOCK MIN"); break;
    case 2: lcd.print("MORNING HOUR"); break;
    case 3: lcd.print("MORNING MIN"); break;
    case 4: lcd.print("AFTERNOON HOUR"); break;
    case 5: lcd.print("AFTERNOON MIN"); break;
    case 6: lcd.print("NIGHT HOUR"); break;
    case 7: lcd.print("NIGHT MIN"); break;
  }

  lcd.setCursor(0, 1);

  if (selectedField == 0) {
    lcd.print("Value: ");
    printTwoDigits(currentHour);
  }
  else if (selectedField == 1) {
    lcd.print("Value: ");
    printTwoDigits(currentMinute);
  }
  else {
    int index = (selectedField - 2) / 2;

    lcd.print("Value: ");

    if ((selectedField - 2) % 2 == 0) {
      printTwoDigits(pillHour[index]);
    } else {
      printTwoDigits(pillMinute[index]);
    }
  }

  lcd.print("  UP/DN");
}

void printTwoDigits(int value) {
  if (value < 10) {
    lcd.print("0");
  }

  lcd.print(value);
}

void handleSettings() {

  if (justPressed(MODE_BTN)) {
    selectedField++;

    if (selectedField > 7) {
      selectedField = 0;
    }

    showSettings();
  }

  if (justPressed(UP_BTN)) {
    increaseField(1);
    showSettings();
  }

  if (justPressed(DOWN_BTN)) {
    increaseField(-1);
    showSettings();
  }

  if (justPressed(OK_BTN)) {
    settingMode = false;
    previousSecond = millis();

    lcd.clear();
    lcd.print("SETTINGS SAVED");
    delay(500);
    lcd.clear();
  }
}

void checkMedicineTime() {

  if (settingMode || alarmActive) {
    return;
  }

  for (int i = 0; i < 3; i++) {

    if (currentHour == pillHour[i] &&
        currentMinute == pillMinute[i] &&
        lastAlarmDay[i] != simulatedDay) {

      alarmActive = true;
      activePill = i;

      lastAlarmDay[i] = simulatedDay;

      digitalWrite(ledPins[i], HIGH);
      tone(buzzer, 1000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(pillNames[i]);

      lcd.setCursor(0, 1);
      lcd.print("PRESS OK");
      return;
    }
  }
}

void acknowledgeAlarm() {

  if (alarmActive && justPressed(OK_BTN)) {

    noTone(buzzer);

    if (activePill >= 0) {
      digitalWrite(ledPins[activePill], LOW);
    }

    alarmActive = false;
    activePill = -1;

    lcd.clear();
    lcd.print("DOSE ACKNOWLEDGED");
    delay(1000);
    lcd.clear();
  }
}

void showMainScreen() {

  lcd.setCursor(0, 0);
  lcd.print("TIME: ");

  printTwoDigits(currentHour);
  lcd.print(":");
  printTwoDigits(currentMinute);
  lcd.print(":");
  printTwoDigits(currentSecond);

  lcd.setCursor(0, 1);
  lcd.print("SET: CONFIGURE  ");
}

void setup() {

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  pinMode(buzzer, OUTPUT);

  for (int i = 0; i < 5; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }

  lcd.begin(16, 2);

  lcd.print("SMART MEDICINE");
  lcd.setCursor(0, 1);
  lcd.print("REMINDER BOX");

  delay(2000);
  lcd.clear();

  previousSecond = millis();
}

void loop() {

  updateClock();

  // Enter settings
  if (!settingMode && !alarmActive &&
      justPressed(SET_BTN)) {

    settingMode = true;
    selectedField = 0;
    showSettings();
  }

  // Settings menu
  if (settingMode) {
    handleSettings();
    return;
  }

  // Check scheduled reminders
  checkMedicineTime();

  // Alarm acknowledgment
  if (alarmActive) {
    acknowledgeAlarm();
    return;
  }

  // Update LCD periodically
  if (millis() - previousDisplay >= 250) {
    previousDisplay = millis();
    showMainScreen();
  }
}
