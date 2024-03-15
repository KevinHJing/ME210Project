// ULTRASONIC SENSOR NEEDS
#define USE_TIMER_1 true
#define USE_TIMER_2 true

#include "TimerInterrupt.h"
#define TIMER_FREQ_HZ 100

#include <Wire.h> 

volatile byte stateUltra = LOW;

// defines pins numbers
const int trigPin = 6;
const int echoPin = 8;

// defines variables
long duration;
int distance;
// for data filtering
int prev2 = 0;
int prev1 = 0;
int average;
int newDistance;

// for arduino I2C connection
/*
COMMUNICATION PROTOCOL:
Send 2 digit number: +/- 10-19, +/- 20-29, 31-32, 41-42, 
First digit: which motor (1 - left driving motor, 2 - right driving motor, 3 - contact servo, 4 - ball servo)
Second digit: speed for driving motors (0-9), position for servo motors (1 - initial position, 2 - second position, contact A, 3 - contact B)
Sign: only for driving motors, negative means backwards, forward means forward
*/
void toMotor(int toSend){
  delay(20);
  Wire.beginTransmission(9);
  Wire.write(toSend);
  Wire.endTransmission();
}

void TimerHandler1() {
  stateUltra = !stateUltra;
  digitalWrite(trigPin, stateUltra);
}

int readUltrasonic(){
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  newDistance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(newDistance);

  // if not noise reading
  // if (newDistance - distance < 10 || distance == 0){
    // move the prev variables down
    prev2 = prev1;
    prev1 = distance;
    distance = newDistance;

    // calculate average distance
    average = ((prev2 + prev1 + distance) / 3);
  // }

  return average;
}

boolean exitedStart = false;

void ExitStartTimerHandler(){
  exitedStart = true;
}

// thresholds - ultrasonic TODO: NEED THESE VALUES THROUGH TESTING
int startzonethresholdmin = 63;
int startzonethresholdmax = 80;
int exittowallthreshold = 52;
int startexitthreshold = 130;
int exitInterval = 3500;


// TAPE SENSOR NEEDS
const int leftTapePin = A1;
const int rightTapePin = A2;
const int midTapePin = A3;

// threshold - tape sensor TODO: NEED THESE VALUES THROUGH TESTING
int blackthreshold = 5;

// SLIDER SWITCH NEEDS
const int AsidePin = 12; //the switch connect to pin 12
int buttonA = 0; 
const int BsidePin = 13; //the switch connect to pin 12
int buttonB = 0; 
boolean sideA;

// LIMIT SWITCH NEEDS
const int limitPinR = 7;
int limitR;
const int limitPinL = 4;
int limitL;

// State Machine
int state = 0;

// Turning TODO: NEED TO SET THESE VALUES FROM TESTING
int turnIntervalLeft = 1000;
int turnIntervalRight = 1300;
int doubleTurnIntervalRight = 500;
int doubleTurnIntervalLeft = 500;
int turnIntervalTest = 1000;
boolean turnComplete = false;
void turnTimerHandler(){
  turnComplete = true;
}

// Backwards TODO: NEED TO SET VALUES FROM TESTING
int backwardsInterval = 800;
int backInterval2 = 300;
boolean backComplete = false;
void backwardsTimerHandler(){
  backComplete = true;
}

// Servo timers TODO: NEED TO SET VALUES FROM TESTING
int servoInterval = 100;
boolean servoComplete = false;
void servoTimerHandler(){
  servoComplete = true;
}

// Small Forward timer TODO: NEED TO SET VALUES FROM TESTING
int smallForwardInterval = 0;
boolean smallForward = false;
void smallForwardTimerHandler(){
  smallForward = true;
}

// speed values
int baseSpeed = 6;

// Reverse timer TODO: NEED TO SET VALUES FROM TESTING
int reverseInterval = 0;


boolean firstVal = false;
boolean secondVal = false;

// for celebration
boolean open = true;
void celebrationTimerHandler(){
  if (open){
    // close gates
    delay(20);
    toMotor(41);
  }
  else{
    delay(20);
    toMotor(42);
  }
  open = !open;
}



void setup() {
  Serial.begin(9600); // Starts the serial communication

  // ultrasonic sensor setup
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  ITimer1.init();
  ITimer1.attachInterrupt(TIMER_FREQ_HZ * 2, TimerHandler1);
  ITimer2.init();

  // slider switch setup
  pinMode(AsidePin, INPUT);
  pinMode(BsidePin, INPUT);

  // limit switch setup
  pinMode(limitPinR, INPUT);
  pinMode(limitPinL, INPUT);

  // begin communication with motor control arduino
  Wire.begin();

  toMotor(10);
  delay(10);
  toMotor(20);
  delay(10);
  toMotor(41);
}

void loop() {
  // Serial.println(state);
  if (state == 0){ // IDLE state
    // Serial.println("0");

    // ready slide switch inputs
    buttonA = digitalRead(AsidePin);
    buttonB = digitalRead(BsidePin);


    // if only one switch is up, set the appropriate side and advance state
    if (buttonA == HIGH && buttonB == LOW){
      sideA = true;
      state++;
      // Serial.println("sideA");
      // TURN Right MOTOR ON
      toMotor(20+(baseSpeed-2));
      toMotor(-10-(baseSpeed-2));
    }
    else if (buttonA == LOW && buttonB == HIGH){
      sideA = false;
      state++;
      // Serial.println("sideB");
      // TURN left MOTOR ON
      toMotor(10+(baseSpeed-1));
      toMotor(-20-(baseSpeed-1));
    }
  }
  else if (state == 1){ // Orientation_spin state
    int dist = readUltrasonic();
    // Serial.print("distance: ");
    // Serial.println(dist);

    if (dist > startzonethresholdmin && dist < startzonethresholdmax){
      if (firstVal){
        if (secondVal){
          // turn the other motor on
          if (sideA){
            // TURN left MOTOR ON
            toMotor(10 + baseSpeed);
            delay(20);
            toMotor(20 + baseSpeed);
          }
          // advance to next state;
          state++;
          firstVal = false;
          secondVal = false;
        }
        else{
          secondVal = true;
          if (!sideA){
            // TURN right MOTOR ON
            delay(20);
            toMotor(20 + baseSpeed);
            delay(20);
            toMotor(10+baseSpeed);
            state++;
            firstVal = false;
            secondVal = false;
          }
        }
      }
      else {
        firstVal = true;
      }

    }
    else{
      firstVal = false;
      secondVal = false;
    }

  }
  else if (state == 2){ // align_start_exit state
    int dist = readUltrasonic();
    // Serial.print("distance: ");
    // Serial.println(dist);

    if (dist < exittowallthreshold){
      if (firstVal){
        if (secondVal){
          // turn the other motor off
          if (sideA){
            // TURN RIGHT MOTOR OFF
            delay(10);
            toMotor(20);
            turnComplete = false;
            // ITimer2.attachInterruptInterval(turnIntervalRight, turnTimerHandler);

          }
          else{
            // TURN LEFT MOTOR OFF
            delay(10);
            toMotor(10);
          }
          // advance to next state;
          state++;
        }
        else{
          secondVal = true;
        }
      }
      else{
        firstVal = true;
      }

    }
    else{
      firstVal = false;
      secondVal = false;
    }

  }
  else if (state == 3){ // turn_to_exit state
    int dist = readUltrasonic();
    Serial.print("distance: ");
    Serial.println(dist);

    // if (turnComplete){
      // once you've turned towards the exit, turn stopped motor on to drive forward
      if (dist > startexitthreshold){
        if (firstVal){
          if (secondVal){
            if (sideA){
              // TURN RIGHT MOTOR ON
              toMotor(20 + baseSpeed);
            }
            else{
              // TURN LEFT MOTOR ON
              toMotor(10 + baseSpeed);
            }
            // start timer
            ITimer2.attachInterruptInterval(exitInterval, ExitStartTimerHandler);
            state ++;
          }
          else  {
            secondVal = true;
          }
        }
        else{
          firstVal = true;
        }   

      }
      else{
        firstVal = false;
        secondVal = false;
      }   
    // }
  }
  else if (state == 4){ // exit_start state
    // once timer is expired, go to next state
    int dist = readUltrasonic();
    Serial.print("distance: ");
    Serial.println(dist);
    // if (exitedStart){
    //   state++;
    // }

    if (dist < 100){
      state++;
    }
  }
  else if(state == 5){ // search_line state
    // read tape sensor input
    int irvalueMiddle = analogRead(midTapePin);
    // Serial.println(irvalue);
    int irvalLeft = analogRead(leftTapePin);
    int irvalRight = analogRead(rightTapePin);

    // if you sense line
    if (irvalueMiddle < blackthreshold || irvalLeft < blackthreshold || irvalRight < blackthreshold){
      delay(500);
      if (sideA){
        // TURN LEFT MOTOR OFF
        toMotor(10);
        // start turn timer
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalLeft, turnTimerHandler);
      }
      else{
        // TURN RIGHT MOTOR OFF
        toMotor(20);
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalRight, turnTimerHandler);
      }

      state++;
    }
  }
  else if (state == 6){ // turn_towards_contact
    // once timer is expired, go to next state
    if (turnComplete == true){
      state++;
      turnComplete = false; // reset turnComplete for the next use of turn timer
    }
  }
  else if (state == 7){ // drive_towards_contact

    // read tape sensor input
    // int midTape = analogRead(midTapePin);
    // int rightTape = analogRead(rightTapePin);
    // int leftTape = analogRead(leftTapePin);
    // Serial.print(leftTape);
    // Serial.print(" | ");
    // Serial.print(midTape);
    // Serial.print(" | ");
    // Serial.println(rightTape);

    // LINE FOLLOWING

    // // if left senses line and right doesnt
    // if (leftTape < blackthreshold && rightTape > blackthreshold){
    //   // right motor faster
    //   toMotor(20 + baseSpeed + 1);
    // }
    // // if right senses line and left doesn't
    // else if (rightTape < blackthreshold && leftTape > blackthreshold){
    //   // left motor faster
    //   toMotor(10 + baseSpeed +1);
    // }
    // // in all other cases drive forward
    // else{
    //   toMotor(10 + baseSpeed);
    //   toMotor(20 + baseSpeed);
    // }

    if (sideA){
      // TURN LEFT MOTOR on
      delay(10);
      toMotor(10+baseSpeed);
    }
    else{
      // TURN RIGHT MOTOR On
      toMotor(20 + baseSpeed);
    }

    // read limit switch values
    limitR = digitalRead(limitPinR);
    limitL = digitalRead(limitPinL);
    // if limit switch is hit
    if (limitR == HIGH || limitL == HIGH){
      Serial.println("limit");

      delay(10);
      toMotor(10);
      delay(10);
      toMotor(20);

      delay(1000);
      // move backwards slightly //THIS doesn't work
      delay(10);
        toMotor(-10 - baseSpeed);
        delay(10);
        toMotor(-20-baseSpeed);

      // start backwards timer
      ITimer2.attachInterruptInterval(backwardsInterval, backwardsTimerHandler);

      // move to next state
      state++;
    }

  }
  else if (state == 8){ // slight_backup
    // once timer is expired
    if (backComplete == true){
      delay(10);
      toMotor(10);
      delay(10);
      toMotor(20);
      delay(1000);
      // start turning
      if (sideA){
        // left motor forward
        toMotor(10 + baseSpeed);
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalRight-600, turnTimerHandler);

      }
      else{
        // left motor forward
        toMotor(20 + baseSpeed);
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalLeft, turnTimerHandler);

      }
      // start turn timer
      state++;
      backComplete = false; // reset backComplete for next use of backwards timer
    }
  }
  else if (state == 9){ // fake_servo_state
    // once timer is expired, turn drive motors off
    if (turnComplete == true){

      int dist = readUltrasonic();
      // Serial.print("distance: ");
      // Serial.println(dist);
      if (dist < 72){
      
        delay(10);
        toMotor(10);
        delay(10);
        toMotor(10);
        delay(1000);
        // turn servo NEED TO DIFFERENTIATE SIDE
        toMotor(33);
        // start servo timer
        ITimer2.attachInterruptInterval(servoInterval, servoTimerHandler);
        // move to next state
        state++; 
        turnComplete = false; // reset turnComplete for the next use of turn timer
      }

    }
  }
  else if (state == 10){ // drive_to_far_wall
    // if done making contact
    if (servoComplete == true){
      // turn servo back
      toMotor(31);
      // turn motors on
      delay(10);
      toMotor(10+baseSpeed+1);
      delay(10);
      toMotor(20+baseSpeed);

      // if hit limit switches
      // read limit switch values
      limitR = digitalRead(limitPinR);
      limitL = digitalRead(limitPinL);
      // if limit switch is hit
      if (limitR == HIGH || limitL == HIGH){
        delay(10);
        toMotor(10);
        delay(10);
        toMotor(20);
        delay(1000);

        Serial.println("limit");
        // move backwards slightly
        toMotor(-10 - baseSpeed);
        delay(10);
        toMotor(-20 - baseSpeed);
        // start backwards timer
        backComplete = false;
        ITimer2.attachInterruptInterval(backInterval2, backwardsTimerHandler);

        // move to next state
        state++;
      }

    }
  }
  else if (state == 11){// backing up
    // once timer is expired
    if (backComplete == true){
      delay(10);
      toMotor(20);
      delay(10);
      toMotor(10);
      delay(1000);
      if (sideA){
        // right motor on
        toMotor(20 +baseSpeed);
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalTest-200, turnTimerHandler);

      }
      else{
        // left motor on
        toMotor(10+baseSpeed);
        turnComplete = false;
        ITimer2.attachInterruptInterval(turnIntervalTest, turnTimerHandler);

      }

      // start turn timer
      state++;
      backComplete = false; // reset backComplete for next use of backwards timer
    }
  }
  else if (state == 12){ // turn_towards_goal''
    if (turnComplete == true){
      delay(10);
      toMotor(10);
      delay(10);
      toMotor(20);
      delay(1000);

      if (sideA){
        delay(10);
        toMotor(10+baseSpeed+3);
        delay(10);
        toMotor(20+baseSpeed);
      }
      else{
        toMotor(20+baseSpeed);
        toMotor(10+baseSpeed);
      }

      state++;
    }

  }
  else if (state == 13){ // drive_towards_goal
    // if hit limit switches
    // read limit switch values
    limitR = digitalRead(limitPinR);
    limitL = digitalRead(limitPinL);
    // if limit switch is hit
    if (limitR == HIGH || limitL == HIGH){
      delay(2000);
      Serial.println("limit");
      delay(10);
      toMotor(10);
      delay(10);
      toMotor(20);
      delay(1000);
      // move to next state
      state++;
    }
  }
  else if (state == 14){ // open servo
  delay(1000);
    toMotor(42);
    open = true;
    delay(1000);
    state++;
  }

  else { // celebration
    Serial.println("celebrate");
    delay(1000);
    if (open){
      toMotor(41);
    }
    else{
      toMotor(42);
    }
    open = !open;
  }

}



  // else if (state == 12){ // slightly_forward
  //   // once timer is expired, drive forward slightly
  //   if (turnComplete == true){
  //     toMotor(10+baseSpeed);
  //     toMotor(20 + baseSpeed);
  //     // start slightly forward timer
  //     ITimer2.attachInterruptInterval(smallForwardInterval, smallForwardTimerHandler);
  //     // move to next state
  //     state++; 
  //     turnComplete = false; // reset turnComplete for the next use of turn timer
  //   }
  // }
  // else if (state == 13){ // turn_180
  //   // if timer is expired, turn 180 degrees
  //   if (smallForward == true){
  //     if (sideA){
  //       // left forward, right reverse
  //       toMotor(10+baseSpeed);
  //       toMotor(-20-baseSpeed);
  //     }
  //     else{
  //       // right forward, left reverse
  //       toMotor(-10-baseSpeed);
  //       toMotor(20+baseSpeed);
  //     }
  //     // start reverse timer
  //     ITimer2.attachInterruptInterval(reverseInterval, turnTimerHandler);
  //     // move to next state
  //     state++; 
  //   }
  // }
  // else if (state == 14){ // drive_towards_goal
  //   // if done turning
  //   if (turnComplete == true){
  //     // drive forward
  //     toMotor(10+baseSpeed);
  //     toMotor(20+baseSpeed);

  //     // if hit limit switches
  //     // read limit switch values
  //     limit = digitalRead(limitPin);
  //     // if limit switch is hit
  //     if (limit == HIGH){
  //       Serial.println("limit");
  //       // stop
  //       toMotor(10);
  //       toMotor(20);

  //       // open gate servo
  //       toMotor(42);

  //       // move to next state
  //       state++;
  //     }
  //   }
  // }
