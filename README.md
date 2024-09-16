This is a self-contained Duino-coin miner written to work with Raspberry Pi Pico W boards, thus it can mine entirely by itself.
You need to provide details about your WIFI network, Duino-coin user, number of active cores and maybe a specific identifier (as default "PiPicoW" is used)
So in the top of of file "Settings.h" you need to set:

```
extern char *DUCO_USER = "";
extern char *MINER_KEY = "";
extern char *RIG_IDENTIFIER = "PiPicoW";
extern const char SSID[] = "";
extern const char PASSWORD[] = "";
```

**Building:**
Make sure you select the board type "Pi Pico W" which includes supports WIFI/BT. 
The following Libraries must be installed via the Library manager:
- ArduinoHttpClient by Arduino (v0.6.1+)
- ArduinoMDNS by Georg Kaindl  (v1.0.0+)
- ArduinoJson by Benoit Blanchon (v7.1.0+)
- Ticker by Stefan Staub (v4.4.0+)
