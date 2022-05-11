
void fillSolid(struct CRGB * leds, int start, int numToFill, const struct CRGB& color)
{
  if (numToFill > NUM_LEDS) numToFill = NUM_LEDS;
  for (int i = start; i < (start + numToFill); ++i) {
    leds[i] = color;
  }
}

void ledShowStatus(int* data, int len)
{
  if (false == warnReached && data[0] > warnThreshold) {
    warnReached = true;
    //fillSolid(leds, 0, NUM_LEDS, CRGB::Black);
    fillSolid(leds, 0, NUM_LEDS, CRGB::Yellow);
    FastLED.show();
  }
  if (false == criticalReached && data[0] > criticalThreshold) {
    criticalReached = true;
    fillSolid(leds, 0, NUM_LEDS, CRGB::Red);
    FastLED.show();
  }

  if (criticalReached && data[0] < criticalThreshold && data[0] > warnThreshold) {
    criticalReached = false;
    fillSolid(leds, 0, NUM_LEDS, CRGB::Yellow);
    FastLED.show();
  }
  if (warnReached && data[0] < warnThreshold) {
    warnReached = false;
    criticalReached = false;
    fillSolid(leds, 0, NUM_LEDS, CRGB::Green);
    FastLED.show();
  }
}

void ledSpin(const unsigned sec) {
  unsigned long t, t2;
  unsigned long remaining_sec = sec;
  t = t2 = millis();

  while(1) {
    for (unsigned n = 0; n < NUM_LEDS; ++n) {
      leds[n] = CRGB::Blue;
      leds[(n+1) % NUM_LEDS] = CRGB::Blue;
      leds[(n+2) % NUM_LEDS] = CRGB::Blue;
      //leds[(n+3) % NUM_LEDS] = CRGB::Blue;

      leds[(n-1) % NUM_LEDS] = CRGB::Blue;
      leds[(n-1) % NUM_LEDS] /= 2;
      leds[(n-2) % NUM_LEDS] = CRGB::Blue;
      leds[(n-2) % NUM_LEDS] /= 3;
      leds[(n-3) % NUM_LEDS] = CRGB::Blue;
      leds[(n-3) % NUM_LEDS] /= 4;

      leds[(n-4) % NUM_LEDS] = CRGB::Black;
      FastLED.show();
      delay(100);
    } // approx. 800 ms (NUM_LEDS * 100 ms delay)
    if ((millis() - t) > (sec * 1000))
      return;

    if ((millis() - t2) > 2000) { // once every two seconds
      t2 = millis();
      remaining_sec -= 2;
      DEBUG_PRINTLN(remaining_sec);
      tft.fillRect(100, 52, 50, 50, TFT_BLACK);
      tft.setCursor(100, 60);
      tft.print(remaining_sec); tft.print(" sec");
    }
  }
}

