#include <AiEsp32RotaryEncoder.h>
#include <Graphics/Graphics.h>
#include <RadioApp/RadioApp.h>

RadioApp radioApp;
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);

unsigned long shortPressAfterMiliseconds = 50;
unsigned long longPressAfterMiliseconds = 350;

void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void on_button_short_click()
{
  radioApp.ChangeMode();
}

void on_button_long_click()
{
  Serial.println("Go sleep...");
  radioApp.PowerOff();
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

void setup()
{
  Serial.begin(115200);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);
  pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENABLE_POWER_TEF6686_PIN, OUTPUT);
  digitalWrite(ENABLE_POWER_TEF6686_PIN, HIGH);
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  radioApp.Start();
  display.init(115200, true, 50, false);
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  display.clearScreen();
  xTaskCreate(UpdateScreen, "UpdateScreen", 20000, &radioApp, 5, NULL);
  radioApp.ScanAll(10);
}

void loop()
{
  radioApp.LoopHandle();

  int encoderChanged = rotaryEncoder.encoderChanged();
  if (encoderChanged)
  {
    if (encoderChanged < 0)
    {
      Serial.println("Encoder --");
      if (radioApp.currentMode == SEEK)
        radioApp.Seek(-10);
      else if (radioApp.currentMode == MANUAL)
        radioApp.TuneIncrement(-10);
      else if (radioApp.currentMode == THRESHOLD)
        radioApp.qualityThreshold -= 10;
      delay(300);
    }
    if (encoderChanged > 0)
    {

      Serial.println("Encoder ++");
      if (radioApp.currentMode == SEEK)
        radioApp.Seek(10);
      else if (radioApp.currentMode == MANUAL)
        radioApp.TuneIncrement(10);
      else if (radioApp.currentMode == THRESHOLD)
        radioApp.qualityThreshold += 10;
      delay(300);
    }
  }
  handle_rotary_button();
}