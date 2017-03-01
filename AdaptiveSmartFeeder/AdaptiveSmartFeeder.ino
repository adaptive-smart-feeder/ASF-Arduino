#include <SoftwareSerial.h>
#include <AccelStepper.h>

#define LED_PIN 2
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1  9     // IN1 on the ULN2003 driver 1
#define motorPin2  10     // IN2 on the ULN2003 driver 1
#define motorPin3  11     // IN3 on the ULN2003 driver 1
#define motorPin4  12     // IN4 on the ULN2003 driver 1

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

// Esquemático:
// https://evothings.com/control-an-led-using-hm-10-ble-module-an-arduino-and-a-mobile-app/

SoftwareSerial mySerial(7, 8); // RX, TX  
// Connect HM10      Arduino Uno
//     Pin 1/TXD          Pin 7
//     Pin 2/RXD          Pin 8

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  //mySerial.print("AT+SHOW1");

  // configurar portas
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("nasceu");
}

void loop() {  
  int c;

  if(mySerial.available()) {
    
    /*  Comandos vindos do iPhone vem em forma de string.
     *  É necessário definir o formato dos comandos para
     *  que não haja uma desinteligência entre o iPhone e
     *  o Arduino.
     *  Necessário também formatar a entrada para extrair
     *  valores, como em comandos na forma "acionar %d"
     *  (Onde pode morar a treta).
     */
    String comando = mySerial.readString();

    Serial.print("comando: ");
    Serial.println(comando);

    // porrada de if a vir
    if(comando == "on") {
      digitalWrite(LED_PIN, HIGH);
    }
    else if(comando == "off") {
      digitalWrite(LED_PIN, LOW);
    }
    else if(comando.startsWith("ac ")) {
      comando.remove(0, 3);
      int a = comando.toInt() + 3;
      Serial.print("  Num = ");
      Serial.println(a);
    }
    else {
      Serial.println("Mas nada aconteceu");
      mySerial.println("oi");
    }
    
  }
} 
