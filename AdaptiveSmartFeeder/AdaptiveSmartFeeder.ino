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
} Schedule;

//Date scheduled[10];

long zero = 83843;
long weightBeforeActivation = 0;

bool isUnderHole = false;
bool shouldMove = false;

int target = -1075;
int desiredMass = 0;
int massOfPlate = 135;

SimpleTimer timer;

String command[20];

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


void split(String str) {

  char string[50];
  char *p, *q;

  str.toCharArray(string, 50);

  p = strtok_r(string, " ", &q);

  for(int i = 0; p != NULL; i++) {
    command[i] = String(p);
    p = strtok_r(NULL, " ", &q);
  }
}

void check() {
  Serial.print("\nHora ");
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

void handleSchedule(String comando) {

  int spaces[10];
  int n = 0;
  int i = 0;
  for (char c = comando[0]; c; c = comando[i++])
    if (c == ' ')
      spaces[n++] = i;
  for(i = 1; i <= n; i++) {
    Serial.println(comando.substring(spaces[i - 1], spaces[i]));
//    if(i%4 == 0) {
//      scheduled[i/4].sec = comando.substring(spaces[i - 1], spaces[i]).toInt();
//    } else if(i%4 == 1) {
//      scheduled[i/4].minu = comando.substring(spaces[i - 1], spaces[i]).toInt();
//    } else if(i%4 == 2) {
//      scheduled[i/4].hour = comando.substring(spaces[i - 1], spaces[i]).toInt();
//    } else {
//      scheduled[i/4].day = comando.substring(spaces[i - 1], spaces[i]).toInt();
//    }
  }
}

void handleAutomatic(String comando) {
  //Faz os tratamentos aí
}

void setup() {
  
  Serial.begin(9600);
  mySerial.begin(9600);

  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(200.0);
  stepper1.setSpeed(300);

  weightBeforeActivation = getWeight();

  timer.setInterval(1000, check);

  setTime(15, 6, 40, 8, 3, 2017);
}

void loop() {

  int c;

  controlMotor();

  timer.run();

  if(desiredMass <= 0 && !isUnderHole && !shouldMove)
      weightBeforeActivation = getWeight();

  if(/*my*/Serial.available()) {
    
    String comando = /*my*/Serial.readString();
    split(comando);

    if(command[0] == "ac") {
      String mode = command[1];
      int massAsked = command[2].toInt();
       
      handleActivation(massAsked, mode);

    } else if (command[0] == "au") {
      
      handleAutomatic(command[1]);
      
    } else if (command[0] == "sc") {

      int id = command[1].toInt();
      int hours = command[2].toInt();
      int minutes = command[3].toInt();
      int weight = command[4].toInt();
      bool isActivated = (command[5].toInt() == 1 ? true : false);
      
      int days[8], i = 0;

      for(i = 0; command[i+6].toInt() != -1; i++) {
        days[i] = command[i+6].toInt();
      }
      days[i] = -1;
      
      handleSchedule(id, hours, minutes, weight, isActivated, days);
      
    } else {
      Serial.println("Mas nada aconteceu");
    }
  }
} 
