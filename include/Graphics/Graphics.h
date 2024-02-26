#ifndef GRAPHICS_H
#define GRAPHICS_H
#define ENABLE_GxEPD2_GFX 0
#include <common.h>
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <RadioApp/RadioApp.h>


GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=5*/ 5, /*DC=*/19, /*RST=*/2, /*BUSY=*/15)); // DEPG0290BS 128x296, SSD1680

int16_t FreqToX(uint16_t freq)
{
  int length = (10 * (display.width() / 11) + 15) - (0 * (display.width() / 11) + 15);
  int start = 0 * (display.width() / 11) + 15;
  float freqFact = (float)(freq - FREQ_MIN) / (float)(FREQ_MAX - FREQ_MIN);
  return freqFact * length + start;
}

void DrawHorizontalDottedLine(uint16_t Y)
{
  for (int x = 0; x < display.width(); x++)
  {
    if (x % 4 == 0)
      display.drawPixel(x, Y, TEXT_COLOR);
  }
}

static void UpdateScreen(void *parameter)
{
    RadioApp *app = (RadioApp*)parameter;

  while (true)
  {
    display.firstPage();
    do
    {
      if(app->powerOff)
      {
        display.clearScreen();
        break;
      }
      display.setTextSize(2);
      display.fillScreen(BACK_COLOR);
      display.setTextColor(TEXT_COLOR);
      display.setCursor(LEFT_OFFSET, 5);
      display.print(app->displayText);
      if(app->scanning)
        display.print("SCANNING...");
        
      display.setFont(NULL);
      display.setCursor(LEFT_OFFSET, 30);
      display.setTextSize(0);
      display.print(app->rtText);

      int vOffset = 10;
      display.drawLine(0, vOffset + display.height() / 2, display.width(), vOffset + display.height() / 2, TEXT_COLOR);

      int freq = 88;
      for (int i = 0; i < 11; i++)
      {
        int offset = i % 2 == 0 ? -20 : 5;

        display.drawLine(i * (display.width() / 11) + 15, vOffset + display.height() / 2 - 10, i * (display.width() / 11) + 15, vOffset + display.height() / 2, TEXT_COLOR);
        display.setCursor(i * (display.width() / 11) + 10, vOffset + display.height() / 2 + offset);
        display.setTextSize(1);
        display.print(freq);
        freq += 2;
      }
      display.drawLine(FreqToX(app->tef.Currentfreq), vOffset + display.height() / 2 - 20, FreqToX(app->tef.Currentfreq), vOffset + display.height() / 2 + 20, TEXT_COLOR);

      int lastHeight = 0;
      int lastX = FreqToX(FREQ_DISPLAY_MIN);
      freq = FREQ_DISPLAY_MIN;
      while (freq < FREQ_DISPLAY_MAX)
      {
        float qF = (float)app->qualityMap[freq] / (float)QUALITY_MAX_VALUE;
        int barHeight = qF * 100 - vOffset;
        display.drawLine(lastX, display.height() - lastHeight, FreqToX(freq), display.height() - barHeight, TEXT_COLOR);
        lastX = FreqToX(freq);
        lastHeight = barHeight;
        freq += 10;
      }
      float qF = (float)app->qualityThreshold / (float)QUALITY_MAX_VALUE;
      int thresholdLineY = qF * 100 - vOffset;
      DrawHorizontalDottedLine(display.height() - thresholdLineY);
      
      
      display.fillRect(LEFT_OFFSET, display.height()-8,50,8,GxEPD_WHITE);
      display.setCursor(LEFT_OFFSET, display.height()-8);      
      char modeText[32];
      const char* modeName;
      if(app->currentMode==SEEK)
        modeName = "Seek";
      else if(app->currentMode==MANUAL)
        modeName = "Manual";
      else if(app->currentMode==THRESHOLD)
        modeName="Threshold";  
      sprintf(modeText, "%s", modeName);
      display.print(modeText);

    } while (display.nextPage());
  }
}



#endif