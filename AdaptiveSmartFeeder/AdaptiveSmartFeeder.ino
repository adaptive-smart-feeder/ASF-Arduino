#include <Time.h>
#include <TimeLib.h>
#include <Q2HX711.h>
#include <SoftwareSerial.h>
#include <AccelStepper.h>
#include "SimpleTimer.h"
#include <string.h>

#define HALFSTEP 8
#define FEED_PERIOD 144
#define PERIOD 7
#define MAX 5
#define motorPin1  9
#define motorPin2  10
#define motorPin3  11
#define motorPin4  12

class Time {

public:

  // Attributes
  int hours, minutes;

  // Constructor
  Time(int hours, int minutes) {
    this->hours = hours;
    this->minutes = minutes;
  }

  Time() : Time(0,0) {};

  // Add time of 'other' to current Time
  void addTime(Time other) {
    this->hours = (this->hours + other.hours + (this->minutes + other.minutes >= 60) % 24);
    this->minutes = (this->minutes + other.minutes) % 60;
  }

  // Return time of 'other' added to the current Time
  Time addedTime(Time other) {
    int hours = (this->hours + other.hours + (this->minutes + other.minutes >= 60) % 24);
    int minutes = (this->minutes + other.minutes) % 60;
    return Time(hours, minutes);
  }

  // Returns 0 if times are equal, -1 if current time is lower, and 1 otherwise
  int compare(Time other) {
    if(this->hours == other.hours && this->minutes == other.minutes)
      return 0;
    if(this->hours < other.hours || (this->hours == other.hours && this->minutes < other.minutes))
      return -1;
    return 1;
  }

};

class Feed {

public:

  int quantity;
  Time time;

  Feed(int quantity, Time time) {
    this->quantity = quantity;
    this->time = time;
  }
  
};

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
int timeWeekDay;

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
  
  Serial.print("\nTime: ");
  Serial.print(hour());
  Serial.print(" : ");
  Serial.print(minute());
  Serial.print(" : ");
  Serial.println(second());

  checkSchedule();
}
  
  void controlMotor() {
  
    if(stepper1.distanceToGo() == 0 && shouldMove) {
      //Serial.println("MOTOR");
      if(!isUnderHole) {
        //Serial.println("MOTOR N");
        delay(100);
        long currentWeight = getWeight();
        if(desiredMass > currentWeight) {
          target *= -1;
          stepper1.moveTo(stepper1.currentPosition() + target);
        } else {
          shouldMove = false;
          desiredMass = 0;
          //Serial.println("MOTOR ELSE");
        }
        delay(200);
      } else {
        //Serial.println("MOTOR S");
        delay(1000);
        target *= -1;
        stepper1.moveTo(stepper1.currentPosition() + target);
      }
      isUnderHole = !isUnderHole;
    }
    stepper1.run();
  }
  
  void handleActivation(int massAsked, int mode) {
  
    Serial.print("\nmassAsked = ");
    Serial.println(massAsked);
    Serial.print("\nmode = ");
    Serial.println(mode);
  
    long weight = getWeight();
  
    desiredMass = mode == 0 ? massAsked: massAsked + weight;
    if(desiredMass > weight) {
      shouldMove = true;
      isUnderHole = false;
    }
  }
  
void handleSchedule(int id, int hours, int minutes, int weight, bool isActivated, int days[]) {
  
  bool isNew = true;
  int i = 0;
  
  Schedule schedule;
  schedule.id = id;
  schedule.weight = weight;
  schedule.time.hours = hours;
  schedule.time.minutes = minutes;
  schedule.isActivated = isActivated;
  Serial.println("OI 1");
  while(days[i] != -1)
    schedule.days[days[i++]] = 1;
    
  Serial.println("OI 2");
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
  Serial.println("Sched");
  Serial.println(schedule.time.hours);
  Serial.println(schedule.time.minutes);
}

void checkSchedule() {

  Serial.println("Entrou Sche");
  for(int i = 0; i < numberOfSchedules; i++)
    if(schedules[i].days[timeWeekDay] == 1 && schedules[i].time.hours == hour() && schedules[i].time.minutes == minute()) {
      handleActivation(schedules[i].weight, 0);
      Serial.println("Peso");
      Serial.println(schedules[i].weight);
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

  timerId = timer.setInterval(60000, check);

  setTime(0, 0, 0, 0, 0, 0);
}

void loop() {

  int c;

  controlMotor();

  timer.run();

  if(desiredMass <= 0 && !isUnderHole && !shouldMove)
      weightBeforeActivation = getWeight();

  if(mySerial.available()) {
    
    String comando = mySerial.readString();
    Serial.print("comando = ");
    Serial.println(comando);
    split(comando);

    if(command[0] == "ac") {
      
      int mode = command[1].toInt();
      int massAsked = command[2].toInt();
       
      handleActivation(massAsked, mode);
      
    } else if (command[0] == "au") {
      
      handleAutomatic(command[1]);
      
    } else if (command[0] == "sc") {
      
      int id = command[1].toInt();
      int hours = command[2].toInt();
      int minutes = command[3].toInt();
      int currentHours = command[4].toInt();
      int currentMinutes = command[5].toInt();
      int currentWeekDay = command[6].toInt();
      int weight = command[7].toInt();
      bool isActivated = (command[8].toInt() == 1 ? true : false);

      int days[8], i = 0;

      for(i = 0; command[i+9].toInt() != -1; i++) {
        days[i] = command[i+9].toInt();
      }
      days[i] = -1;

      setTime(currentHours, currentMinutes, 0, 1, 1, 2017);
      timeWeekDay = currentWeekDay;
      handleSchedule(id, hours, minutes, weight, isActivated, days);
          
    } else if (command[0] == "dt") {
      
      Serial.println("\ndt");
      int hours = command[1].toInt();
      int minutes = command[2].toInt();
      setTime(hours, minutes, 0, 1, 1, 2017);
      
    } else {
      Serial.println("Mas nada aconteceu");
    }
  }
} 
