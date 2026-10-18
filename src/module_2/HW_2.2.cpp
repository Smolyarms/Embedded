#include <Arduino.h>

const uint8_t RELAY_PIN = 1; 
const uint8_t SENSOR_PIN = 4; 

volatile uint32_t startTime = 0;
volatile uint32_t stopTime = 0;

volatile bool firstContact = false;
volatile bool relayState = false;

uint8_t count = 0;
uint32_t sumOn = 0;
uint32_t sumOff = 0;
uint32_t lastTime = 0;
const uint32_t INTERVAL = 1500;

void IRAM_ATTR firstContactChange() {
    if (firstContact) return;
    
    if (relayState) {
        if (digitalRead(SENSOR_PIN) == LOW) {
            stopTime = micros();
            firstContact = true;
        }
    } else if (!relayState) {
        if (digitalRead(SENSOR_PIN) == HIGH) {
            stopTime = micros();
            firstContact = true;
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW);

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), firstContactChange, CHANGE);
}

void loop() {
    uint32_t currentTime = millis();

    if (currentTime - lastTime >= INTERVAL) {
        lastTime = currentTime;
        count++;
        
        Serial.printf("Замір %d | ", count);

        // Вимірювання часу Увімкнення
        relayState = true; 
        firstContact = false;
        
        startTime = micros();
        digitalWrite(RELAY_PIN, HIGH);
        
        delay(30);
        
        uint32_t timeOn = firstContact ? (stopTime - startTime) : 0;
        sumOn += timeOn;
        Serial.printf("Увімкнення: %.2f мс | ", timeOn / 1000.0);

        // Вимірювання часу Вимкнення
        relayState = false; 
        firstContact = false;
        
        startTime = micros();
        digitalWrite(RELAY_PIN, LOW);
        
        delay(30);
        
        uint32_t timeOff = firstContact ? (stopTime - startTime) : 0;
        sumOff += timeOff;
        Serial.printf("Вимкнення: %.2f мс\n", timeOff / 1000.0);

        if (count >= 10) {
            Serial.printf("Середній час | Увімкнення: %.2f мс | Вимкнення: %.2f мс <<<\n", 
                          (sumOn / 10.0) / 1000.0, (sumOff / 10.0) / 1000.0);
            
            count = 0;
            sumOn = 0;
            sumOff = 0;
        }
    }
}
