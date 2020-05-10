#include <IotWebConf.h> //Web Configuration https://github.com/prampec/IotWebConf
#include <time.h> //Time Module

const char thingName[] = "Napper";
const char wifiInitialApPassword[] = "123456789";

const int statusPin = 2; // pin to hold status
const int errorPin = 0; // pin to show error
const int CONFIG_PIN = 5;

const int analogInPin = A0; // Sensor Pin (1v)
int sensorValue = 0; //Starting Value


DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

//Custom Config Setting for Data Collector
//Note you only have 512 bytes of storage on eeprom
#define STRING_LEN 128
#define NUMBER_LEN 32

char monitorHostNameValue[STRING_LEN];
char timeZoneValue[NUMBER_LEN];
char dstValue[NUMBER_LEN];


IotWebConfParameter monitorParam = IotWebConfParameter("Monitor Host Name","monitorParam", monitorHostNameValue, STRING_LEN);
IotWebConfParameter timeZoneParam = IotWebConfParameter("Time Zone", "timeZoneParam", timeZoneValue, NUMBER_LEN, "number", "1..100", NULL, "min='1' max='100' step='1'");
IotWebConfParameter dstParam = IotWebConfParameter("DST", "dstParam", dstValue, NUMBER_LEN, "number", "0..1", NULL, "min='1' max='100' step='1'");

void setup() 
{
  pinMode(statusPin, OUTPUT); //Set status pin to output
  pinMode(errorPin, OUTPUT);  //Set error pin to output
  pinMode(analogInPin, INPUT); //Set Sensor pin to input
  
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");
  digitalWrite(errorPin, HIGH);
  digitalWrite(statusPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);

  
  
  // -- Initializing the configuration.
  iotWebConf.setStatusPin(statusPin);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameter(&monitorParam);
  iotWebConf.addParameter(&timeZoneParam);
  iotWebConf.addParameter(&dstParam);
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/data", handleDataRequest);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Ready.");
}

void loop() 
{
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>";
  s += thingName;
  s += "</title></head><body>";
  s += "<H1>";
  s += thingName;
  s += "</H1>";
  s += "<HR/>";
  s += "Monitor Host Name:" + String(monitorHostNameValue) + "<br/>";
  s += "Time Zone:" + String(atoi(timeZoneValue)) + "<br/>";
  s += "DST:" + String(atoi(dstValue)) + "<br/>";
  s += "<br/>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}


void handleDataRequest()
{
   // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal())
    {
      // -- Captive portal request were already served.
      return;
    }
    //Shoud set it up so the reader is powered down then up then down..
    //i.e. don't keep the sensor powered all the time...
    sensorValue = analogRead(analogInPin);


    // Connect to time site
    // Timezone -4 DST 1/0
    //https://github.com/esp8266/Arduino/blob/9913e5210779d2f3c4197760d6813270dbba6232/cores/esp8266/time.c#L58
    //configTime(atoi(timeZoneValue) * 3600, atoi(dstValue), "pool.ntp.org", "time.nist.gov"); //Local Time
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); //UTC
      Serial.println("\nWaiting for time");
      while (!time(nullptr)) {
        Serial.print(".");
        delay(1000);
      }
      Serial.println("received time");
      time_t now = time(nullptr);
      Serial.println(ctime(&now));
      
      struct tm * timeinfo;
      timeinfo = gmtime(&now);
      
      Serial.println(timeinfo->tm_hour);
      Serial.println(timeinfo->tm_hour);

      
    String s = "{";
    s += "\"SensorId\":\"" + String(ESP.getChipId(), HEX) + "\",";
    s += "\"SensorName\":\"" + String(thingName) + "\",";
    s += "\"Readings\":[";
    //Insert Sensor Readings Here - This whole thing needs cleaned up
    s +=    "{";
    s +=    "\"ReadingType\":\"Moisture\",";
    s +=    "\"ReadingUnit\":\"adc\",";
//  s +=    "\"ReadingDateTime\":\"1900-01-01 00:00:00.000\"," ; //1994-11-05T08:15:3
    s +=    "\"ReadingDateTime\":\"" + String(ctime(&now)) + "\"," ;
    s +=    "\"SensorValue\":\"";
    s +=        String(sensorValue);
    s +=        "\"";
    s +=    "}";
    s += "]";
    s += "}";

    server.send(200, "application/json", s);
}
