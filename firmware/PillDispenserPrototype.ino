#include <Wire.h>
#include <Stepper.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

RTC_DS3231 rtc; //Objeto da classe real time clock;

LiquidCrystal lcd(A0, A1, A2, A3, 3, 4);

const int stepsPerRevolution = 256; // a full 360-degree output rotation requires 2048 steps.

const int luzLCD = 5;

const int buzzerPin = 12;

const int pushButton = 2;

Stepper stepper(stepsPerRevolution, 8, 9, 10, 11); //esses números são os ports do arduino

char diasDaSemana[7][12] = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"};
//char é um dado de 1 byte ou uma matriz de textos;

void setup() {

  Serial.begin(9600);

  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("Teste do LCD");

  lcd.setCursor(0, 1);
  lcd.print("Funcionando!");

  pinMode(luzLCD, OUTPUT);
  digitalWrite(luzLCD, LOW);

  stepper.setSpeed(32);

  pinMode(buzzerPin, OUTPUT);
  pinMode(pushButton, INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("RTC NAO INICIALIZADO");
    while (1); //Trava o programa;
  }

  //rtc.adjust(DateTime(2026, 10, 24, 17, 50, 00));
  delay(100); //milissegundos;

}

void loop() {

  DateTime agora = rtc.now(); // Faz a leitura de dados de data e hora

  int buttonState = digitalRead(pushButton);

  if (buttonState == LOW) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  Serial.print("Data: ");
  Serial.print(agora.day(), DEC);
  Serial.print('/');
  Serial.print(agora.month(), DEC);
  Serial.print('/');
  Serial.print(agora.year(), DEC);
  Serial.print(" / Dia da semana: ");
  Serial.print(diasDaSemana[agora.dayOfTheWeek()]);
  Serial.print(" / Hora: ");
  Serial.print(agora.hour(), DEC);
  Serial.print(':');
  Serial.print(agora.minute(), DEC);
  Serial.print(':');
  Serial.print(agora.second(), DEC);
  Serial.print(" / Temperatura: ");
  Serial.print(rtc.getTemperature());
  Serial.print(" *C");
  Serial.println();

  digitalWrite(luzLCD, HIGH);
  delay(1000); // Wait 1 second

  digitalWrite(luzLCD, LOW);
  delay(1000);

  motor();
  }

void motor() {
  Serial.println("Moving Clockwise...");
  stepper.step(stepsPerRevolution);

  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
}

