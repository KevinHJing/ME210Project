#include <PWMServo.h>
#include <Wire.h> 


PWMServo gateServo;  // create servo object to control a servo
PWMServo contactServo;

int pos = 0;    // variable to store the servo position

int recieve;

const int left1 = 2;
const int left2 = 4;
const int leftspeed = 11;

const int right1 = 6;
const int right2 = 7;
const int rightspeed = 3;

// processing recieve
int firstDigit;
int secondDigit;
boolean positive;

// servo positions
int contactOriginal= 60 ;
int contactA = 0;
int contactB = 120;
int gateDown=90;
int gateUp=180;

void setup() {
  Serial.begin(9600);

  gateServo.attach(SERVO_PIN_B);  // attaches the servo on pin 9 to the servo object
  contactServo.attach(SERVO_PIN_A); 

  // left motor
  pinMode(left1, OUTPUT);// direction
  pinMode(left2, OUTPUT);
  pinMode(leftspeed, OUTPUT); // speed 

  // right motor
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
  pinMode(rightspeed, OUTPUT);// speed

  // Start the I2C Bus as Slave on address 9
  Wire.begin(9); 
  // Attach a function to trigger when something is received.
  Wire.onReceive(receiveEvent);
}

void receiveEvent(int bytes) {
  recieve = Wire.read();
  if (recieve > 128){
    recieve = recieve -256;
  }
}

void loop() {

  Wire.onReceive(receiveEvent);
  Serial.println(recieve);

  // processing information
  if (recieve < 0){
    positive = false;
  }
  else{
    positive = true;
  }

  secondDigit = recieve % 10;
  firstDigit = (recieve - secondDigit) / 10;

  secondDigit = abs(secondDigit);
  firstDigit = abs(firstDigit);

  // turning on proper motor
  // left motor
  if (firstDigit == 1){
    Serial.print("left motor ");
    // set map to proper speed
    int speed = map(secondDigit, 0, 9, 80, 200);
    if (secondDigit == 0){
      speed = 0;
    }
    Serial.print(speed);
    analogWrite(leftspeed, speed);

    // if positive, go forwards
    if (positive){
      Serial.println(" forward");
      digitalWrite(left1, LOW);
      digitalWrite(left2, HIGH);
    }
    // if negative, go backwards
    else{
      Serial.println(" back");
      digitalWrite(left1, HIGH);
      digitalWrite(left2, LOW);
    }
  }
  else if (firstDigit == 2){
    Serial.print("right motor ");
    // set map to proper speed
    int speed = map(secondDigit, 0, 9, 80, 180);
    if (secondDigit == 0){
      speed = 0;
    }
    Serial.print(speed);
    analogWrite(rightspeed, speed);

    // if positive, go forwards
    if (positive){
      Serial.println(" forward");
      digitalWrite(right1, LOW);
      digitalWrite(right2, HIGH);
    }
    // if negative, go backwards
    else{
      Serial.println(" back");
      digitalWrite(right1, HIGH);
      digitalWrite(right2, LOW);
    }

  }
  // contact servo
  else if (firstDigit == 3){
    Serial.print("contact servo ");
    if (secondDigit == 1){
      Serial.println(" original position");
      contactServo.write(contactOriginal);
    }
    if (secondDigit == 2){
      Serial.println(" active position A");
      contactServo.write(contactA);
    }
    if (secondDigit == 3){
      Serial.println(" active position B");
      contactServo.write(contactB);
    }
  }
  // gate servo
  else if (firstDigit == 4){
    Serial.print("gate servo ");
    if (secondDigit == 1){
      Serial.println(" down");
      gateServo.write(gateDown);
    }
    if (secondDigit == 2){
      Serial.println(" up");
      gateServo.write(gateUp);
    }
  }
  

}