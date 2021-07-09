
/*
 * Firmware for Mark's sidekick
 * by Michael L. Rivera and Stephanie Valencia
 *
 */

#include <Servo.h>

//Setting pin numbers 
const int buttonPin = 3;  // make sure to use interrupt pins for your button/switch pin (pin 3 is an interrupt pin in the MetroMini)
const int servoBase = 7; 
const int servoTop = 8; 

int posTop;
int posBase;

const int HOME_POS_TOP = 0;
const int HOME_POS_BOTTOM = 100;

//create servo objects
Servo myservoBase; 
Servo myservoTop;

// initializing our time variables and boolean
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers
unsigned long buttonPressStartTime = 0;
unsigned long buttonPressDuration = 0;
volatile boolean shouldCheckButtonInput = false;  //tells us when we need to debounce the button during the clock state

// tracking the states
const int STATE_NONE = 0;
const int STATE_CLOCK = 1;
const int STATE_WIGGLE = 2;

int currentState = STATE_NONE;

// MOVEMENT STATE for CLOCK and WIGGLE
const int MOVEMENT_STATE_NONE = 0;
const int MOVEMENT_STATE_PREAMBLE = 1;
const int MOVEMENT_STATE_ACTIVE = 2;

int movementState = MOVEMENT_STATE_NONE;
bool isMovementReversed = false;
bool wiggleBack = false;

long movementStartTime = 0;

void setup() {
  //inputs
  pinMode(buttonPin, INPUT);

  //outputs
  myservoBase.attach(servoBase);
  myservoTop.attach(servoTop); 

  //serial monitor
  Serial.begin(9600);
  Serial.println("Set up has started");

  // Set default position
  setInitialServoState();
  
  // For pin change interrupt
  attachInterrupt(digitalPinToInterrupt(buttonPin), interruptionCheck, CHANGE);
}

void interruptionCheck() { //ISR
  // LOW -> HIGH
  int buttonValue = digitalRead(buttonPin);
  if (buttonValue == HIGH) {
    buttonPressStartTime = millis();
    shouldCheckButtonInput = true; 
  } else {
    // HIGH -> LOW
    buttonPressDuration = millis() - buttonPressStartTime; 
    shouldCheckButtonInput = false; 
  }
}


void loop() {
  checkButtonInput();
  updateState();
  updateServoPositions();
}

void updateState() {
  //Serial.println(currentState);
  switch (currentState) {
    case STATE_NONE:
      updateNoneState();
      break;
    case STATE_CLOCK:
      updateClockState();
      break;
    case STATE_WIGGLE:
      updateWiggleState();
      break;
    default:
      break;
  }
  
}

void checkButtonInput() {

  if (shouldCheckButtonInput){
    delay(1000);
    if (buttonPressDuration < debounceDelay) {
      return;
      }
      // debouncing the button to make sure we had a button press and not noise
      // NONE -> WIGGLE || NONE -> CLOCK
      // WIGGLE || CLOCK ---> NONE
      if (currentState == STATE_NONE) {
        if (buttonPressDuration < 1000) {
          currentState = STATE_CLOCK;
          return;
        } else if (buttonPressDuration >= 1000) {
          currentState = STATE_WIGGLE;
          return;
        }    
      } else {
        // currentState = WIGGLE or CLOCK
        currentState = STATE_NONE;
        return;
      } 
  } else{
    return;
  }
}

void updateServoPositions() {
   long delayTime = getStateDelay();
   myservoTop.write(posTop);
   delay(delayTime);
   myservoBase.write(posBase);              
   delay(delayTime);
}

long getStateDelay() { // this function returns a specific time delay between servo positions, which helps achieve desired motion sequences.
  switch (currentState) {
    case STATE_NONE:
      return 40; //the delay between servo position change
      
    case STATE_CLOCK:
      if (movementState == MOVEMENT_STATE_NONE || movementState == MOVEMENT_STATE_PREAMBLE) {
        return 40;
      } else if (movementState == MOVEMENT_STATE_ACTIVE) {
        return 500;// this works great
      } else {
        return 40;
      }
      
    case STATE_WIGGLE:
      if (movementState == MOVEMENT_STATE_NONE || movementState == MOVEMENT_STATE_PREAMBLE) {
        return 15;
      } else if (movementState == MOVEMENT_STATE_ACTIVE) {
        return 25;
      } else {
        return 40;
      }
    default:
      return 40;
  }
}

void setInitialServoState() {
  // set initial motor states
  myservoTop.write(HOME_POS_TOP);
  delay(40);
  for (int temp = 45; temp <= 100; temp += 1){ 
    myservoBase.write(temp);              
    delay(40);                      
  }
  setHomePosition();
}

void setHomePosition() {
  posTop = HOME_POS_TOP;
  posBase = HOME_POS_BOTTOM;
}

void updateNoneState() {
  setHomePosition();
  movementState = MOVEMENT_STATE_NONE;
  movementStartTime = 0;
  isMovementReversed = false;
}

//clock or "timer" motion - moves sidekick from left to right for a set duration mimicking a clock or timer.
void updateClockState() {
  if (movementState == MOVEMENT_STATE_NONE) {
      posTop = HOME_POS_TOP;
      posBase = HOME_POS_BOTTOM;
      movementStartTime = millis();
      isMovementReversed = false;
      movementState = MOVEMENT_STATE_ACTIVE; // our user requested to skip the preamble for the "CLOCK" motion but if you want to activate the preamble you can set this to = MOVEMENT_STATE_PREAMBLE
      return;
  } else if (movementState == MOVEMENT_STATE_PREAMBLE) {
    posTop += 2;
    if (posTop >= 95) {
      posTop = 0;  //goes back to zero once it gets to 95 deg.   
      movementState = MOVEMENT_STATE_ACTIVE;
      movementStartTime = millis();
      isMovementReversed = false;
    }
    return;
  } else if (movementState == MOVEMENT_STATE_ACTIVE) {
    long currentDuration = millis() - movementStartTime;
    Serial.println("clock is active now");
    Serial.println(currentDuration);
    if (currentDuration >= 90000) { //current "CLOCK" motion duration (90 secs).This can be adjusted as desired.
      currentState = STATE_NONE;
      return;
    }
    if (posTop >= 179) {
      isMovementReversed = true;
    } else if (posTop <= 0) {
      isMovementReversed = false;
    }
    if (isMovementReversed) {
      posTop -= 6;
    } else {
      posTop += 6;
    }
  } 
} 

// wiggle function - this motion can be used to call for attention it makes the sidekick move to the center point and then wiggle for a set duration
void updateWiggleState() {
  if (movementState == MOVEMENT_STATE_NONE) {
      posTop = HOME_POS_TOP;
      posBase = HOME_POS_BOTTOM;
      movementState = MOVEMENT_STATE_PREAMBLE;
      return;
   } else if (movementState == MOVEMENT_STATE_PREAMBLE){ // the wiggle motion preamble is to rise up first for a few seconds.
      posTop += 2;
      if (posTop >= 95) { 
        movementState = MOVEMENT_STATE_ACTIVE;
        movementStartTime = millis();
        wiggleBack = false;
       }
       return;
   } else if (movementState == MOVEMENT_STATE_ACTIVE) {
        long currentDuration = millis() - movementStartTime;
        if (currentDuration >= 8000) { // total duration for the wiggle motion in milliseconds
          currentState = STATE_NONE;
          return;
        }
        if (posTop >= 110) {
            wiggleBack = true;
          } else if (posTop <= 70) {
            wiggleBack = false;
          }
          
          if (wiggleBack) {
            posTop -= 2;
          } else {
            posTop += 2;
          } 
   } 
}
