#include <Arduino.h>
#include "PS2X_lib_STM32_LITE.h"

#define PS2_CLK PA4
#define PS2_CMD PA6
#define PS2_ATT PA5
#define PS2_DAT PA7

PS2X ps2;

void setup() {
    Serial.begin(115200);
    uint8_t err = ps2.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT);
    if(err != 0){
        Serial.print("PS2 Init Error: "); Serial.println(err);
    } else {
        Serial.println("PS2 Ready");
    }
}

void loop() {
    if(ps2.read_gamepad()){
        uint8_t LY = ps2.Analog(8);
        uint8_t LX = ps2.Analog(7);
        uint8_t RY = ps2.Analog(6);
        uint8_t RX = ps2.Analog(5);
        uint16_t btnMask = ps2.CurrentButton();

        Serial.print("LX:"); Serial.print(LX);
        Serial.print(" LY:"); Serial.print(LY);
        Serial.print(" RX:"); Serial.print(RX);
        Serial.print(" RY:"); Serial.println(RY);
        Serial.print(" BTN:"); Serial.println(btnMask);
    }

    delay(50);
}
