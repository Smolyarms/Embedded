#include <Arduino.h>

struct LedConfig {
    const uint8_t pin;       
    const uint32_t interval; 
    uint32_t last_toggle;    
    bool led_state;             
};

LedConfig leds[] = {
    {4, 200,  0, false},
    {5, 500,  0, false},
    {6, 1000, 0, false}
};

void toggle_led(LedConfig &led) {
    led.led_state = !led.led_state;
    digitalWrite(led.pin, led.led_state ? HIGH : LOW);
} 

void update_led(LedConfig &led) {
    uint32_t now = millis();

    if (now - led.last_toggle >= led.interval) {
        toggle_led(led);
        led.last_toggle = now;
    }
}

void init_leds() {
    for (LedConfig &l : leds) {
        pinMode(l.pin, OUTPUT);
        digitalWrite(l.pin, LOW);
    }
}
void setup() {
    init_leds();
}

void loop() {
    // update_led(led[0]);
    // update_led(led[1]);
    // update_led(led[2]);
    for (LedConfig &led : leds) {
        update_led(led);
    }
}