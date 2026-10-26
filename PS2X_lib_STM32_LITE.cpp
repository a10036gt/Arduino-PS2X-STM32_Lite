/**
 * @file PS2X_lib_STM32_LITE.cpp
 * @brief A lightweight library for STM32duino interfacing with the PS2 joystick.
 * @version 1.1 (see header for the list of fixes)
 */
#include "PS2X_lib_STM32_LITE.h"

static const uint8_t enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
static const uint8_t type_read[]    = {0x01, 0x45, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t set_mode[]     = {0x01, 0x44, 0x00, 0x01, 0x03, 0, 0, 0, 0};  /* analog + lock */
static const uint8_t exit_config[]  = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t poll_cmd[]     = {0x01, 0x42, 0, 0, 0, 0, 0, 0, 0};

static inline bool isAnalog(uint8_t mode) {
    return mode == PS2_MODE_ANALOG || mode == PS2_MODE_PRESSURE;
}

//------------------------------------------------------------------------------
uint8_t PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {
    _clk_port = digitalPinToPort(clk);  _clk_mask = digitalPinToBitMask(clk);
    _cmd_port = digitalPinToPort(cmd);  _cmd_mask = digitalPinToBitMask(cmd);
    _att_port = digitalPinToPort(att);  _att_mask = digitalPinToBitMask(att);
    _dat_port = digitalPinToPort(dat);  _dat_mask = digitalPinToBitMask(dat);

    pinMode(clk, OUTPUT);
    pinMode(cmd, OUTPUT);
    pinMode(att, OUTPUT);
    pinMode(dat, INPUT_PULLUP);   /* DAT is open-collector */

    CMD_SET(); CLK_SET(); ATT_SET();
    delay(5);

    poll();
    poll();
    if (PS2data[2] != 0x5A) {
        last_config = millis();
        return 1;                 /* nothing answering yet; read_gamepad() will retry */
    }

    for (int y = 0; y < 10; y++) {
        if (configure()) {
            return 0;
        }
    }
    last_config = millis();
    return 2;
}

//------------------------------------------------------------------------------
bool PS2X::read_gamepad() {
    if (!_clk_port) {
        return false;             /* config_gamepad() not called */
    }

    uint32_t now = millis();
    if ((now - last_read) < PS2_POLL_MS) {
        return _valid;            /* too soon: keep the last result */
    }
    last_read = now;

    poll();
    bool header_ok = (PS2data[2] == 0x5A);
    _mode = PS2data[1];

    if (header_ok && (isAnalog(_mode) || _mode == PS2_MODE_DIGITAL)) {
        last_buttons = buttons;
        buttons = (uint16_t)(PS2data[4] << 8) | PS2data[3];
    }

    _valid = header_ok && isAnalog(_mode);

    /* Not in analog mode (or nothing answering): re-configure, rate limited.
     * Typical case: wireless receiver paired after power-up, or pad was replugged. */
    if (!_valid && (now - last_config) >= PS2_RECONFIG_MS) {
        last_config = now;
        configure();
    }
    return _valid;
}

//------------------------------------------------------------------------------
void PS2X::poll() {
    sendCommandString(poll_cmd, sizeof(poll_cmd), PS2data);
}

//------------------------------------------------------------------------------
bool PS2X::configure() {
    uint8_t temp[sizeof(type_read)];

    sendCommandString(enter_config, sizeof(enter_config));
    sendCommandString(type_read, sizeof(type_read), temp);
    if (temp[2] == 0x5A) {
        controller_type = temp[3];
    }
    sendCommandString(set_mode, sizeof(set_mode));
    sendCommandString(exit_config, sizeof(exit_config));

    poll();
    _mode = PS2data[1];
    _valid = (PS2data[2] == 0x5A) && isAnalog(_mode);
    return _valid;
}

//------------------------------------------------------------------------------
// One packet: ATT low, shift every byte, ATT high, then a short pause so the
// pad sees ATT high before the next packet.
void PS2X::sendCommandString(const uint8_t *cmd, uint8_t len, uint8_t *rx) {
    ATT_CLR();
    delayMicroseconds(PS2_ATT_SETUP_US);
    for (uint8_t i = 0; i < len; i++) {
        uint8_t v = _gamepad_shiftinout(cmd[i]);
        if (rx) {
            rx[i] = v;
        }
    }
    ATT_SET();
    delay(1);
}

//------------------------------------------------------------------------------
// SPI mode 3, LSB first: CMD set while CLK high, pad shifts DAT on the falling
// edge, we sample before the rising edge.
uint8_t PS2X::_gamepad_shiftinout(uint8_t byte) {
    uint8_t tmp = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & (1 << i)) CMD_SET(); else CMD_CLR();
        CLK_CLR();
        delayMicroseconds(PS2_CLK_HALF_US);
        if (DAT_CHK()) tmp |= (1 << i);
        CLK_SET();
        delayMicroseconds(PS2_CLK_HALF_US);
    }
    CMD_SET();
    delayMicroseconds(PS2_BYTE_GAP_US);
    return tmp;
}

//------------------------------------------------------------------------------
uint16_t PS2X::CurrentButton() {
    return (uint16_t)~buttons;
}

//------------------------------------------------------------------------------
uint8_t PS2X::Analog(uint8_t axis) {
    /* 8=LY, 7=LX, 6=RY, 5=RX */
    return (axis < sizeof(PS2data)) ? PS2data[axis] : 0;
}
