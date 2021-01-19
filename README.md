# ESP32 TTGO CO2-Ampel

arduino sample sketch for ESP32 (here: LILYGO TTGO-T w. color oled) to measure CO2 concentration and display value on TFT, as well as use an 8-Bit RGB-LED-Ring to show/indicate CO2 concentration (ppm). In addition the value is published to an mqtt broker (using tls/ssl).

The LED-ring indicates green as good, yellow is warning level and red is critical.

It uses MH_Z19 sensor. During pre-heating (3 minutes) the LEDs show some spinning blue "wheel".

## CO2 Thresholds (ppm).

Recommendation from REHVA (Federation of European Heating, Ventilation and Air Conditioning associations, rehva.eu) for preventing COVID-19 aerosol spread especially in schools:

- warn: 800, critical: 1000 (<https://www.rehva.eu/fileadmin/user_upload/REHVA_COVID-19_guidance_document_V3_03082020.pdf>)

General air quality recommendation by the German Federal Environmental Agency (2008):

- warn: 1000, critical: 2000 (<https://www.umweltbundesamt.de/sites/default/files/medien/pdfs/kohlendioxid_2008.pdf>)

# Requirements / Dependencies / Parts used

## Parts

- [TTGO T-Display ESP32](https://de.aliexpress.com/item/4000509604970.html)
- [RGB LED Ring](https://de.aliexpress.com/item/32835427711.html)
- [MH-Z19](https://de.aliexpress.com/item/1005001888955609.html)

## Arduino Libraries

- arduinoota.h
- fastled.h
- tft_espi.h
- wifi.h
- pubsubclient.h
- softwareserial.h
- mhz.h

# Features / Ideas

## On-Screen (mqtt published) value display

On a Linux Desktop one could use a command like the following to have the co2 value displayed "on-screen" and even swith color (via mqtt) using osd_cat.

```
threshold_val=1000
threshold=0

while read co2val; do
    color='#0F0'
    if [ $co2val -gt $threshold_val -a $threshold -eq 0 ]; then
    color='#F00'
    fi
    if [ $co2val -lt $threshold_val -a $threshold -eq 1 ]; then
    color='#0F0'
    fi
    echo $co2val | osd_cat -p top -A center -o 25 -i -50 -f '-*-liberation mono-*-*-*-*-40-*-*-*-*-*-*-*' -s 2 -c $color;
done < <(mosquitto_sub -h yourhost.de -p 8883 -u username -P passw0rd --cafile myCA.pem --cert client.crt --key client.key -t co2_ampel1/co2ppm)
```

# OTA

## Issue

OTA failed: no response from device (icmp type 3, code 13)

## Fix

could be related to firewall rules, try either one of

```
$ sudo iptables -I INPUT  -s 192.168.1.123 -j ACCEPT
$ sudo iptables -I OUTPUT -d 192.168.1.123 -j ACCEPT
```

or

```
$ firewall-cmd --zone=trusted --add-source=192.168.1.123
```

I read that people also have similar issues (destination unreachable) when on different wifi networks (2.4GHz vs 4GHz) but at least here that is not a problem (esp on 2.4GHz and laptop on 5GHz)
