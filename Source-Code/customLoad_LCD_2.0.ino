#include "HX711.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

#define echoPin 2
#define trigPin 3

const int LOADCELL_DATA_PIN = 4;
const int LOADCELL_CLOCK_PIN = 5;

const int button1Pin = 6; // Button 1
const int button2Pin = 7; // Button 2

HX711 scale;


double knownVal = 1000; // known weight value used for calibration.
double tare = -28.77; // variable carries tare value for claibration
double duration; // variable for the duration of sound wave travel
double distance; // variable for the distance measurement
double rollDistance = 0; // variable used to find rolling average of distance
double rollLoad = 0; // variable used to find rolling average of load
double weight = 0.5; // weight used for rollLoad and rollDistance calculations.
double load; // variable carries running weight in grams
int calibrate = 0; // variable will hod 1 if user wants to calibrate, 0 if not.
double calibration = -0.1; // variable holds the running calibration value.
double tempLoad = 0; // variable will hold largest load value.
int button1State = 0; // variable will hold 1 if button 1 is pressed
int button2State = 0; // variable will hold 1 if button 2 is pressed

LiquidCrystal_I2C lcd(0x27, 20, 4);


void setup() {
  Serial.begin(9600);
  wantCalibration();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  scale.begin(LOADCELL_DATA_PIN, LOADCELL_CLOCK_PIN);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  rollDistance = duration * 0.034 / 2.0; // Speed of sound wave divided by 2 (go and back)


  // If user wants to calibrate run this block.
  if (calibrate == 1) {

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("GettingTareValue");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(1000);

    tare = scale.read_average(20) / 100;

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Put 1kg on load cell");
    lcd.setCursor(0, 1);
    lcd.print("Then push button");
    int tempPress;
    tempPress =  buttonPress();

    load = (scale.read_average(20) / 100) - tare;

    calibration = load / knownVal;

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("  Remove Weight ");
    lcd.setCursor(0, 1);
    lcd.print("Then push button");
    tempPress = buttonPress();

    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Calibration = ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(calibration);
    EEPROM.put(0, calibration);
    delay(1000);
    delay(1000);
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("PutSampleInClamp");
    lcd.setCursor(0, 1);
    lcd.print("Press any button");
    tempPress = buttonPress();

    tare = scale.read_average(20) / 100;
  }
  // If user doesn't want to calibrate run this block.
  else {
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("FetchingDataFrom");
    lcd.setCursor(0, 1);
    lcd.print("Memory          ");
    delay(1000);
    delay(1000);
    calibration = EEPROM.get(0, calibration);
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Calibration =   ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(calibration);
    delay(1000);
    delay(1000);
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("PutSampleInClamp");
    lcd.setCursor(0, 1);
    lcd.print("Press any button");
    int tempPress;
    tempPress = buttonPress();

    tare = scale.read_average(20) / 100;

  }
  rollLoad = ((scale.get_value() / 100) - tare) / calibration;

}

void loop() {
  // Check if calibration failed.
  if (checkCalibration() == false) {
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Calibration Fail");
    lcd.setCursor(0, 1);
    lcd.print("RunProgramAgain");
    redLed();
  }
  // If calibration is successful check for break.
  else {
    if (checkBreak() == false) {
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(" Turn the crank ");
      lcd.setCursor(0, 1);
      lcd.print("     slowly      ");
      greenLed();
    }
    else {
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(" Break Occurred ");
      lcd.setCursor(0, 1);
      lcd.print("      Stop      ");
      redLed();
    }
  }

  load = ((scale.get_value() / 100) - tare) / calibration;

  if ( load > tempLoad) {
    tempLoad = load;
  }

  double tempD;
  distance = 0;
  for (int i = 0; i < 10; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);

    tempD = duration * 0.034 / 2.0;
    distance = distance + tempD;
  }
  distance =  distance / 10;

  if (distance > rollDistance) {
    rollDistance = ((1.0 - weight) * rollDistance) + (weight * distance);
    rollLoad = ((1.0 - weight) * rollLoad) + (weight * load);

    Serial.print(rollLoad);
    Serial.print('\t');

    Serial.print(rollDistance);
    Serial.print('\t');

    Serial.println(tempLoad);

  }
}

// Function checks if user wants to calibrate
void wantCalibration() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Always Calibrate");
  lcd.setCursor(0, 1);
  lcd.print("On First Startup");
  delay(1000);
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("B1:Calibrate    ");
  lcd.setCursor(0, 1);
  lcd.print("B2:No Calibrate ");
  int push = 0;
  push = buttonPress();

  // If user presses button 1, Calibrate is 1. If user presses button 2 , Calibrate is 0.
  if (push == 1) {
    calibrate = 1;
  }
  else {
    calibrate = 0;
  }
}

// Function checks if calibration was successsful.
bool checkCalibration() {
  if (calibration != 0) {
    return true;
  }
  else {
    return false;
  }
}

//Function checks if material broke.
bool checkBreak() {
  if (load < (0.25 * tempLoad)) {
    return true;
  }
  else {
    return false;
  }
}

// Function waits for user to either press button 1 or button 2.
int buttonPress() {
  while (button1State == 0 && button2State == 0) {
    button1State = digitalRead(button1Pin);
    button2State = digitalRead(button2Pin);
    if (button1State == HIGH) {
      button1State = 0;
      button2State = 0;
      return 1;
    }
    if (button2State == HIGH) {
      button1State = 0;
      button2State = 0;
      return 2;
    }
  }
}

// Function turns on Green LED and Turns off Red LED
void greenLed() {
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
}

// Function turns on REd LED and Turns off Green LED
void redLed() {
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  digitalWrite(8, LOW);
}
