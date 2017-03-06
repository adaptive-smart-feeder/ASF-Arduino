#include <Q2HX711.h>
#include <SoftwareSerial.h>
#include <AccelStepper.h>
#include <string.h>

#define LED_PIN 2
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1  9      // IN1 on the ULN2003 driver 1
#define motorPin2  10     // IN2 on the ULN2003 driver 1
#define motorPin3  11     // IN3 on the ULN2003 driver 1
#define motorPin4  12     // IN4 on the ULN2003 driver 1

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

Q2HX711 hx711(A3, A2);

long zero = 83843;
long weightBeforeActivation = 0;

bool isUnderHole = false;
bool shouldMove = false;

int target = -1075;
int desiredMass = 0;
int massOfPlate = 135;

long getWeight() {

  long diff = (zero - hx711.read() / 100);
  diff = (diff / 3.08 - massOfPlate);
  return diff > 0 ? diff : 0;
}

// Esquemático:
// https://evothings.com/control-an-led-using-hm-10-ble-module-an-arduino-and-a-mobile-app/

SoftwareSerial mySerial(7, 8); // RX, TX  
// Connect HM10      Arduino Uno
//     Pin 1/TXD          Pin 7
//     Pin 2/RXD          Pin 8

String command[10];

void split(char *string) {

  char *p, *q;

  p = strtok_r(string, " ", &q);

  for(int i = 0; p != NULL; i++) {
    command[i] = String(p);
    p = strtok_r(NULL, " ", &q);
  }
}

void setup() {
  
  Serial.begin(9600);
  mySerial.begin(9600);

  // configurar portas
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("nasceu");

  //Stepper's setup
  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(200.0);
  stepper1.setSpeed(300);

  weightBeforeActivation = getWeight();
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
  //stepper1.run();

  if(desiredMass <= 0 && !isUnderHole && !shouldMove)
      weightBeforeActivation = getWeight();

  if(/*my*/Serial.available()) {
    
    /*  Comandos vindos do iPhone vem em forma de string.
     *  É necessário definir o formato dos comandos para
     *  que não haja uma desinteligência entre o iPhone e
     *  o Arduino.
     *  Necessário também formatar a entrada para extrair
     *  valores, como em comandos na forma "acionar %d"
     *  (Onde pode morar a treta).
     */
    String comando = /*my*/Serial.readString();

    Serial.print("comando: ");
    Serial.println(comando);

    // porrada de if a vir
    if(comando == "on") {
      digitalWrite(LED_PIN, HIGH);
    } else if(comando == "off") {
      digitalWrite(LED_PIN, LOW);
    } else if(comando.startsWith("ac ")) {
      comando.remove(0, 3);
      String mode = comando.substring(0,1);
      int massAsked = comando.substring(2).toInt();
      Serial.print("  massAsked = ");
      Serial.println(massAsked);

      long weight = getWeight();

      //0: exatamente; 1: a mais
      desiredMass = mode == "0" ? massAsked: massAsked + weight;
      if(desiredMass > weight) {
        shouldMove = true;
        isUnderHole = false;
      }
    }
    //Debugs
    else if (comando == "we"){
      Serial.print("\n\n----------\n\n");
      Serial.print(getWeight());
      Serial.print(" g\n\n----------\n\n");
      delay(250);
    } else if(comando == "sh") {
      Serial.print("\n\n----------\n\n");
      Serial.print(shouldMove);
      Serial.print("\n\n----------\n\n");
    } else if(comando == "is") {
      Serial.print("\n\n----------\n\n");
      Serial.print(isUnderHole);
      Serial.print("\n\n----------\n\n");
    } else if(comando == "de") {
      Serial.print("\n\n----------\n\n");
      Serial.print(desiredMass);
      Serial.print(" g\n\n----------\n\n");
    } else if(comando == "mo") {
      target *= -1;
      stepper1.moveTo(stepper1.currentPosition() + target);
    } else {
      Serial.println("Mas nada aconteceu");
    }
  }
} 
