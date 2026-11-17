const char* FIRMWARE_ID = "pedal-buttons";
const char* FIRMWARE_VERSION = "1.2.0";

const int PIN_LEFT = 3;
const int PIN_RIGHT = 2;

const byte PIN_RED = 9;
const byte PIN_GREEN = 10;
const byte PIN_BLUE = 11;

const bool COMMON_ANODE = true;

const unsigned long HOLD_THRESHOLD_MS = 750;
const unsigned long DEBOUNCE_MS = 25;
const unsigned long HOST_TIMEOUT_MS = 3000;

struct Color {
    byte red;
    byte green;
    byte blue;
};

const Color COLOR_POWER  = {1,   0,   0};
const Color COLOR_HOST   = {0,   10,  0};
const Color COLOR_CYAN   = {0,   255, 255};
const Color COLOR_BLUE   = {0,   0,   255};
const Color COLOR_YELLOW = {255, 70,  0};
const Color COLOR_ORANGE = {255, 25,  0};

struct Button {
    int pin;
    const char* name;
    const Color* clickColor;
    const Color* holdColor;
    bool reading;
    bool pressed;
    bool holdFired;
    unsigned long lastChange;
    unsigned long pressTime;
};

Button leftButton  = {PIN_LEFT,  "left",  &COLOR_CYAN,   &COLOR_BLUE,   false, false, false, 0, 0};
Button rightButton = {PIN_RIGHT, "right", &COLOR_YELLOW, &COLOR_ORANGE, false, false, false, 0, 0};

const Color* shownColor = 0;
bool hostSeen = false;
unsigned long lastHostPing = 0;

void announce() {
    Serial.print(FIRMWARE_ID);
    Serial.print(' ');
    Serial.println(FIRMWARE_VERSION);
}

void writeChannel(byte pin, byte value) {
    analogWrite(pin, COMMON_ANODE ? 255 - value : value);
}

void showColor(const Color* color) {
    if (color == shownColor) {
        return;
    }
    shownColor = color;
    writeChannel(PIN_RED, color->red);
    writeChannel(PIN_GREEN, color->green);
    writeChannel(PIN_BLUE, color->blue);
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    showColor(&COLOR_POWER);

    const unsigned long now = millis();
    leftButton.lastChange = now;
    rightButton.lastChange = now;

    announce();
}

void loop() {
    const unsigned long now = millis();

    handleButton(leftButton, now);
    handleButton(rightButton, now);
    handleSerial(now);
    updateLed(now);
}

void handleSerial(unsigned long now) {
    while (Serial.available() > 0) {
        const int incoming = Serial.read();
        if (incoming == '?') {
            announce();
        }
        else if (incoming == '!') {
            hostSeen = true;
            lastHostPing = now;
        }
    }
}

void updateLed(unsigned long now) {
    if (hostSeen && now - lastHostPing >= HOST_TIMEOUT_MS) {
        hostSeen = false;
    }

    const Button* active = 0;
    if (leftButton.pressed) {
        active = &leftButton;
    }
    if (rightButton.pressed && (active == 0 || rightButton.pressTime - active->pressTime < 0x80000000UL)) {
        active = &rightButton;
    }

    if (active != 0) {
        showColor(active->holdFired ? active->holdColor : active->clickColor);
    }
    else {
        showColor(hostSeen ? &COLOR_HOST : &COLOR_POWER);
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
