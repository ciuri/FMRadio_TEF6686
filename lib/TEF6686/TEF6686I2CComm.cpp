#include "TEF6686I2CComm.h"
#include <Wire.h>

void TEF6686I2CComm::Writei2c(uint8_t data)
{
    char text[8];
    sprintf(text, " 0x%02x ", data);
    Wire.write(data);
}

void TEF6686I2CComm::GetCommand(uint8_t module, uint8_t cmd, uint16_t *response, uint8_t responseLength)
{
    SetCommand(module, cmd, nullptr, 0);
    uint8_t dataLength = Wire.requestFrom(DEVICE_ADDR, responseLength * 2);
    for (int i = 0; i < dataLength / 2; i++)
    {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        response[i] = msb << 8 | lsb;
    }
}

void TEF6686I2CComm::SetCommand(uint8_t module, uint8_t cmd, uint16_t *params, uint8_t paramsCount)
{
    Wire.beginTransmission(DEVICE_ADDR);
    Writei2c(module);
    Writei2c(cmd);
    Writei2c(1);
    for (int i = 0; i < paramsCount; i++)
    {
        uint8_t msb = params[i] >> 8;
        uint8_t lsb = params[i];
        Writei2c(msb);
        Writei2c(lsb);
    }
    Wire.endTransmission();
}
