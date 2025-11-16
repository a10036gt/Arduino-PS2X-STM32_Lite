/**
 * @file PS2X_lib_STM32_LITE.h
 * @brief A Liteweight library for STM32duino interfacing with the PS2 joystick.
 * 
 * Based on PS2X_lib project, but Only the most frequently used functions (button IDs, oystick values) were retained,
 * and some code was modified to make it compatible with the STM32duino framework.
 * 
 * @author Anthony Hsu
 * @version 1.0
 * @date 2025/11/16
 */
#pragma once
#include <Arduino.h>
#include <stdint.h>

class PS2X {
public:
    PS2X() : buttons(0), last_buttons(0), controller_type(0), last_read(0), read_delay(1) {}

    uint8_t config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat);
    bool read_gamepad();
    
	uint16_t CurrentButton();
    uint8_t Analog(uint8_t axis); // 8=LY,7=LX,6=RY,5=RX

    uint16_t buttons;
    
private:
    uint16_t last_buttons;
    uint32_t last_read;
    uint8_t read_delay;
    uint8_t controller_type;
    uint8_t PS2data[12];

    // STM32 port access
    volatile uint32_t* _clk_port;
    volatile uint32_t* _cmd_port;
    volatile uint32_t* _att_port;
    volatile uint32_t* _dat_port;

    uint8_t _clk_mask;
    uint8_t _cmd_mask;
    uint8_t _att_mask;
    uint8_t _dat_mask;

    void sendCommandString(const uint8_t* cmd, uint8_t len);
    uint8_t _gamepad_shiftinout(uint8_t byte);

    inline void CLK_SET()   { *_clk_port |= _clk_mask; }
    inline void CLK_CLR()   { *_clk_port &= ~_clk_mask; }
    inline void CMD_SET()   { *_cmd_port |= _cmd_mask; }
    inline void CMD_CLR()   { *_cmd_port &= ~_cmd_mask; }
    inline void ATT_SET()   { *_att_port |= _att_mask; }
    inline void ATT_CLR()   { *_att_port &= ~_att_mask; }
    inline bool DAT_CHK()   { return (*_dat_port & _dat_mask) != 0; }
};
