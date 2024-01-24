#ifndef TEF6686I2CCOMM_H
#define TEF6686I2CCOMM_H

#include <Arduino.h>

class TEF6686I2CComm
{
private:
    void Writei2c(uint8_t data);
    uint8_t DEVICE_ADDR = 0x64;

public:
    void GetCommand(uint8_t module, uint8_t cmd, uint16_t *response, uint8_t responseLength);
    void SetCommand(uint8_t module, uint8_t cmd, uint16_t *params, uint8_t paramsCount);
};
#endif