#include <PWM.h>
#include <Servo.h> 


Servo rampServo;

struct NutType {
  bool isConductive;
  bool isTall;
  bool isMagnetic;
  int currentPosition;
  int nextPosition;
};

const bool DEBUG_MODE = false;
// POSITIONS
const int NUMBER_OF_NUTS = 5;
const int NUMBER_OF_INDEXES = 5;
const int RESET_DISTANCE = 200;
const int INDEX_DISTANCE_INCREMENT = RESET_DISTANCE/NUMBER_OF_INDEXES;
const int SERVO_DROP_DISTANCE = RESET_DISTANCE - INDEX_DISTANCE_INCREMENT;

const int continuityPosition = 2.5*INDEX_DISTANCE_INCREMENT;
const int tallSwitchPosition = 3.5*INDEX_DISTANCE_INCREMENT;
const int magnetoPosition = 1.5*INDEX_DISTANCE_INCREMENT;

// TIMING DECLARATIONS
const long SERVO_WAIT_INTERVAL_HOLD = 100;
const long SERVO_WAIT_INTERVAL_RETURN = 350;

unsigned long TALLSWITCH_LASTINTERRUPT;
unsigned long HALLEFFECT_LASTINTERRUPT;
unsigned long CONDUCTIVITY_LASTINTERRUPT;
const unsigned long INTERRUPT_WAIT_TIME = 450;

long previousMillis = 0; 

const int INDEX_TIME_WAIT = 740;

// NUT OBJECT ARRAY
volatile NutType nuts[NUMBER_OF_NUTS] = {
                            {false, false, false, 0, 1*INDEX_DISTANCE_INCREMENT}, 
                            {false, false, false, 1*INDEX_DISTANCE_INCREMENT, 2*INDEX_DISTANCE_INCREMENT}, 
                            {false, false, false, 2*INDEX_DISTANCE_INCREMENT, 3*INDEX_DISTANCE_INCREMENT}, 
                            {false, false, false, 3*INDEX_DISTANCE_INCREMENT, 4*INDEX_DISTANCE_INCREMENT}, 
                            {false, false, false, 4*INDEX_DISTANCE_INCREMENT, 5*INDEX_DISTANCE_INCREMENT}
                            };

// STEPPER DECS
int stepperFrequencyPin = 12, stepperDirection = 50, enableStepper = 48;

// SERVO PIN
const int servoPin = 7;

// INTERRUPT PINS
volatile int tallSwitchInterruptPin = 18;
volatile int shortSwitchInterruptPin = 2;
volatile int continuityTestPin = 3;
volatile int hallEffectDigitalPin = 2;
int shutOffSwitch = 5;


void moveServo(int servoPosition) {
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  rampServo.write(servoPosition);
//  while(currentMillis - previousMillis < SERVO_WAIT_INTERVAL_HOLD) {
////      Serial.print("\nMoving Servo");
////      Serial.println(currentMillis - previousMillis);
//      currentMillis = millis();
//  

//  rampServo.write(90);
//  currentMillis = millis();
//  previousMillis = currentMillis;
//    while(currentMillis - previousMillis < SERVO_WAIT_INTERVAL_RETURN) {
////      Serial.println("\nMoving Servo Back");
//      currentMillis = millis();
////      Serial.println(currentMillis - previousMillis);
//  }
}


void tallSwitchInterrupt() {
  if (millis() - TALLSWITCH_LASTINTERRUPT > INTERRUPT_WAIT_TIME) {
    TALLSWITCH_LASTINTERRUPT = millis();
    
    if (DEBUG_MODE) { 
      Serial.print("\n---- TALL SWITCH HIT ---------\nDigitalVal: \t");
      Serial.println(digitalRead(tallSwitchInterruptPin));
      Serial.println("-----------------");
    }
    for (int i = 0; i < NUMBER_OF_NUTS; i++) {
      if (nuts[i].currentPosition <= tallSwitchPosition && nuts[i].nextPosition >= tallSwitchPosition) {
        nuts[i].isTall = true;
      }
    }
  }
}

void continuityTest() {
  if (millis() - CONDUCTIVITY_LASTINTERRUPT > INTERRUPT_WAIT_TIME) {
    CONDUCTIVITY_LASTINTERRUPT = millis();
    
    if (DEBUG_MODE) { 
      Serial.print("\n---- Contiuty Detected ---------\nDigitalVal: \t");
      Serial.println(digitalRead(continuityTestPin));
      Serial.println("-----------------");
    }
    for (int i = 0; i < NUMBER_OF_NUTS; i++) {
      if (nuts[i].currentPosition <= continuityPosition && nuts[i].nextPosition >= continuityPosition) {
        nuts[i].isConductive = true;
      }
    }
  }
}

void magnetoSensorRead() {
  if (millis() - HALLEFFECT_LASTINTERRUPT > INTERRUPT_WAIT_TIME) {
    HALLEFFECT_LASTINTERRUPT = millis();
    
    if (DEBUG_MODE) { 
      Serial.print("\n---- Magentic Detected ---------\nDigitalVal: \t");
      Serial.println(digitalRead(hallEffectDigitalPin));
      Serial.println("-----------------");
    }
    for (int i = 0; i < NUMBER_OF_NUTS; i++) {
      if (nuts[i].currentPosition <= magnetoPosition && nuts[i].nextPosition >= magnetoPosition) {
        nuts[i].isMagnetic = true;
      }
    }
  }
}
 
int calculateStepperFrequency(float moveTime) {
  return (int) INDEX_DISTANCE_INCREMENT/moveTime;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(enableStepper, OUTPUT);
  pinMode(stepperDirection, OUTPUT);
  pinMode(stepperFrequencyPin, OUTPUT);
  pinMode(hallEffectDigitalPin, INPUT);
  pinMode(shutOffSwitch, INPUT);
  
  pinMode(servoPin, OUTPUT);
  rampServo.attach(servoPin);

  attachInterrupt(digitalPinToInterrupt(tallSwitchInterruptPin), tallSwitchInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(continuityTestPin), continuityTest, RISING);
  attachInterrupt(digitalPinToInterrupt(hallEffectDigitalPin), magnetoSensorRead, FALLING);

  Serial.println("SERVO DROP POSITION: " + (String) SERVO_DROP_DISTANCE);
  Serial.println("NUMBER OF STEPS IN FULL ROTATION: " + (String) RESET_DISTANCE);
  Serial.println("TALL POSITION: " + (String) tallSwitchPosition);
  Serial.println("CONDUC POSITION: " + (String) continuityPosition);
  Serial.println("HALL POSITION: " + (String) magnetoPosition);
}

void loop() {
  // put your main code here, to run repeatedly:
    // INCREMENT ALL NUT POSITIONS BY INDEX_DISTANCE_INCREMENT
  // SERVO LOGIC POSITION CHECKS
  for (int i = 0; i < NUMBER_OF_NUTS; i++) {
    if (nuts[i].currentPosition == SERVO_DROP_DISTANCE) {
      //SERVO CODE GOES HERE

      if (DEBUG_MODE) {
        Serial.println("Nut '" + (String) i + "' Properties To Move Servo to are: ");
        Serial.println("isConductive: " + (String) nuts[i].isConductive); 
        Serial.println("isTall: " + (String) nuts[i].isTall); 
        Serial.println("isMagnetic: " + (String) nuts[i].isMagnetic); 
        Serial.println("currentPosition: " + (String) nuts[i].currentPosition);
        Serial.println("nextPosition: " + (String) nuts[i].nextPosition);
        Serial.println("");
      }

      // big brass check
      if (nuts[i].isConductive == true && nuts[i].isTall == true && nuts[i].isMagnetic == false) {
        moveServo(110);
        if (DEBUG_MODE) { 
          Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!BIG BRASS NUT DETECTED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          Serial.println("");
        }
      }
      // nylon check
      if (nuts[i].isConductive == false && nuts[i].isTall  == true && nuts[i].isMagnetic == false) {     
        moveServo(138);
        
        if (DEBUG_MODE) { 
          Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!NYLON NUT DETECTED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          Serial.println("");
        }

      }
      // small steel check
      if (nuts[i].isConductive == true && nuts[i].isTall == false && nuts[i].isMagnetic == true) {
        moveServo(86);
        if (DEBUG_MODE) {         
          Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!SMALL STEEL NUT DETECTED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          Serial.println("");
        }
      }
      // small brass check  
      if (nuts[i].isConductive == true && nuts[i].isTall == false && nuts[i].isMagnetic == false) {
        moveServo(62);
        if (DEBUG_MODE) { 
          Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!SMALL BRASS NUT DETECTED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          Serial.println("");
        }
      }
    }
  }
//  delay(2000);
  //RUN STEPPER MOTOR
  digitalWrite(enableStepper, HIGH); 
  digitalWrite(stepperDirection, HIGH);
//  SetPinFrequencySafe(stepperFrequencyPin, 0); 51.5
  analogWrite(stepperFrequencyPin, 51.5);
  delay(813);
  digitalWrite(enableStepper, LOW); 
  
  for (int i = 0; i < NUMBER_OF_NUTS; i++) {
    if (nuts[i].currentPosition == RESET_DISTANCE) {
      // RESET PROPERTIES
      nuts[i].isConductive = false;
      nuts[i].isTall = false;
      nuts[i].isMagnetic = false;
      nuts[i].currentPosition = 0*INDEX_DISTANCE_INCREMENT;
      nuts[i].nextPosition = 1*INDEX_DISTANCE_INCREMENT;
      if (DEBUG_MODE) { 
        Serial.println("Reset all of '" + (String) i + "' properties to: ");
        Serial.println("isConductive: " + (String) nuts[i].isConductive); 
        Serial.println("isTall: " + (String) nuts[i].isTall); 
        Serial.println("isMagnetic: " + (String) nuts[i].isMagnetic); 
        Serial.println("currentPosition: " + (String) nuts[i].currentPosition);
        Serial.println("nextPosition: " + (String) nuts[i].nextPosition);
        Serial.println("");
      }
    }
  }

  if (DEBUG_MODE) {
    Serial.println("---------------------Nut Properties----------------------------");
  }
  for (int i = 0; i < NUMBER_OF_NUTS; i++) {
    nuts[i].currentPosition += INDEX_DISTANCE_INCREMENT;
    nuts[i].nextPosition += INDEX_DISTANCE_INCREMENT;
    if (DEBUG_MODE) { 
      Serial.println("Incremented Position " + (String) i + " from: " + (String) nuts[i].currentPosition
        + " , To " + (String) nuts[i].nextPosition);
      Serial.println("isConductive: " + (String) nuts[i].isConductive); 
      Serial.println("isTall: " + (String) nuts[i].isTall); 
      Serial.println("isMagnetic: " + (String) nuts[i].isMagnetic); 
      Serial.println("currentPosition: " + (String) nuts[i].currentPosition);
      Serial.println("nextPosition: " + (String) nuts[i].nextPosition);
      Serial.println("");
    }
  }

  if (DEBUG_MODE) {
    Serial.println("---------------------Nut Properties----------------------------");
  }
  // SHUT OFF SWITCH CHECK
  Serial.println("");
  if (digitalRead(shutOffSwitch) == 0){
    Serial.println("Shutting down");
    digitalWrite(enableStepper, LOW);
    exit(0);
  }
}
