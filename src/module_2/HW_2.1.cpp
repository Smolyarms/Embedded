#include <Arduino.h>

enum class LedState : uint8_t {
    Off = LOW,
    On = HIGH
};

struct LedConfig {
    static constexpr uint8_t  LED_PIN = 4;  
    static constexpr uint32_t BLINK_INTERVAL = 1000;
};

class Led {
private:
    const uint8_t pin;

public:
    explicit constexpr Led(uint8_t pinNumber) : pin(pinNumber) {}

    void init() const {
        pinMode(pin, OUTPUT);
    }

    void set(LedState state) const {
        digitalWrite(pin, static_cast<uint8_t>(state));
    }
};

const Led FirstLed(LedConfig::LED_PIN);
uint32_t previousMillis = 0;
LedState currentState = LedState::Off;

void setup() {
    
    FirstLed.init();
    FirstLed.set(LedState::Off);
}

void loop() {
    uint32_t currentMillis = millis();

    if (currentMillis - previousMillis >= LedConfig::BLINK_INTERVAL) {
        previousMillis = currentMillis;

        currentState = (currentState == LedState::Off) ? LedState::On : LedState::Off;
        FirstLed.set(currentState);
    }
}

