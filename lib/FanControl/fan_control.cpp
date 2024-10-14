#include "fan_control.h"
#include <Arduino.h>

// Pin für den Lüfter (z.B. PWM-Pin)
#define FAN_PIN 5

// Initialisiere den Lüfter-Pin
void setupFan() {
  pinMode(FAN_PIN, OUTPUT);
}

// Setze die Lüftergeschwindigkeit (Wert zwischen 0 und 100)
void setFanSpeed(int speed) {
  // Konvertiere den Prozentwert in einen PWM-Wert (0-255)
  int pwmValue = map(speed, 0, 100, 0, 255);
  
  // Setze den PWM-Wert für den Lüfter
  analogWrite(FAN_PIN, pwmValue);
}
