

int getCO2andTemp(int* data, int len) {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  // command to ask for data
  byte response[9]; // for answer
  if (2 != len)
    return -1;

  co2Serial.write(cmd, 9); //request PPM CO2

  // The serial stream can get out of sync. The response starts with 0xff, try to resync.
  while (co2Serial.available() > 0 &&
	 (unsigned char) co2Serial.peek() != 0xFF) {
    co2Serial.read();
  }

  memset(response, 0, 9);
  co2Serial.readBytes(response, 9);

  if (response[1] != 0x86) {
    DEBUG_PRINTLN("Invalid response from co2 sensor!");
    DEBUG_PRINTLN(response[1]);
    return -1;
  }

  // crc check
  byte crc = 0;
  for (int i = 1; i < 8; i++) {
    crc += response[i];
  }
  crc = 255 - crc + 1;
  if (response[8] != crc) {
    DEBUG_PRINTLN("CRC error!");
    return -2;
  }

  int responseHigh = (int) response[2];
  int responseLow  = (int) response[3];
  int temp         = ((int) response[4]) - 40;
  int ppm = (256 * responseHigh) + responseLow;

  data[0] = ppm;
  data[1] = temp;
  return 0;
}


