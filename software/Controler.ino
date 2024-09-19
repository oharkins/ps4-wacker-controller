#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

const int fuel = 13;
bool fuelState = false;
const int starter = 12;
const int breakout = 14;
const int forward = 27;
const int backward = 26;
const int vibHigh = 25;
bool vibHighState = false;
const int vibLow = 33;
bool vibLowState = false;
unsigned long vibToggleTime = 0;
const int highSpeed = 32;
bool highSpeedState = false;
unsigned long lastToggleTime = 0;



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
            fuelState = true;
            digitalWrite(fuel, fuelState);
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
            digitalWrite(fuel, false);
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



void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...

    uint8_t dpad = ctl->dpad();

    // Toggel Fuel Relay
    if ((ctl->miscButtons() & 0x04) && millis() - lastToggleTime >= 500) {
        Serial.println("Options button pressed");
        fuelState = !fuelState;
        digitalWrite(fuel, fuelState);
        Serial.print("Fuel: ");
        Serial.println(fuelState ? "ON" : "OFF");
        if(fuelState){
          ctl->setColorLED(0, 255, 0);
        }
        else{
          ctl->setColorLED(255, 0, 0);
        }
        lastToggleTime = millis();
    }
    // Hold Option Button to Start
    if (ctl->miscButtons() & 0x01) {
        Serial.println("PS button pressed");
        digitalWrite(starter, LOW);
        fuelState = true;
        digitalWrite(fuel, fuelState);
        if(fuelState){
          ctl->setColorLED(0, 255, 0);
        }
        else{
          ctl->setColorLED(255, 0, 0);
        }
        Serial.println("Starting");
    } else {
        // Release the pin when the A button is not pressed
        digitalWrite(starter, HIGH);
    }
    
    // Toggel High Speed mode
    if (ctl->y() && millis() - lastToggleTime >= 500) {
        Serial.println("Triangle Pressed");
        highSpeedState = !highSpeedState;
        digitalWrite(highSpeed, highSpeedState);
        Serial.print("High Speed: ");
        Serial.println(highSpeedState ? "ON" : "OFF");
        lastToggleTime = millis();
    }

    // Set Vibaration Mode HIGH
    if (ctl->a() && millis() - vibToggleTime >= 10000) {
        Serial.println("X Pressed");
        vibHighState = !vibHighState;
        digitalWrite(vibHigh, vibHighState);
        Serial.print("Vib High: ");
        Serial.println(vibHighState ? "ON" : "OFF");
        vibToggleTime = millis();
    }

    // Set Vibaration Mode HIGH
    if (ctl->b() && millis() - vibToggleTime >= 10000) {
        Serial.println("B Pressed");
        vibLowState = !vibLowState;
        digitalWrite(vibLow, vibLowState);
        Serial.print("Vib Low: ");
        Serial.println(vibLowState ? "ON" : "OFF");
        vibToggleTime = millis();
    }

    if (ctl->x()) {
        Serial.println("Square button pressed");
        digitalWrite(starter, LOW);
        fuelState = true;
        digitalWrite(fuel, fuelState);
        if(fuelState){
          ctl->setColorLED(0, 255, 0);
        }
        else{
          ctl->setColorLED(255, 0, 0);
        }
        Serial.println("Starting");
    } else {
        digitalWrite(starter, HIGH);
    }
    
    

    // Check if more than one D-pad button is pressed
    if ((dpad & DPAD_UP) && (dpad & (DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT))) {
        // More than one D-pad button is pressed, ignore this input
        return;
    }
    if ((dpad & DPAD_DOWN) && (dpad & (DPAD_UP | DPAD_LEFT | DPAD_RIGHT))) {
        return;
    }
    if ((dpad & DPAD_LEFT) && (dpad & (DPAD_UP | DPAD_DOWN | DPAD_RIGHT))) {
        return;
    }
    if ((dpad & DPAD_RIGHT) && (dpad & (DPAD_UP | DPAD_DOWN | DPAD_LEFT))) {
        return;
    }

    // Process the D-pad input
    if (dpad & DPAD_UP) {
        Serial.println("Forward");
    }
    if (dpad & DPAD_DOWN) {
        Serial.println("Back");
    }
    if (dpad & DPAD_LEFT) {
        Serial.println("Left");
    }
    if (dpad & DPAD_RIGHT) {
        Serial.println("Right");
    }
    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
   //dumpGamepad(ctl);
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
    
    pinMode(fuel, OUTPUT);
    pinMode(starter, OUTPUT);
    pinMode(breakout, OUTPUT);
    pinMode(forward, OUTPUT);
    pinMode(backward, OUTPUT);
    pinMode(vibHigh, OUTPUT);
    pinMode(vibLow, OUTPUT);
    pinMode(highSpeed, OUTPUT);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
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
    delay(100);
}
