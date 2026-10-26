/**
 * @file PS2X_lib_STM32_LITE.h
 * @brief A lightweight library for STM32duino interfacing with the PS2 joystick.
 *
 * Based on PS2X_lib, keeping only button IDs and joystick values.
 *
 * v1.1 (2026/10/26) compatibility fixes:
 *  - DAT uses INPUT_PULLUP (DAT is open-collector; an external 1k-4.7k pull-up is still recommended)
 *  - read_gamepad() polls at most every PS2_POLL_MS (read_delay was declared but never used)
 *  - automatic re-configuration when the pad is not in analog mode (wireless pads pair late)
 *  - packet validated with the 0x5A header byte
 *  - 32-bit pin masks + atomic BSRR writes (old uint8_t masks broke on pins >= 8)
 *
 * @author Anthony Hsu
 * @version 1.1
 */
#pragma once
#include <Arduino.h>
#include <stdint.h>

/* ---- Timing (tune here if a pad still misbehaves) ---- */
#ifndef PS2_CLK_HALF_US
#define PS2_CLK_HALF_US   10    /* clock half period: 10 us = 50 kHz (original lib: 20 us) */
#endif
#ifndef PS2_BYTE_GAP_US
#define PS2_BYTE_GAP_US   20    /* gap between bytes */
#endif
#ifndef PS2_ATT_SETUP_US
#define PS2_ATT_SETUP_US  20    /* ATT low -> first clock */
#endif
#ifndef PS2_POLL_MS
#define PS2_POLL_MS       15    /* minimum interval between polls (console: ~16 ms) */
#endif
#ifndef PS2_RECONFIG_MS
#define PS2_RECONFIG_MS   500   /* retry configuration at most this often */
#endif

/* Mode byte values (PS2data[1]) */
#define PS2_MODE_DIGITAL  0x41
#define PS2_MODE_ANALOG   0x73
#define PS2_MODE_PRESSURE 0x79
#define PS2_MODE_CONFIG   0xF3

class PS2X {
public:
    PS2X() {}

    /* Returns 0 = OK (analog), 1 = no controller yet, 2 = mode command ignored.
     * Not fatal: read_gamepad() keeps retrying the configuration on its own. */
    uint8_t config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat);

    /* Call as often as you like; it only talks to the pad every PS2_POLL_MS.
     * Returns true while the last poll was a valid analog packet. */
    bool read_gamepad();

    uint16_t CurrentButton();             /* 1 = pressed */
    uint8_t  Analog(uint8_t axis);        /* 8=LY, 7=LX, 6=RY, 5=RX */

    uint8_t  Mode() const { return _mode; }          /* last mode byte, for debugging */
    uint8_t  ControllerType() const { return controller_type; }

    uint16_t buttons = 0xFFFF;            /* raw, active low */

private:
    uint16_t last_buttons = 0xFFFF;
    uint32_t last_read = 0;
    uint32_t last_config = 0;
    bool     _valid = false;
    uint8_t  _mode = 0;
    uint8_t  controller_type = 0;
    uint8_t  PS2data[9] = {0};

    GPIO_TypeDef *_clk_port = nullptr;
    GPIO_TypeDef *_cmd_port = nullptr;
    GPIO_TypeDef *_att_port = nullptr;
    GPIO_TypeDef *_dat_port = nullptr;
    uint32_t _clk_mask = 0;
    uint32_t _cmd_mask = 0;
    uint32_t _att_mask = 0;
    uint32_t _dat_mask = 0;

    void    poll();
    bool    configure();
    void    sendCommandString(const uint8_t *cmd, uint8_t len, uint8_t *rx = nullptr);
    uint8_t _gamepad_shiftinout(uint8_t byte);

    /* BSRR: lower 16 bits set, upper 16 bits reset -> atomic, no read-modify-write */
    inline void CLK_SET() { _clk_port->BSRR = _clk_mask; }
    inline void CLK_CLR() { _clk_port->BSRR = _clk_mask << 16; }
    inline void CMD_SET() { _cmd_port->BSRR = _cmd_mask; }
    inline void CMD_CLR() { _cmd_port->BSRR = _cmd_mask << 16; }
    inline void ATT_SET() { _att_port->BSRR = _att_mask; }
    inline void ATT_CLR() { _att_port->BSRR = _att_mask << 16; }
    inline bool DAT_CHK() { return (_dat_port->IDR & _dat_mask) != 0; }
};
