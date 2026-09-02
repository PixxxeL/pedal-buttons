const char* FIRMWARE_ID = "pedal-buttons";
const char* FIRMWARE_VERSION = "1.1.0";

const int PIN_LEFT = 3;
const int PIN_RIGHT = 2;

const unsigned long HOLD_THRESHOLD_MS = 750;
const unsigned long DEBOUNCE_MS = 25;

struct Button {
    int pin;
    const char* name;
    bool reading;
    bool pressed;
    bool holdFired;
    unsigned long lastChange;
    unsigned long pressTime;
};

Button leftButton  = {PIN_LEFT,  "left",  false, false, false, 0, 0};
Button rightButton = {PIN_RIGHT, "right", false, false, false, 0, 0};

void announce() {
    Serial.print(FIRMWARE_ID);
    Serial.print(' ');
    Serial.println(FIRMWARE_VERSION);
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);

    const unsigned long now = millis();
    leftButton.lastChange = now;
    rightButton.lastChange = now;

    announce();
}

void loop() {
    const unsigned long now = millis();

    handleButton(leftButton, now);
    handleButton(rightButton, now);
    handleSerial();
}

void handleSerial() {
    while (Serial.available() > 0) {
        const int incoming = Serial.read();
        if (incoming == '?') {
            announce();
        }
    }
}

void handleButton(Button& button, unsigned long now) {
    const bool raw = digitalRead(button.pin) == LOW;

    if (raw != button.reading) {
        button.reading = raw;
        button.lastChange = now;
        return;
    }

    if (raw != button.pressed && now - button.lastChange >= DEBOUNCE_MS) {
        button.pressed = raw;

        if (raw) {
            button.holdFired = false;
            button.pressTime = now;
        }
        else if (!button.holdFired) {
            report(button, "-click");
        }
        return;
    }

    if (button.pressed && !button.holdFired && now - button.pressTime >= HOLD_THRESHOLD_MS) {
        button.holdFired = true;
        report(button, "-hold");
    }
}

void report(const Button& button, const char* suffix) {
    Serial.print(button.name);
    Serial.println(suffix);
}
