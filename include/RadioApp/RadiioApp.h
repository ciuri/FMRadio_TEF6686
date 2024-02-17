#ifndef RADIOAPP_H
#define RADIOAPP_H

#include <common.h>
#include <TEF6686.h>
#include <map>

class RadioApp
{
private:
public:
    TEF6686 tef;
    uint8_t qualityOK;
    std::map<uint16_t, uint16_t> qualityMap;
    uint16_t qualityThreshold = 400;
    bool scanning;
    bool powerOff;

    void Start();
    void Seek(int step);
    void ScanAll(int step);
    void PowerOff();
    void LoopHandle();
    unsigned long _lastRDSTime;
    char displayText[256];
    char text[256];
    char rtText[256];
};

void RadioApp::Start()
{
    tef.Init(21, 22, 100000);
    tef.Audio_Set_Mute(1);
}

void RadioApp::PowerOff()
{
    powerOff = true;
    delay(1000);
    digitalWrite(ENABLE_POWER_TEF6686_PIN, LOW);
    esp_deep_sleep_start();
}

void RadioApp::Seek(int step)
{
    uint16_t f = tef.Currentfreq;
    while (f < FREQ_MAX)
    {
        f += step;
        if (qualityMap[f] > qualityThreshold)
        {
            tef.Tune_To(tef.MODULE_FM, f);
            qualityOK = 1;
            return;
        }
    }
}

void RadioApp::ScanAll(int step)
{
    scanning = true;
    tef.Tune_To(tef.MODULE_FM, FREQ_DISPLAY_MIN);
    Serial.println("Start scan...");
    do
    {
        qualityOK = 0;
        tef.Tune_To(tef.MODULE_FM, tef.Currentfreq + step);
        delay(50);
        tef.UpdateQualityStatus();
        qualityMap[tef.Currentfreq] = tef.quality;
        Serial.printf("Freq: %i, Quality: %i \n", tef.Currentfreq, tef.quality);
    } while (tef.Currentfreq < FREQ_DISPLAY_MAX);
    scanning = false;
    tef.Tune_To(tef.MODULE_FM, FREQ_MIN);
    Seek(10);
    tef.Audio_Set_Mute(0);
}

void RadioApp::LoopHandle()
{
    unsigned long currentTime = millis();

    if (qualityOK && (currentTime - 87 > _lastRDSTime))
    {
        _lastRDSTime = currentTime;
        tef.UpdateRDSStatus();
        tef.UpdateQualityStatus();
        Serial.printf(text, "Freq: %i.%i, MS: %i, TA: %i, PTY: %s, PS: %s,  RT: %s, Quality: %i", tef.Currentfreq / 100, tef.Currentfreq % 100, tef.rdsData.ms, tef.rdsData.ta, ptyLabels[tef.rdsData.pty], tef.rdsData.psText, tef.rdsData.rtText, tef.quality);
    }
    sprintf(displayText, "FM %i.%i Mhz  %s", tef.Currentfreq / 100, tef.Currentfreq % 100, tef.rdsData.psText);
    sprintf(rtText, "%s", tef.rdsData.rtText);
    if (Serial.available())
    {
        int readed = Serial.read();
        if (readed == 49)
            Seek(-10);
        else if (readed == 50)
            Seek(10);
        else if (readed == 51)
        {
            tef.Tune_To(tef.MODULE_FM, tef.Currentfreq - 10);
        }
        else if (readed == 52)
        {
            tef.Tune_To(tef.MODULE_FM, tef.Currentfreq + 10);
        }
        else if (readed == 53)
        {
            qualityThreshold -= 10;
        }
        else if (readed == 54)
        {
            qualityThreshold += 10;
        }
    }
    delay(10);
}
#endif