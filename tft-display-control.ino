
void drawVersion()
{
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(  0, 120);
  tft.print(ipAddr);
  tft.setCursor(120, 120);
  tft.print("Version: "); tft.print(VERSION);
}


void drawSensorData(int* data, int len)
{
  //tft.setTextDatum(TC_DATUM);

  tft.fillRect(0, 35, 240, 60, TFT_BLACK);
  // tft.println("");

  tft.setCursor(0, 40, 4); // posX, posY, font size=4
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("ppm:  ");
  tft.println(data[0]);
  tft.print("temp: ");
  tft.println(data[1]);
}


// tft drawing ...
/*
  0,0            240,0
  0,128          240,128

  b'000 - off
  b'100 - red     stop
  b'010 - yellow  attn. going red
  b'001 - green   go
  b'110 - red/yel attn. going green

 */

void drawTrafficLight(int mode, bool clear)
{
  int32_t radius = 15;

  int16_t x1, x2, x3;
  int16_t y1, y2, y3;
  int16_t d = 2;//TFT_HEIGHT / 2 - radius;

  // if (clear) {
  //   tft.fillRect(10, 18, 2*radius + 2*d, 100+2, TFT_BLACK);
  // }
  // 30 30 30 -> r=15 90px | o o o | 10/4=2.5 -> d=2
  tft.drawRect(10, 18, 2*radius + 2*d, 100+2, TFT_WHITE);

  x1 = 10 + d + radius;
  y1 = 18 + d + radius;
  x2 = x1;
  y2 = y1 + 2*d + 2*radius;
  x3 = x1;
  y3 = y2 + 2*d + 2*radius;

  tft.drawCircle(x1, y1, radius, TFT_RED);    // top    / red
  tft.drawCircle(x2, y2, radius, TFT_YELLOW); // middle / yellow
  tft.drawCircle(x3, y3, radius, TFT_GREEN);  // bottom / green

  if (mode & 4) {
    tft.fillCircle(x1, y1, radius, clear ? TFT_BLACK : TFT_RED);    // top    / red
    if (clear) tft.drawCircle(x1, y1, radius, TFT_RED);
  }
  if (mode & 2) {
    tft.fillCircle(x2, y2, radius, clear ? TFT_BLACK : TFT_YELLOW); // middle / yellow
    if (clear) tft.drawCircle(x2, y2, radius, TFT_YELLOW);
  }
  if (mode & 1) {
    tft.fillCircle(x3, y3, radius, clear ? TFT_BLACK : TFT_GREEN);  // bottom / green
    if (clear) tft.drawCircle(x3, y3, radius, TFT_GREEN);
  }

}
