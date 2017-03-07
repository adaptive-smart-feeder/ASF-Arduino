#include <Q2HX711.h>
#include <SoftwareSerial.h>
#include <AccelStepper.h>
#include "SimpleTimer.h"

#define HALFSTEP 8
#define motorPin1  9
#define motorPin2  10
#define motorPin3  11
#define motorPin4  12

AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

Q2HX711 hx711(A3, A2);

long zero = 83843;
long weightBeforeActivation = 0;

bool isUnderHole = false;
bool shouldMove = false;

int target = -1075;
int desiredMass = 0;
int massOfPlate = 135;

SimpleTimer timer;

long getWeight() {

  long diff = (zero - hx711.read() / 100);
  diff = (diff / 3.08 - massOfPlate);
  return diff > 0 ? diff : 0;
}

void repeatMe() {
  
    Serial.print("Uptime (s): ");
    Serial.println(millis() / 1000);
}

SoftwareSerial mySerial(7, 8);

void setup() {
  
  Serial.begin(9600);
  mySerial.begin(9600);

  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(200.0);
  stepper1.setSpeed(300);

  weightBeforeActivation = getWeight();

  timer.setInterval(1000, repeatMe);
}

void controlMotor() {

  if(stepper1.distanceToGo() == 0 && shouldMove) {
    if(!isUnderHole) {
      delay(100);
      long currentWeight = getWeight();
      if(desiredMass > currentWeight) {
        target *= -1;
        stepper1.moveTo(stepper1.currentPosition() + target);
      } else {
        shouldMove = false;
        desiredMass = 0;
      }
      delay(200);
    } else {
      delay(1000);
      target *= -1;
      stepper1.moveTo(stepper1.currentPosition() + target);
    }
    isUnderHole = !isUnderHole;
  }
  stepper1.run();
}

void loop() {

  int c;

  controlMotor();

  if(desiredMass <= 0 && !isUnderHole && !shouldMove)
      weightBeforeActivation = getWeight();

  if(mySerial.available()) {
    
    String comando = mySerial.readString();

    if(comando.startsWith("ac ")) {
      comando.remove(0, 3);
      String mode = comando.substring(0,1);
      int massAsked = comando.substring(2).toInt();
      
      long weight = getWeight();

      desiredMass = mode == "0" ? massAsked: massAsked + weight;
      if(desiredMass > weight) {
        shouldMove = true;
        isUnderHole = false;
      }
    } else if (comando.startsWith("auto")) {
      
    } else if (comando.startsWith("sche")) {
      
    }
  }
} 
