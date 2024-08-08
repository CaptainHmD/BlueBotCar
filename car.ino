#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial BTSerial(9, 10);  // RX, TX

Servo servo;
const int trigPin = 13;  // ultrasonic
const int echoPin = 12;  // ultrasonic

const int servoPin = 11;

const int enAPin = 6;
const int in1Pin = 7;
const int in2Pin = 5;
const int in3Pin = 4;
const int in4Pin = 2;
const int enBPin = 3;

enum Motor { LEFT,
             RIGHT };
enum State { STOP,
             WALK } currentState;
enum Mode {
  AUTO,
  CONTROLLER
} currentMode;

void go(enum Motor m, int speed) {
  Serial.print("go(): Motor: ");
  Serial.print(m == LEFT ? "LEFT" : "RIGHT");
  Serial.print(", speed: ");
  Serial.println(speed);

  if (m == RIGHT) {
    digitalWrite(in1Pin, speed > 0 ? LOW : HIGH);
    digitalWrite(in2Pin, speed <= 0 ? LOW : HIGH);
    analogWrite(enAPin, speed < 0 ? -speed : speed);
  } else {
    digitalWrite(in3Pin, speed > 0 ? LOW : HIGH);
    digitalWrite(in4Pin, speed <= 0 ? LOW : HIGH);
    analogWrite(enBPin, speed < 0 ? -speed : speed);
  }
}


void testMotors() {
  static int speed[8] = { 128, 255, 128, 0, -128, -255, -128, 0 };
  go(RIGHT, 0);
  for (unsigned char i = 0; i < 8; i++)
    go(LEFT, speed[i]), delay(200);
  for (unsigned char i = 0; i < 8; i++)
    go(RIGHT, speed[i]), delay(200);
}

unsigned int readDistance() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long period = pulseIn(echoPin, HIGH);
  return period * 343 / 2000;
}

#define NUM_ANGLES 7
#define NUM_STOP_ANGLES 4
unsigned char sensorWalkAngle[NUM_ANGLES] = { 60, 70, 80, 90, 100, 110, 120 };
unsigned int distance[NUM_ANGLES];

void readNextDistance() {
  static unsigned char angleIndex = 0;
  static signed char step = 1;
  distance[angleIndex] = readDistance();
  angleIndex += step;
  if (angleIndex == NUM_ANGLES - 1)
    step = -1;
  else if (angleIndex == 0)
    step = 1;
  servo.write(sensorWalkAngle[angleIndex]);
}
void setup() {
  Serial.begin(9600);    // Starts the serial communication
  BTSerial.begin(9600);  // Initialize Bluetooth Serial at 9600 baud
  Serial.println("JDY-31 Bluetooth Module Setup");

  // Set up the pins for the ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  // Set up the pins for the motors
  pinMode(enAPin, OUTPUT);
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(in3Pin, OUTPUT);
  pinMode(in4Pin, OUTPUT);
  pinMode(enBPin, OUTPUT);


  // Attach the servo to the servoPin
  servo.attach(servoPin);
  // Set the servo to the middle position (90 degrees)

  servo.write(90);
  // Initialize the motors to stop
  go(LEFT, 0);
  go(RIGHT, 0);
  testMotors();

  delay(200);
  servo.write(sensorWalkAngle[0]);
  delay(200);
  for (unsigned char i = 0; i < NUM_ANGLES; i++)
    readNextDistance(), delay(200);


  // Initialize the current mode
  currentMode = CONTROLLER;
  Serial.print("Initial Mode: ");
  Serial.println(currentMode == AUTO ? "AUTO" : "CONTROLLER");
}
void loop() {
  if (BTSerial.available()) {
    Serial.println("loop");
    char data = BTSerial.read();  // Read the incoming data from Bluetooth
    Serial.print("Received: ");
    Serial.println(data);  // Print the received data to Serial Monitor
                           // Add Mode
    switch (data) {

      case 'A':
        currentMode = AUTO;
        break;
      case 'P':
        currentMode = CONTROLLER;
        break;
    }
    if (currentMode == CONTROLLER) {
      switch (data) {
        case 'F':  // Forward
          go(LEFT, 200);
          go(RIGHT, 220);
          Serial.print("case F: ");
          break;
        case 'L':  // Left
          go(LEFT, -180);
          go(RIGHT, 180);
          Serial.print("case L: ");
          break;
        case 'R':  // Right
          go(LEFT, 180);
          go(RIGHT, -180);
          Serial.print("case R: ");
          break;
        case 'B':  // Backward
          go(LEFT, -180);
          go(RIGHT, -180);
          Serial.print("case B: ");
          break;
        case '0':  // Stop
          go(LEFT, 0);
          go(RIGHT, 0);
          Serial.print("case 0: ");
          break;
      }
    }
    delay(20);
  } else if (currentMode == AUTO) {
    Serial.println("else");
    bool tooClose = false;
    readNextDistance();
    for (unsigned char i = 0; i < NUM_ANGLES; i++) {
      Serial.print("distance[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(distance[i]);
      if (distance[i] < 300)
        tooClose = true;
    }
    if (tooClose) {
      Serial.println("Obstacle detected, moving backward");
      go(LEFT, -180);
      go(RIGHT, 100);
    } else {
      Serial.println("Path clear, moving forward");
      go(LEFT, 180);
      go(RIGHT, 200);
    }
    delay(20);
  }
}
