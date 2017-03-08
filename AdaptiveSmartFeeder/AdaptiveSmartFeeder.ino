#include <Time.h>
#include <TimeLib.h>
#include <Q2HX711.h>
#include <SoftwareSerial.h>
#include <AccelStepper.h>
#include "SimpleTimer.h"
#include <string.h>

#define HALFSTEP 8
#define motorPin1  9
#define motorPin2  10
#define motorPin3  11
#define motorPin4  12

typedef struct time {
  int minutes, hours;
} Time;

typedef struct Schedule {

  int id;
  int weight;
  int days[7] = {0, 0, 0, 0, 0, 0, 0};
  Time time;
  bool isActivated;
} Schedule;

Schedule schedules[10];
int numberOfSchedules = 0;

long zero = 83843;
long weightBeforeActivation = 0;

bool isUnderHole = false;
bool shouldMove = false;

int target = -1075;
int desiredMass = 0;
int massOfPlate = 135;
int timerId = 0;

SimpleTimer timer;

AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

Q2HX711 hx711(A3, A2);

SoftwareSerial mySerial(7, 8);

long getWeight() {

  long diff = (zero - hx711.read() / 100);
  diff = (diff / 3.08 - massOfPlate);
  return diff > 0 ? diff : 0;
}

void checkAutomatic() {
  
    Serial.print("Uptime (s): ");
    Serial.println(millis() / 1000);
    Serial.print("-----\n P = ");
    Serial.println(getWeight());
    Serial.println("-----");
}

void checkSchedule() {
  //testa se é o horário de liberar em todos os schedules
}

String command[10];

void split(char *string) {

  char *p, *q;

  p = strtok_r(string, " ", &q);

  for(int i = 0; p != NULL; i++) {
    command[i] = String(p);
    p = strtok_r(NULL, " ", &q);
  }
}

void check() {
  
  Serial.print("\nTime: ");
  Serial.print(minute());
  Serial.print(" : ");
  Serial.println(second());
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

void handleActivation(int massAsked, String mode) {

  long weight = getWeight();

  long desiredMass = mode == "0" ? massAsked: massAsked + weight;
  if(desiredMass > weight) {
    shouldMove = true;
    isUnderHole = false;
  }
}

void handleSchedule(int id, int hours, int minutes, int isActivated, int days[7]) {

  bool isNew = true;
  int i;
  
  Schedule schedule;
  schedule.id = id;
  schedule.time.hours = hours;
  schedule.time.minutes = minutes;
  schedule.isActivated = isActivated ? true : false;
  for(i = 0; i < 7; i++)
     schedule.days[i] = days[i];
  
  for(i = 0; i < numberOfSchedules; i++)
    if(schedules[i].id == id) {
      isNew = false;
      break;
    }

  if(isNew) {
    schedules[numberOfSchedules++] = schedule;
  } else {
    schedules[i] = schedule;
  }
}

void handleAutomatic(String comando) {

  
}

void setup() {

  Serial.begin(9600);
  mySerial.begin(9600);

  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(200.0);
  stepper1.setSpeed(300);

  weightBeforeActivation = getWeight();

  timerId = timer.setInterval(1000, check);

  setTime(0, 0, 0, 0, 0, 0);
}

void loop() {

  int c;

  controlMotor();

  timer.run();

  if(desiredMass <= 0 && !isUnderHole && !shouldMove)
      weightBeforeActivation = getWeight();

  if(/*my*/Serial.available()) {
    
    String comando = /*my*/Serial.readString();

    if(comando.startsWith("acti")) {
      String mode = comando.substring(3, 4);
      int massAsked = comando.substring(4).toInt();
      
      handleActivation(massAsked, mode);
    } else if (comando.startsWith("auto")) {
      //handleAutomatic(comando);
    } else if (comando.startsWith("sche")) {
      if(numberOfSchedules < 10) {
        
      }
    } else {
      Serial.println("Mas nada aconteceu");
    }
  }
} 
