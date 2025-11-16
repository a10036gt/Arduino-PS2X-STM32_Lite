/**
 * @file PS2X_lib_STM32_LITE.cpp
 * @brief A Liteweight library for STM32duino interfacing with the PS2 joystick.
 * 
 * Based on PS2X_lib project, but Only the most frequently used functions (button IDs, oystick values) were retained,
 * and some code was modified to make it compatible with the STM32duino framework.
 * 
 * @author Anthony Hsu
 * @version 1.0
 * @date 2025/11/16
 */
#include "PS2X_lib_STM32_LITE.h"

static const uint8_t enter_config[] = {0x01,0x43,0x00,0x01,0x00};
static const uint8_t set_mode[]    = {0x01,0x44,0x00,0x01,0x03,0,0,0,0};
static const uint8_t exit_config[] = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static const uint8_t type_read[]   = {0x01,0x45,0,0,0,0,0,0,0};

//------------------------------------------------------------------------------
uint8_t PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {

    _clk_mask = digitalPinToBitMask(clk);
    _cmd_mask = digitalPinToBitMask(cmd);
    _att_mask = digitalPinToBitMask(att);
    _dat_mask = digitalPinToBitMask(dat);

    _clk_port = portOutputRegister(digitalPinToPort(clk));
    _cmd_port = portOutputRegister(digitalPinToPort(cmd));
    _att_port = portOutputRegister(digitalPinToPort(att));
    _dat_port = portInputRegister(digitalPinToPort(dat));

    pinMode(clk, OUTPUT);
    pinMode(cmd, OUTPUT);
    pinMode(att, OUTPUT);
    pinMode(dat, INPUT);

    CMD_SET(); CLK_SET(); ATT_SET();

    read_gamepad();
    read_gamepad();

    controller_type = PS2data[3];

    if(PS2data[1] != 0x41 && PS2data[1] != 0x73 && PS2data[1] != 0x79)
        return 1; // Not connected.

    uint8_t temp[sizeof(type_read)];
    for(int y=0; y<=10; y++){
        sendCommandString(enter_config, sizeof(enter_config));

        ATT_CLR(); delayMicroseconds(20);
        for(int i=0;i<9;i++)
            temp[i] = _gamepad_shiftinout(type_read[i]);
        ATT_SET();

        controller_type = temp[3];

        sendCommandString(set_mode, sizeof(set_mode));
        sendCommandString(exit_config, sizeof(exit_config));

        read_gamepad();
        if(PS2data[1] == 0x73 || PS2data[1] == 0x79)
            break;

        if(y == 10) return 2; // unresponsive mode command 
    }

    if(PS2data[1] == 0x73)
        return 3; // rumble mode is not supported.

    return 0; // OK
}

//------------------------------------------------------------------------------
bool PS2X::read_gamepad() {
    uint8_t cmd[9] = {0x01,0x42,0,0,0,0,0,0,0};
    ATT_CLR(); delayMicroseconds(20);

    for(int i=0;i<9;i++){
        PS2data[i] = _gamepad_shiftinout(cmd[i]);
		// Serial.print(PS2data[i], HEX);
		// Serial.print(",");
	}
	// Serial.println("");
    ATT_SET();
	
    if ((PS2data[1] & 0xf0) != 0x70) {
        return false;
    }

    last_buttons = buttons;
    buttons = (uint16_t)(PS2data[4]<<8) | PS2data[3];
    return true;
}

//------------------------------------------------------------------------------
// shift in/out
uint8_t PS2X::_gamepad_shiftinout(uint8_t byte){
    uint8_t tmp=0;
    for(uint8_t i=0;i<8;i++){
        if(byte & (1<<i)) CMD_SET(); else CMD_CLR();
        CLK_CLR(); delayMicroseconds(20);
        if(DAT_CHK()) tmp |= (1<<i);
        CLK_SET(); delayMicroseconds(20);
    }
    CMD_SET(); delayMicroseconds(20);
    return tmp;
}

//------------------------------------------------------------------------------
uint16_t PS2X::CurrentButton() {
    return ~buttons;
}

//------------------------------------------------------------------------------
uint8_t PS2X::Analog(uint8_t axis) {
    // 8=LY,7=LX,6=RY,5=RX
    return PS2data[axis]; 
}

//------------------------------------------------------------------------------
void PS2X::sendCommandString(const uint8_t* cmd, uint8_t len){
    ATT_CLR(); delayMicroseconds(20);
    for(int i=0;i<len;i++)
        _gamepad_shiftinout(cmd[i]);
    ATT_SET();
    delay(1);
}
