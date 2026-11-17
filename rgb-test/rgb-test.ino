const byte PIN_RED = 9;
const byte PIN_GREEN = 10;
const byte PIN_BLUE = 11;

const bool COMMON_ANODE = true;

const unsigned long STEP_MS = 2000;

const byte STEP_COUNT = 4;
const byte STEP_PINS[STEP_COUNT] = {PIN_RED, PIN_GREEN, PIN_BLUE, 0};
const char* STEP_NAMES[STEP_COUNT] = {"pin 9", "pin 10", "pin 11", "off"};

void setChannel(byte pin, bool on) {
    digitalWrite(pin, on != COMMON_ANODE ? HIGH : LOW);
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
}

void loop() {
    for (byte step = 0; step < STEP_COUNT; step++) {
        setChannel(PIN_RED, STEP_PINS[step] == PIN_RED);
        setChannel(PIN_GREEN, STEP_PINS[step] == PIN_GREEN);
        setChannel(PIN_BLUE, STEP_PINS[step] == PIN_BLUE);
        Serial.println(STEP_NAMES[step]);
        delay(STEP_MS);
    }
}
