#include <TEF6686.h>

void TEF6686::DspWriteData(const uint8_t *data)
{
    uint8_t *pa = (uint8_t *)data;
    uint8_t len, first;
    for (;;)
    {
        len = pgm_read_byte_near(pa++);
        first = pgm_read_byte_near(pa);
        if (!len)
            break;
        if (len == 2 && first == 0xff)
        {
            int delaytime = pgm_read_byte_near(++pa);
            delay(delaytime);
            pa++;
        }
        else
        {
            Wire.beginTransmission(DEVICE_ADDR);
            for (int i = 0; i < len; i++)
                Wire.write(pgm_read_byte_near(pa++));
            Wire.endTransmission();
        }
    }
}

void TEF6686::Init()
{
    delay(40);
    Wire.begin(21,22,100000);
    uint16_t state[1];
    do
    {
        GetCommand(64, 128, state, 1);
        if (state[0] < 2)
            DspWriteData(DSP_INIT);

        else if (state[0] >= 2)
        {
            Appl_Set_OperationMode(1);            
        }
        Serial.println(state[0]);
    } while (state[0] < 2);
}

void TEF6686::SetCommand(uint8_t module, uint8_t cmd, uint16_t *params, uint8_t paramsCount)
{
    // Serial.print("\nSend data: [");
    Wire.beginTransmission(DEVICE_ADDR);
    Writei2c(module);
    Writei2c(cmd);
    Writei2c(1); // index
    for (int i = 0; i < paramsCount; i++)
    {
        uint8_t msb = params[i] >> 8;
        uint8_t lsb = params[i];
        Writei2c(msb);
        Writei2c(lsb);
    }
    Wire.endTransmission();
    // Serial.print("]");
}

void TEF6686::GetCommand(uint8_t module, uint8_t cmd, uint16_t *response, uint8_t responseLength)
{
    SetCommand(module, cmd, nullptr, 0);
    uint8_t dataLength = Wire.requestFrom(DEVICE_ADDR, responseLength * 2);
    // Serial.print("\nReceive data: [");
    for (int i = 0; i < dataLength / 2; i++)
    {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        response[i] = msb << 8 | lsb;

        char text[16];
        // sprintf(text, " 0x%04x ", response[i]);
        sprintf(text, " %i ", response[i]);

        //      Serial.print(text);
    }
    //  Serial.print("]");
}

void TEF6686::Writei2c(uint8_t data)
{
    char text[8];
    sprintf(text, " 0x%02x ", data);
    //  Serial.print(text);
    Wire.write(data);
}

void TEF6686::Tune_To(uint8_t module, uint16_t freq)
{
    uint16_t params[2] = {1, freq};
    SetCommand(module, 0x1, params, 2);
    Currentfreq = freq;
    memset(rtText, 0, sizeof(rtText));
    memset(psText, 0, sizeof(psText));
}

void TEF6686::Audio_Set_Mute(uint8_t mute)
{
    uint16_t unmuteParams[1] = {mute};
    SetCommand(MODULE_AUDIO, 11, unmuteParams, 1);
}

void TEF6686::Appl_Set_OperationMode(uint8_t mode)
{
    uint16_t param[1] = {mode};
    SetCommand(MODULE_AUDIO, 1, param, 1);
}

uint16_t TEF6686::Get_Quality_Status()
{
    uint16_t result[7];
    GetCommand(MODULE_FM, 128, result, 7);
    quality = result[1];
    return result[1];
}


void TEF6686::Get_RDS_Status()
{
    uint16_t uRds_Data[6] = {0};
    GetCommand(MODULE_FM, 130, uRds_Data, 6);

    if (bitRead(uRds_Data[0], 15) == 1)
    {
        HandleGroup0(uRds_Data);
        HandleGroup2(uRds_Data);
    }
}

void TEF6686::HandleGroup0(uint16_t *rdsData)
{
    uint8_t groupType = (uint8_t)((rdsData[2] >> 12) & 0xff);

    if (groupType != 0)
        return;

    ms = (uint8_t)((rdsData[2] >> 3) & 1) == 1;
    ta = (uint8_t)((rdsData[2] >> 4) & 1) == 1;
    pty = (uint8_t)((rdsData[2] >> 5) & 0x1F);

    uint8_t offset = rdsData[2] & (uint16_t)3;
    psText[2 * offset] = (uint8_t)(rdsData[4] >> 8);
    psText[2 * offset + 1] = (uint8_t)(rdsData[4]);
}

void TEF6686::HandleGroup2(uint16_t *rdsData)
{
    uint8_t groupType = (uint8_t)((rdsData[2] >> 12) & 0xff);

    if (groupType != 2)
        return;

    bool versionA = bitRead(rdsData[0], 12) == 0;
    uint8_t offset = rdsData[2] & (uint16_t)0xf;

    if (versionA)
    {
        rtText[4 * offset] = (uint8_t)(rdsData[3] >> 8);
        rtText[4 * offset + 1] = (uint8_t)(rdsData[3]);
        rtText[4 * offset + 2] = (uint8_t)(rdsData[4] >> 8);
        rtText[4 * offset + 3] = (uint8_t)(rdsData[4]);
    }
    else
    {
        rtText[2 * offset] = (uint8_t)(rdsData[4] >> 8);
        rtText[2 * offset + 1] = (uint8_t)(rdsData[4]);
    }
}

void TEF6686::Get_Identification()
{
    uint16_t result[3];
    GetCommand(MODULE_APPL, 130, result, 3);
}