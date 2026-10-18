#include <Arduino.h>


#define PIN_FAN_RELAY  1

#define WORK_TIME   1000UL   // 1 секунда роботи вентилятора(реле)
#define PERIOD_TIME 4000UL   // 4 секунди загального періоду (робота + пауза)

#define TIMER_DIVIDER  80
#define MS_TO_TICKS(ms) ((uint64_t)(ms) * 1000ULL)

hw_timer_t* fanTimer = nullptr;

void IRAM_ATTR onFanTimerTimeout() {
    static bool fanIsOn = false;

    if (!fanIsOn) {
        digitalWrite(PIN_FAN_RELAY, HIGH);
        fanIsOn = true;

        timerAlarmWrite(fanTimer, MS_TO_TICKS(WORK_TIME), true);
    } 
    else {
        digitalWrite(PIN_FAN_RELAY, LOW);
        fanIsOn = false;

        uint32_t pauseTime = PERIOD_TIME - WORK_TIME;

        timerAlarmWrite(fanTimer, MS_TO_TICKS(pauseTime), true);
    }
}

void setup() {
    pinMode(PIN_FAN_RELAY, OUTPUT);
    digitalWrite(PIN_FAN_RELAY, LOW);

    fanTimer = timerBegin(0, TIMER_DIVIDER, true);
    timerAttachInterrupt(fanTimer, &onFanTimerTimeout, true);
    timerAlarmWrite(fanTimer, MS_TO_TICKS(WORK_TIME), true);
    timerAlarmEnable(fanTimer);
    timerStart(fanTimer);
}

void loop() {

}