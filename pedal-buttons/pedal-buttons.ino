const int PIN_LEFT = 3;
const int PIN_RIGHT = 2;

const unsigned long HOLD_THRESHOLD_MS = 750;

struct Button {
    int pin;
    const char* name;
    bool pressed;
    bool holdFired;
    unsigned long pressTime;
};

Button leftButton  = {PIN_LEFT,  "left",  false, false, 0};
Button rightButton = {PIN_RIGHT, "right", false, false, 0};

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
}

void loop() {
    handleButton(leftButton);
    handleButton(rightButton);
    delay(50);
}

void handleButton(Button& btn) {
    bool raw = digitalRead(btn.pin) == LOW;

    if (raw && !btn.pressed) {
        btn.pressed = true;
        btn.holdFired = false;
        btn.pressTime = millis();
    }
    else if (raw && !btn.holdFired) {
        if (millis() - btn.pressTime >= HOLD_THRESHOLD_MS) {
            btn.holdFired = true;
            Serial.print(btn.name);
            Serial.println("-hold");
        }
    }
    else if (!raw && btn.pressed) {
        if (!btn.holdFired) {
            Serial.print(btn.name);
            Serial.println("-click");
        }
        btn.pressed = false;
    }
}
