// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0


#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/Picopixel.h>
#include <uICAL.h>
#include <TEF6686.h>

TEF6686 tef;
uint8_t qualityOK;
unsigned long _lastRDSTime;
char displayText[256];
char text[256];
char rtText[256];

// int qualityMap[430];
std::map<uint16_t, uint16_t> qualityMap;

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=5*/ 5, /*DC=*/19, /*RST=*/2, /*BUSY=*/15)); // DEPG0290BS 128x296, SSD1680

void Seek(int step)
{
  uint16_t f = tef.Currentfreq;
   while(f<10800)
   {
     f+=step;
      if(qualityMap[f]>400)
      {
        tef.Tune_To(tef.MODULE_FM,f);
        qualityOK=1;
        return;
      }
     
   }
}

void ScanAll(int step)
{
  Serial.println("Start scan...");
  do
  {
    qualityOK = 0;
    tef.Tune_To(tef.MODULE_FM, tef.Currentfreq + step);
    delay(50);
    uint16_t q = tef.Get_Quality_Status();
    qualityMap[tef.Currentfreq] = q;
    Serial.printf("Freq: %i, Quality: %i \n", tef.Currentfreq, q);
  } while (tef.Currentfreq < 10800);
}

void DrawBox(int16_t x, int16_t y, int16_t w, int16_t h)
{
  display.drawLine(x, y, x + w, y, GxEPD_BLACK);
  display.drawLine(x, y, x, y + h, GxEPD_BLACK);
  display.drawLine(x, y + h, x + w, y + h, GxEPD_BLACK);
  display.drawLine(x + w, y, x + w, y + h, GxEPD_BLACK);
}

int16_t freqToX(uint16_t freq)
{
  int length = (10 * (display.width() / 11) + 15) - (0 * (display.width() / 11) + 15);
  int start = 0 * (display.width() / 11) + 15;
  float freqFact = (float)(freq - 8800) / (float)(10800 - 8800);
  return freqFact * length + start;
}

static void UpdateScreen(void *parameter)
{   
  while (true)
  {
    display.firstPage();
    do
    {
      display.setCursor(10, 90);
      // display.fillScreen(GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);

      display.setCursor(0, 10);
      display.setTextSize(2);
      display.print(displayText);
      display.setCursor(0, 30);
      display.setTextSize(0);
      display.print(rtText);

      int vOffset = 10;

      display.drawLine(0, vOffset + display.height() / 2, display.width(), vOffset + display.height() / 2, GxEPD_BLACK);

      int freq = 88;
      for (int i = 0; i < 11; i++)
      {
        int offset = i % 2 == 0 ? -20 : 5;

        display.drawLine(i * (display.width() / 11) + 15, vOffset + display.height() / 2 - 10, i * (display.width() / 11) + 15, vOffset + display.height() / 2, GxEPD_BLACK);
        display.setCursor(i * (display.width() / 11) + 10, vOffset + display.height() / 2 + offset);
        display.setTextSize(1);
        display.print(freq);
        freq += 2;
      }
      //  display.drawLine(currentFreq + (display.width() / 11) + 15, vOffset + display.height() / 2 - 20, currentFreq + (display.width() / 11) + 15, vOffset + display.height() / 2 + 20, GxEPD_BLACK);
      display.drawLine(freqToX(tef.Currentfreq), vOffset + display.height() / 2 - 20, freqToX(tef.Currentfreq), vOffset + display.height() / 2 + 20, GxEPD_BLACK);

      int lastHeight = 0;
      int lastX = freqToX(8000);
      freq = 8000;
      while (freq < 10800)
      {
        float qF = (float)qualityMap[freq] / (float)1200;
        int barHeight = qF * 70;

        //  display.drawLine(freqToX(f), display.height()  - barHeight, freqToX(f),  display.height() , GxEPD_BLACK);
        display.drawLine(lastX + 1, display.height() - lastHeight, freqToX(freq) + 1, display.height() - barHeight, GxEPD_BLACK);
        lastX = freqToX(freq);
        lastHeight = barHeight;
        freq += 10;
      }

    } while (display.nextPage());

    //delay(500);
  }
}

void setup()
{
  Serial.begin(115200);
  tef.Init();
  tef.Audio_Set_Mute(0);
  tef.Tune_To(tef.MODULE_FM, 8500);

  display.init(115200, true, 50, false);
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  // display.setFont(&Picopixel);
  display.setTextColor(GxEPD_BLACK);
  display.setTextSize(1);
  display.clearScreen();
  xTaskCreate(UpdateScreen, "UpdateScreen", 20000, NULL, 5, NULL);
  ScanAll(10);
  tef.Tune_To(tef.MODULE_FM, 8500);
  Seek(10);
}

void loop()
{
  unsigned long currentTime = millis();

  if (qualityOK && (currentTime - 87 > _lastRDSTime))
  {
    _lastRDSTime = currentTime;
    tef.Get_RDS_Status();
    tef.Get_Quality_Status();
    sprintf(text, "Freq: %i.%i, MS: %i, TA: %i, PTY: %s, PS: %s,  RT: %s, Quality: %i", tef.Currentfreq / 100, tef.Currentfreq % 100, tef.ms, tef.ta, ptyLabels[tef.pty], tef.psText, tef.rtText, tef.quality);

    Serial.println(text);
  }
  sprintf(displayText, "%i.%i Mhz    %s", tef.Currentfreq / 100, tef.Currentfreq % 100, tef.psText);
  sprintf(rtText, "%s", tef.rtText);
  if (Serial.available())
  {
    int readed = Serial.read();
    Serial.println("Readed: ");
    Serial.println(readed);
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
  }
  delay(10);
}