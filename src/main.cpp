#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans12pt7b.h>
#include <TEF6686.h>
#include <AiEsp32RotaryEncoder.h>
#include <map>

#define BACK_COLOR GxEPD_WHITE
#define TEXT_COLOR GxEPD_BLACK
#define FREQ_MAX 10800
#define FREQ_MIN 8800
#define FREQ_DISPLAY_MIN 8000
#define FREQ_DISPLAY_MAX 11000
#define QUALITY_MAX_VALUE 1200

#define ROTARY_ENCODER_A_PIN 27
#define ROTARY_ENCODER_B_PIN 26
#define ROTARY_ENCODER_BUTTON_PIN 14
#define ROTARY_ENCODER_STEPS 4

#define ENABLE_POWER_TEF6686_PIN 32

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);
TEF6686 tef;
uint8_t qualityOK;
unsigned long _lastRDSTime;
char displayText[256];
char text[256];
char rtText[256];
uint16_t qualityThreshold = 400;
bool scanning;
bool powerOff;
std::map<uint16_t, uint16_t> qualityMap;

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=5*/ 5, /*DC=*/19, /*RST=*/2, /*BUSY=*/15)); // DEPG0290BS 128x296, SSD1680

void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

unsigned long shortPressAfterMiliseconds = 50;
unsigned long longPressAfterMiliseconds = 350;

void PowerOff()
{
  powerOff=true;
  delay(1000);
  digitalWrite(ENABLE_POWER_TEF6686_PIN,LOW);
  esp_deep_sleep_start();
}

void ChangeMode()
{

}

void on_button_short_click()
{
  ChangeMode(); 
}

void on_button_long_click()
{
  Serial.println("Go sleep...");
  PowerOff();
}

void handle_rotary_button()
{
  static unsigned long lastTimeButtonDown = 0;
  static bool wasButtonDown = false;

  bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();

  if (isEncoderButtonDown)
  {
    if (!wasButtonDown)
    {
      lastTimeButtonDown = millis();
    }
    wasButtonDown = true;
    return;
  }

  if (wasButtonDown)
  {
    if (millis() - lastTimeButtonDown >= longPressAfterMiliseconds)
    {
      on_button_long_click();
    }
    else if (millis() - lastTimeButtonDown >= shortPressAfterMiliseconds)
    {
      on_button_short_click();
    }
  }
  wasButtonDown = false;
}

void Seek(int step)
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

void ScanAll(int step)
{
  scanning=true;
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
}

int16_t freqToX(uint16_t freq)
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
  while (true)
  {
    display.firstPage();
    do
    {
      if(powerOff)
      {
        display.clearScreen();
        break;
      }
      display.setTextSize(2);
      display.fillScreen(BACK_COLOR);
      display.setTextColor(TEXT_COLOR);
      display.setCursor(0, 5);
      display.print(displayText);
      if(scanning)
        display.print("SCANNING...");
        
      display.setFont(NULL);
      display.setCursor(0, 30);
      display.setTextSize(0);
      display.print(rtText);

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
      display.drawLine(freqToX(tef.Currentfreq), vOffset + display.height() / 2 - 20, freqToX(tef.Currentfreq), vOffset + display.height() / 2 + 20, TEXT_COLOR);

      int lastHeight = 0;
      int lastX = freqToX(FREQ_DISPLAY_MIN);
      freq = FREQ_DISPLAY_MIN;
      while (freq < FREQ_DISPLAY_MAX)
      {
        float qF = (float)qualityMap[freq] / (float)QUALITY_MAX_VALUE;
        int barHeight = qF * 100 - vOffset;
        display.drawLine(lastX, display.height() - lastHeight, freqToX(freq), display.height() - barHeight, TEXT_COLOR);
        lastX = freqToX(freq);
        lastHeight = barHeight;
        freq += 10;
      }
      float qF = (float)qualityThreshold / (float)QUALITY_MAX_VALUE;
      int thresholdLineY = qF * 100 - vOffset;
      DrawHorizontalDottedLine(display.height() - thresholdLineY);

    } while (display.nextPage());
  }
}

void setup()
{
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);
  pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENABLE_POWER_TEF6686_PIN, OUTPUT);
  digitalWrite(ENABLE_POWER_TEF6686_PIN, HIGH);
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  Serial.begin(115200);
  tef.Init(21, 22, 100000);
  tef.Audio_Set_Mute(1);
  display.init(115200, true, 50, false);
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  display.clearScreen();
  xTaskCreate(UpdateScreen, "UpdateScreen", 20000, NULL, 5, NULL);
  ScanAll(10);
  tef.Tune_To(tef.MODULE_FM, FREQ_MIN);
  Seek(10);
  tef.Audio_Set_Mute(0);
}

void loop()
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

  int encoderChanged = rotaryEncoder.encoderChanged();
  if (encoderChanged)
  {
    if (encoderChanged < 0)
    {
      Serial.println("Encoder --");
      Seek(-10);
      delay(300);
    }
    if (encoderChanged > 0)
    {

      Serial.println("Encoder ++");
      Seek(10);
      delay(300);
    }
  }
  handle_rotary_button();
}