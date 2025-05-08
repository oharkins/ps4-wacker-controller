// WackerController.ino
// Controller for Wacker equipment using Bluepad32
// Controls fuel, starter, vibration modes, and movement

#include <Bluepad32.h>

// Pin Definitions
namespace Pins {
    // Valid GPIO pins for ESP32 (0-39)
    const int FUEL = 4;      // GPIO4
    const int STARTER = 2;    // GPIO2
    const int BREAKOUT = 14;  // GPIO14
    const int LEFT = 12;      // GPIO12
    const int RIGHT = 13;     // GPIO13
    const int FORWARD = 27;   // GPIO27
    const int BACKWARD = 26;  // GPIO26
    const int VIB_HIGH = 25;  // GPIO25
    const int VIB_LOW = 33;   // GPIO33
    const int HIGH_SPEED = 32;// GPIO32

    // Function to validate GPIO pin number
    bool isValidPin(int pin) {
        return (pin >= 0 && pin <= 39);
    }
}

// Timing Constants
namespace Timing {
    const unsigned long DEBOUNCE_DELAY = 500;    // ms
    const unsigned long VIB_TOGGLE_DELAY = 10000; // ms
    const unsigned long LOOP_DELAY = 100;        // ms
}

// Controller state
struct ControllerState {
    bool fuelState = false;
    bool vibHighState = false;
    bool vibLowState = false;
    bool highSpeedState = false;
    unsigned long lastToggleTime = 0;
    unsigned long vibToggleTime = 0;
    bool lastAState = false;  // Track previous button states
    bool lastBState = false;
} state;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                           properties.product_id);
            Serial.println("Controller Connected Turn On Fuel.");
            state.fuelState = true;
            digitalWrite(Pins::FUEL, state.fuelState);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            Serial.println("Controller Not Connected Turn off fuel.");
            digitalWrite(Pins::FUEL, false);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
      //  "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
      //   "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        "buttons: 0x%04x, misc: 0x%02x \n \r",
        // ctl->index(),        // Controller Index
        // ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        // ctl->axisX(),        // (-511 - 512) left X Axis
        // ctl->axisY(),        // (-511 - 512) left Y axis
        // ctl->axisRX(),       // (-511 - 512) right X axis
        // ctl->axisRY(),       // (-511 - 512) right Y axis
        // ctl->brake(),        // (0 - 1023): brake button
        // ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons()  // bitmask of pressed "misc" buttons
        // ctl->gyroX(),        // Gyro X
        // ctl->gyroY(),        // Gyro Y
        // // ctl->gyroZ(),        // Gyro Z
        // ctl->accelX(),       // Accelerometer X
        // ctl->accelY(),       // Accelerometer Y
        // ctl->accelZ()        // Accelerometer Z
    );
}

// Helper function to safely write to GPIO pins
void safeDigitalWrite(int pin, int value) {
    if (Pins::isValidPin(pin)) {
        digitalWrite(pin, value);
    } else {
        Serial.printf("Error: Attempted to write to invalid GPIO pin %d\n", pin);
    }
}

// Update processDpadMovement to use safeDigitalWrite
void processDpadMovement(uint8_t dpad) {
    // Check for invalid D-pad combinations
    if ((dpad & DPAD_UP) && (dpad & (DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT))) return;
    if ((dpad & DPAD_DOWN) && (dpad & (DPAD_UP | DPAD_LEFT | DPAD_RIGHT))) return;
    if ((dpad & DPAD_LEFT) && (dpad & (DPAD_UP | DPAD_DOWN | DPAD_RIGHT))) return;
    if ((dpad & DPAD_RIGHT) && (dpad & (DPAD_UP | DPAD_DOWN | DPAD_LEFT))) return;

    // Process valid D-pad inputs
    safeDigitalWrite(Pins::FORWARD, (dpad & DPAD_UP) ? HIGH : LOW);
    safeDigitalWrite(Pins::BACKWARD, (dpad & DPAD_DOWN) ? HIGH : LOW);
    safeDigitalWrite(Pins::LEFT, (dpad & DPAD_LEFT) ? HIGH : LOW);
    safeDigitalWrite(Pins::RIGHT, (dpad & DPAD_RIGHT) ? HIGH : LOW);
    
    // Set breakout based on forward/backward state
    safeDigitalWrite(Pins::BREAKOUT, ((dpad & DPAD_UP) || (dpad & DPAD_DOWN)) ? HIGH : LOW);
}

// Update processGamepad to use safeDigitalWrite
void processGamepad(ControllerPtr ctl) {
    if (!ctl) return;  // Safety check

    uint8_t dpad = ctl->dpad();
    unsigned long currentTime = millis();

    // Toggle Fuel Relay
    if ((ctl->miscButtons() & 0x04) && currentTime - state.lastToggleTime >= Timing::DEBOUNCE_DELAY) {
        Serial.println("Options button pressed");
        state.fuelState = !state.fuelState;
        safeDigitalWrite(Pins::FUEL, state.fuelState);
        Serial.print("Fuel: ");
        Serial.println(state.fuelState ? "ON" : "OFF");
        ctl->setColorLED(state.fuelState ? 0 : 255, state.fuelState ? 255 : 0, 0);
        state.lastToggleTime = currentTime;
    }

    // Hold Option Button to Start
    if (ctl->miscButtons() & 0x01) {
        Serial.println("PS button pressed");
        safeDigitalWrite(Pins::STARTER, LOW);
        state.fuelState = true;
        safeDigitalWrite(Pins::FUEL, state.fuelState);
        ctl->setColorLED(0, 255, 0);
        Serial.println("Starting");
    } else {
        safeDigitalWrite(Pins::STARTER, LOW);
    }
    
    // Toggle High Speed mode
    if (ctl->y() && currentTime - state.lastToggleTime >= Timing::DEBOUNCE_DELAY) {
        Serial.println("Triangle Pressed");
        state.highSpeedState = !state.highSpeedState;
        safeDigitalWrite(Pins::HIGH_SPEED, state.highSpeedState);
        Serial.print("High Speed: ");
        Serial.println(state.highSpeedState ? "ON" : "OFF");
        state.lastToggleTime = currentTime;
    }

    // Toggle Vibration Modes
    bool currentAState = ctl->a();
    bool currentBState = ctl->b();
    
    // Handle A button (High Vibration)
    if (currentAState && !state.lastAState) {  // Button just pressed
        unsigned long currentVibTime = millis();
        if (state.vibHighState) {
            // Can turn off high vibration immediately
            Serial.println("X Pressed - Turning off High Vibration");
            state.vibHighState = false;
            safeDigitalWrite(Pins::VIB_HIGH, false);
            state.vibToggleTime = currentVibTime;
        } else if (!state.vibLowState && currentVibTime - state.vibToggleTime >= Timing::VIB_TOGGLE_DELAY) {
            // Can turn on high vibration after delay if low is off
            Serial.println("X Pressed - Turning on High Vibration");
            state.vibHighState = true;
            state.vibLowState = false;
            safeDigitalWrite(Pins::VIB_HIGH, true);
            safeDigitalWrite(Pins::VIB_LOW, false);
            state.vibToggleTime = currentVibTime;
        }
    }
    state.lastAState = currentAState;

    // Handle B button (Low Vibration)
    if (currentBState && !state.lastBState) {  // Button just pressed
        unsigned long currentVibTime = millis();
        if (state.vibLowState) {
            // Can turn off low vibration immediately
            Serial.println("B Pressed - Turning off Low Vibration");
            state.vibLowState = false;
            safeDigitalWrite(Pins::VIB_LOW, false);
            state.vibToggleTime = currentVibTime;
        } else if (!state.vibHighState && currentVibTime - state.vibToggleTime >= Timing::VIB_TOGGLE_DELAY) {
            // Can turn on low vibration after delay if high is off
            Serial.println("B Pressed - Turning on Low Vibration");
            state.vibLowState = true;
            state.vibHighState = false;
            safeDigitalWrite(Pins::VIB_LOW, true);
            safeDigitalWrite(Pins::VIB_HIGH, false);
            state.vibToggleTime = currentVibTime;
        }
    }
    state.lastBState = currentBState;

    // Square button for starting
    if (ctl->x()) {
        Serial.println("Square button pressed");
        safeDigitalWrite(Pins::STARTER, HIGH);
        state.fuelState = true;
        safeDigitalWrite(Pins::FUEL, state.fuelState);
        ctl->setColorLED(0, 255, 0);
        Serial.println("Starting");
    } else {
        safeDigitalWrite(Pins::STARTER, LOW);
    }

    // Process D-pad movement
    processDpadMovement(dpad);
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

// Arduino setup function. Runs in CPU 1
void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
    // Initialize all pins with validation
    const int pins[] = {
        Pins::FUEL, Pins::STARTER, Pins::BREAKOUT, Pins::FORWARD,
        Pins::BACKWARD, Pins::LEFT, Pins::RIGHT, Pins::VIB_HIGH,
        Pins::VIB_LOW, Pins::HIGH_SPEED
    };

    for (int pin : pins) {
        if (Pins::isValidPin(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
        } else {
            Serial.printf("Error: Invalid GPIO pin %d\n", pin);
        }
    }

    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    //     vTaskDelay(1);
    delay(Timing::LOOP_DELAY);
}