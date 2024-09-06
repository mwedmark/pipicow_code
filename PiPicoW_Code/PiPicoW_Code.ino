/*
   ____  __  __  ____  _  _  _____       ___  _____  ____  _  _
  (  _ \(  )(  )(_  _)( \( )(  _  )___  / __)(  _  )(_  _)( \( )
   )(_) ))(__)(  _)(_  )  (  )(_)((___)( (__  )(_)(  _)(_  )  (
  (____/(______)(____)(_)\_)(_____)     \___)(_____)(____)(_)\_)
  Official code for PiPico W boards            version 4.3
  Started life as official ESP32 code, changed by overclocked
  Main .ino file

  The Duino-Coin Team & Community 2019-2024 © MIT Licensed
  https://duinocoin.com
  https://github.com/revoxhere/duino-coin

  If you don't know where to start, visit official website and navigate to
  the Getting Started page. Have fun mining!

  To edit the variables (username, WiFi settings, etc.) use the Settings.h tab!
*/

/* If optimizations cause problems, change them to -O0 (the default) */
//#pragma GCC optimize("-Ofast")
#pragma GCC optimize("-Ofast")
/* If during compilation the line below causes a
  "fatal error: arduinoJson.h: No such file or directory"
  message to occur; it means that you do NOT have the
  ArduinoJSON library installed. To install it,
  go to the below link and follow the instructions:
  https://github.com/revoxhere/duino-coin/issues/832 */
#include <ArduinoJson.h>
#include <ArduinoMDNS.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <Ticker.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "MiningJob.h"
#include "Settings.h"

namespace 
{
    MiningConfig *configuration = new MiningConfig(
        DUCO_USER,
        RIG_IDENTIFIER,
        MINER_KEY
    );

    void UpdateHostPort(String input) 
    {
        JsonDocument doc;
        deserializeJson(doc, input);
        const char *name = doc["name"];

        configuration->host = doc["ip"].as<String>().c_str();
        configuration->port = doc["port"].as<int>();
        node_id = String(name);

        #if defined(SERIAL_PRINTING)
          Serial.println("Poolpicker selected the best mining node: " + node_id);
        #endif
    }

    void VerifyWifi() 
    {
      
      while (WiFi.status() != WL_CONNECTED 
              || WiFi.localIP() == IPAddress(0, 0, 0, 0)
              || WiFi.localIP() == IPAddress(192, 168, 4, 2) 
              || WiFi.localIP() == IPAddress(192, 168, 4, 3)) 
      {
        #if defined(SERIAL_PRINTING)
          Serial.println("WiFi reconnecting...");
        #endif
        WiFi.disconnect();
        delay(500);
        WiFi.begin(SSID,PASSWORD);
        delay(500);
      }
    }

    #define HTTP_CODE_OK 220
    #define HTTP_CODE_MOVED_PERMANENTLY 301

    String httpGetString(String serverAddr, String subUrl) 
    {
        String payload = "";
        
        WiFiClientSecure client;

        //EthernetClient c;
        HttpClient https(client, serverAddr, 443);
        client.setInsecure();
        //client.enableInsecure

        //https.begin(client, URL);

        https.sendHeader("Accept", "*/*");
        
        int httpCode = https.get(subUrl);
        #if defined(SERIAL_PRINTING)
            Serial.printf("HTTP Response code: %d\n", httpCode);
        #endif

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == 0) {
            payload = https.responseBody();
        } else {
            #if defined(SERIAL_PRINTING)
               Serial.printf("Error fetching node from poolpicker");//: %s\n", toString(https.getWriteError()).c_str());
               VerifyWifi();
            #endif
            #if defined(DISPLAY_SSD1306) || defined(DISPLAY_16X2)
              display_info(http.errorToString(httpCode));
            #endif
        }
        //https.end();
        https.endRequest();
        return payload;
    }

    void SelectNode() 
    {
        String input = "";
        int waitTime = 1;
        int poolIndex = 0;

        while (input == "") 
        {
            #if defined(SERIAL_PRINTING)
              Serial.println("Fetching mining node from the poolpicker in " + String(waitTime) + "s");
            #endif
            delay(waitTime * 1000);
            
            input = httpGetString("server.duinocoin.com", "/getPool");
            Serial.println(input); // check answer
        }

        UpdateHostPort(input);
    }

    void SetupWifi() 
    {
  
        #if defined(SERIAL_PRINTING)
            Serial.println("Connecting to: " + String(SSID));
        #endif
        WiFi.begin(SSID, PASSWORD);
        while(WiFi.status() != WL_CONNECTED) 
        {
            Serial.print(".");
            delay(1000);
        }
        VerifyWifi();
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), DNS_SERVER);
        
        #if defined(SERIAL_PRINTING)
            Serial.println("\n\nSuccessfully connected to WiFi");
            Serial.println("Rig name: " + String(RIG_IDENTIFIER));
            Serial.println("Local IP address: " + WiFi.localIP().toString());
            Serial.println("Gateway: " + WiFi.gatewayIP().toString());
            Serial.println("DNS: " + WiFi.localIP().toString());
            Serial.println();
        #endif

      SelectNode();
    }
}

MiningJob *job[NUMBER_OF_CORES];
bool hasBeenInitialized = false;

void setup() 
{
 
    #if defined(SERIAL_PRINTING)
        Serial.begin(SERIAL_BAUDRATE);
        Serial.println("\n\nDuino-Coin " + String(configuration->MINER_VER));
        Serial.println("Setting up thread 1");
    #endif
    pinMode(LED_BUILTIN, OUTPUT);

    WALLET_ID = String(random(0, 2811)); // Needed for miner grouping in the wallet

    for(int i=0;i<NUMBER_OF_CORES;i++)
    {
        job[i] = new MiningJob(i, configuration);
    }
    //WiFi.mode(WIFI_STA); // Setup ESP in client mode
    
    delay(1000);
    VerifyWifi();
    SelectNode();

    SetupWifi();

    job[0]->blink(BLINK_SETUP_COMPLETE);
}

void system_events_func(void* parameter) 
{
  while (true) 
  {
    delay(10);
  }
}

void core_loop(int core) 
//static void __no_inline_not_in_flash_func(core_loop)(int core)
{
    if(!hasBeenInitialized && core==0) hasBeenInitialized = true;
    while(true && hasBeenInitialized)
    {
      job[core]->mine();
      VerifyWifi();
    }
    delay(10);
}

void loop() 
{
    core_loop(0);
    WiFiClientSecure* client = new WiFiClientSecure();
    uint8_t data_crypt[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    client->setFingerprint(data_crypt);
    //setFingerprint(" ");
}

//uint8_t state = HIGH;

void loop1() 
{
     core_loop(1);
}