#include <Arduino.h>

// ---------- Налаштування пінів ----------
#define PIN_GREEN   5
#define PIN_YELLOW  6
#define PIN_RED     7
#define PIN_BUTTON  4

// ---------- Тривалості (мілісекунди) ----------
#define T_GREEN_MS          10000UL   // зелений: 10 с
#define T_GREEN_BLINK_MS     3000UL   // зелений миготливий: 3 с
#define T_YELLOW_MS          3000UL   // жовтий: 3 с
#define T_RED_MS            10000UL   // червоний: 10 с
#define T_RED_YELLOW_MS      2000UL   // червоний + жовтий: 2 с
#define BLINK_HALF_PERIOD_MS  500UL   // півперіод миготіння: 0,5 с
#define DEBOUNCE_MS           200UL   // антидребезг кнопки

// Тактування таймера: 80 МГц (APB) / 80 = 1 МГц => 1 тік = 1 мкс
#define TIMER_DIVIDER  80
#define MS_TO_TICKS(ms) ((uint64_t)(ms) * 1000ULL)

// ---------- Стани автомата ----------
enum TrafficState {
  STATE_GREEN = 0,      // зелений
  STATE_GREEN_BLINK,    // зелений миготливий
  STATE_YELLOW,         // жовтий
  STATE_RED,            // червоний
  STATE_RED_YELLOW,     // червоний + жовтий
  STATE_YELLOW_BLINK    // особливий режим: жовтий миготливий
};

hw_timer_t* stateTimer = nullptr;
hw_timer_t* blinkTimer = nullptr;
volatile bool stateTimeoutEvent = false;
volatile bool blinkTickEvent = false;
volatile bool buttonEvent = false;
portMUX_TYPE eventMux = portMUX_INITIALIZER_UNLOCKED;

TrafficState currentState = STATE_GREEN;
bool blinkOn = true;

void setSignals(bool red, bool yellow, bool green) {
    digitalWrite(PIN_RED, red ? HIGH : LOW);
    digitalWrite(PIN_YELLOW, yellow ? HIGH : LOW);
    digitalWrite(PIN_GREEN, green ? HIGH : LOW);
}

uint32_t getStateDuration(TrafficState state) {
    if (state == STATE_GREEN)        return T_GREEN_MS;
    if (state == STATE_GREEN_BLINK)  return T_GREEN_BLINK_MS;
    if (state == STATE_YELLOW)       return T_YELLOW_MS;
    if (state == STATE_RED)          return T_RED_MS;
    if (state == STATE_RED_YELLOW)   return T_RED_YELLOW_MS;
    return 0;
}

void enterState(TrafficState nextState) {
    currentState = nextState;
    blinkOn = true;

    timerAlarmDisable(stateTimer);
    timerStop(stateTimer);
    timerWrite(stateTimer, 0);

    portENTER_CRITICAL(&eventMux);
    stateTimeoutEvent = false;
    portEXIT_CRITICAL(&eventMux);

    if (currentState == STATE_GREEN)             setSignals(false, false, true);
    else if (currentState == STATE_GREEN_BLINK)  setSignals(false, false, true);
    else if (currentState == STATE_YELLOW)       setSignals(false, true, false);
    else if (currentState == STATE_RED)          setSignals(true, false, false);
    else if (currentState == STATE_RED_YELLOW)   setSignals(true, true, false);
    else if (currentState == STATE_YELLOW_BLINK) setSignals(false, true, false);

    uint32_t duration = getStateDuration(currentState);
    if (duration > 0) {
        timerAlarmWrite(stateTimer, MS_TO_TICKS(duration), false);
        timerAlarmEnable(stateTimer);
        timerStart(stateTimer);
    }
}

void IRAM_ATTR onStateTimer() {
    portENTER_CRITICAL_ISR(&eventMux);
    stateTimeoutEvent = true;
    portEXIT_CRITICAL_ISR(&eventMux);
}

void IRAM_ATTR onBlinkTimer() {
    portENTER_CRITICAL_ISR(&eventMux);
    blinkTickEvent = true;
    portEXIT_CRITICAL_ISR(&eventMux);
}

void IRAM_ATTR onButtonPress() {
    portENTER_CRITICAL_ISR(&eventMux);
    buttonEvent = true;
    portEXIT_CRITICAL_ISR(&eventMux);
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    stateTimer = timerBegin(0, TIMER_DIVIDER, true);
    timerAttachInterrupt(stateTimer, &onStateTimer, true);

    blinkTimer = timerBegin(1, TIMER_DIVIDER, true);
    timerAttachInterrupt(blinkTimer, &onBlinkTimer, true);
    timerAlarmWrite(blinkTimer, MS_TO_TICKS(BLINK_HALF_PERIOD_MS), true);
    timerAlarmEnable(blinkTimer);

    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onButtonPress, FALLING);

    enterState(STATE_GREEN);
}

void loop() {
    bool stateTimeout;
    bool blinkTick;
    bool buttonPressed;

    portENTER_CRITICAL(&eventMux);
    stateTimeout = stateTimeoutEvent;
    blinkTick = blinkTickEvent;
    buttonPressed = buttonEvent;
    stateTimeoutEvent = false;
    blinkTickEvent = false;
    buttonEvent = false;
    portEXIT_CRITICAL(&eventMux);

    static uint32_t lastAcceptedButtonTime = 0;
    static bool buttonWasAccepted = false;
    const uint32_t now = millis();
    const bool acceptButton = buttonPressed &&
        (!buttonWasAccepted || now - lastAcceptedButtonTime >= DEBOUNCE_MS);
    bool stateChanged = false;

    if (acceptButton) {
        buttonWasAccepted = true;
        lastAcceptedButtonTime = now;

        if (currentState == STATE_YELLOW_BLINK) {
            enterState(STATE_GREEN);
            Serial.println("Режим: Світлофор");
        } else {
            enterState(STATE_YELLOW_BLINK);
            Serial.println("Режим: Жовтий");
        }
        stateChanged = true;
    } else if (stateTimeout) {
        if (currentState == STATE_GREEN)             enterState(STATE_GREEN_BLINK);
        else if (currentState == STATE_GREEN_BLINK)   enterState(STATE_YELLOW);
        else if (currentState == STATE_YELLOW)        enterState(STATE_RED);
        else if (currentState == STATE_RED)           enterState(STATE_RED_YELLOW);
        else if (currentState == STATE_RED_YELLOW)    enterState(STATE_GREEN);
        stateChanged = true;
    }

    if (blinkTick && !stateChanged) {
        if (currentState == STATE_GREEN_BLINK) {
            blinkOn = !blinkOn;
            setSignals(false, false, blinkOn);
        } else if (currentState == STATE_YELLOW_BLINK) {
            blinkOn = !blinkOn;
            setSignals(false, blinkOn, false);
        }
    }
}