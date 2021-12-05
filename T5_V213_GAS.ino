#define LILYGO_T5_V213
#include <boards.h>
#include <GxEPD.h>
#include <GxGDEH0213B73/GxGDEH0213B73.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include <time.h>               // In-built

#include "my_secret.h"

#define SCREEN_WIDTH   122
#define SCREEN_HEIGHT  250
#define barchart_on   true
#define barchart_off  false

const char* Timezone    = "CET-1CEST,M3.5.0,M10.5.0/3";  // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
const char* ntpServer   = "0.uk.pool.ntp.org";             // Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
                                                           // then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
                                                           // EU "0.europe.pool.ntp.org"
                                                           // US "0.north-america.pool.ntp.org"
                                                           // See: https://www.ntppool.org/en/     

int     CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     gmtOffset_sec     = 0;    // UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
int     daylightOffset_sec = 3600; // In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
enum    alignment {LEFT, RIGHT, CENTER};

//Day of the week
const char* weekday_D[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

//Month
const char* month_M[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// STORE data for displaying in GRAPH
const int readings = 5;
float DataArray[readings];

//-----------------------------------------------------------------------------
#include <HTTPClient.h>         // In-built
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
WiFiClient client;              // wifi client object

// INFLUXDB SERVER
// compaq:
//   192.168.0.119 : wireless
//   192.168.0.252 : fixed
// ProBook:
//   192.168.0.224 : wireless
//   192.168.0.160 : fixed
const char server[] = "192.168.0.160";
int        nbrRecords = 0;

//-----------------------------------------------------------------------------
bool pingInFluxDBServer(WiFiClient & client) {
  client.stop(); // close connection before sending a new request
  HTTPClient http;
  String uri = "/ping";
  http.begin(client, server, 8086, uri);
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // CODE : 204
    if (httpCode == HTTP_CODE_NO_CONTENT ) {
      Serial.printf("[HTTP] InfluxDB server is live on %s\n", server);
      client.stop();
      http.end();
      return true;
      }
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

//-----------------------------------------------------------------------------
bool decodeQuery(WiFiClient& json) {
  //Serial.print(F("[JSON] Creating JSON object...and "));
  //DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DynamicJsonDocument doc(2 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print(F("[JSON]decodeQuery(): deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  //Serial.println("decoding QUERY data :");

  nbrRecords = root["results"][0]["series"][0]["values"][0][1];
  Serial.printf("[JSON] decodeQuery: nbrRecords: %d\n", nbrRecords);

  return true;
}


//-----------------------------------------------------------------------------
float decodeQuery2(WiFiClient& json) {
  //Serial.print(F("[JSON] Creating JSON object...and "));
  //DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DynamicJsonDocument doc(2 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print(F("[JSON]decodeQuery2(): deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  //Serial.println("decoding QUERY data :");

  float gasCount[readings+1];
  for (int i=0; i <=  readings; i++)
  { gasCount[i] = root["results"][0]["series"][0]["values"][i][1]; }

  //float gasCount1 = root["results"][0]["series"][0]["values"][0][1];
  //float gasCount2 = root["results"][0]["series"][0]["values"][1][1];
  //float gasCount3 = root["results"][0]["series"][0]["values"][2][1];
  //float gasCount4 = root["results"][0]["series"][0]["values"][3][1];
  //float gasCount5 = root["results"][0]["series"][0]["values"][4][1];
  //float gasCount6 = root["results"][0]["series"][0]["values"][5][1];

  for (int i=0; i <  readings; i++)
  { DataArray[i] = gasCount[readings-i] - gasCount[readings-i-1]; }
  
  //DataArray[0] = gasCount6-gasCount5;  // TODAY
  //DataArray[1] = gasCount5-gasCount4;  // TODAY -1
  //DataArray[2] = gasCount4-gasCount3;  // TODAY -2
  //DataArray[3] = gasCount3-gasCount2;  // TODAY -3
  //DataArray[4] = gasCount2-gasCount1;  // TODAY -4

  //Serial.printf("[JSON] decodeQuery: gasCount1: %.1f\n", gasCount1);
  //Serial.printf("[JSON] decodeQuery: gasCount2: %.1f\n", gasCount2);
  //Serial.printf("[JSON] decodeQuery: gasCount3: %.1f\n", gasCount3);
  //Serial.printf("[JSON] decodeQuery: gasCount4: %.1f\n", gasCount4);
  //Serial.printf("[JSON] decodeQuery: gasCount5: %.1f\n", gasCount5);
  //Serial.printf("[JSON] decodeQuery: gasCount6: %.1f\n", gasCount6);

  //for (int i=0; i <  readings; i++)
  //{ Serial.printf("decodeQuery2: DataArray[%d]-> %.2f\n",i,DataArray[i]); }
  //Serial.printf("decodeQuery2: DataArray[0]-> %.2f m3 spend TODAY\n",DataArray[0]);

  return(DataArray[0]);
  //return(gasCount6-gasCount5);
}


//-----------------------------------------------------------------------------
bool queryInFluxDBServer(WiFiClient & client, String & uri) {
  client.stop(); // close connection before sending a new request
  HTTPClient http;

  
  http.useHTTP10(true);
  http.begin(client, server, 8086, uri);

  int httpCode = http.GET();
  //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  // httpCode will be negative on error
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      //Serial.print(http.getString());
      decodeQuery(http.getStream());
      client.stop();
      http.end();
      return true;
    }
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
  }
  client.stop();
  http.end();
  return false;
}


//-----------------------------------------------------------------------------
float queryInFluxDBServer2(WiFiClient & client, String & uri) {
  client.stop(); // close connection before sending a new request
  HTTPClient http;

  http.useHTTP10(true);
  http.begin(client, server, 8086, uri);

  int httpCode = http.GET();
  //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  // httpCode will be negative on error
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      //Serial.print(http.getString());
      float delta = decodeQuery2(http.getStream());
      client.stop();
      http.end();
      return(delta);
    }
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
  }
  client.stop();
  http.end();
  return(0.0);
}

//-----------------------------------------------------------------------------
//#include <InfluxDbClient.h>


//#define INFLUXDB_URL "http://192.168.0.160:8086"
// InfluxDB v1 database name 
//#define INFLUXDB_DB_NAME "energydb"

// InfluxDB client instance for InfluxDB 1
//InfluxDBClient clientdb(INFLUXDB_URL, INFLUXDB_DB_NAME);


//-----------------------------------------------------------------------------
boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
    Serial.println("UpdateLocalTime(): Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  Serial.println(&timeinfo, "%A %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  //sprintf(day_output, "%s %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
  sprintf(day_output, "%s %02u %s", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon]);
  strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   
  sprintf(time_output, "%s", update_time);
  Date_str = day_output;
  Time_str = time_output;
  return true;
}

//-----------------------------------------------------------------------------
//void printLocalTime()
//{
//  struct tm timeinfo;
//  if(!getLocalTime(&timeinfo)){
//    Serial.println("Failed to obtain time");
//    return;
//  }
//  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//}

//-----------------------------------------------------------------------------
boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  //printLocalTime();
  return UpdateLocalTime();
}
//-----------------------------------------------------------------------------
uint8_t StartWiFi() {
  int     wifi_signal;
  Serial.println("Connecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Use Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return WiFi.status();
}

//-----------------------------------------------------------------------------
void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi switched Off");
}


//-----------------------------------------------------------------------------
void cleanDisplay() {
  for (uint16_t r = 0; r < 2; r++) {
    display.fillScreen(GxEPD_WHITE);
    display.updateWindow(0,0,display.width(), display.height());
    delay(500);
  }
}


//-----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(String(__FILE__) + "\nStarting...");

  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);

  display.init();
  display.eraseDisplay();
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    //Serial.print("Screen width: ");
    //Serial.print(display.width());
    //Serial.print(" , height: ");
    //Serial.println(display.height());
    // Check INFLUXDB server connection
    pingInFluxDBServer(client);
  }
  cleanDisplay();
}

//-----------------------------------------------------------------------------
void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  //epd_draw_hline(x0, y0, length, color, framebuffer);
  display.drawLine(x0, y0, x0+length, y0, color);
}
//-----------------------------------------------------------------------------
void drawString(int x, int y, String text, alignment align) {
  //char * data  = const_cast<char*>(text.c_str());
  //int  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  //int w; 
  int h=0;
  //int xx = x, yy = y;
  //get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  //if (align == RIGHT)  x = x - w;
  //if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  //write_string(&currentFont, data, &x, &cursor_y, framebuffer);
  display.setCursor(x , cursor_y);
  display.print(text);
}


//-----------------------------------------------------------------------------
void drawGraph(boolean barchart_mode)
{
#define auto_scale_margin 0 // Sets the autoscale increment
 int x_pos = 24;
 int y_pos = 100;
 int gwidth = 100;
 int gheight = 100;
 float Y1Min = 0.0 ;
 float Y1Max = 20.0 ;
 String title = "GAS m3";

 int last_x, last_y;
 float x2, y2;
 boolean auto_scale = true;
 int maxYscale = -10000;
 int minYscale =  10000;

  if (auto_scale == true) {
    //for (int i = 1; i < readings; i++ ) {
    for (int i = 0; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }

 // Draw the graph
 //last_x = x_pos + 1;
 last_x = x_pos;
 //last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
 last_y = y_pos + (Y1Max - constrain(DataArray[0], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;

 display.drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, GxEPD_BLACK);
 //display.drawString(x_pos - 20 + gwidth / 2, y_pos - 28, title, CENTER); 
 display.setCursor(x_pos + 10 , y_pos - 10);
 display.print(title);

 for (int gx = 0; gx < readings; gx++) {
    //x2 = x_pos + (gx+1) * gwidth / (readings - 1) - 1 ;
    x2 = x_pos + (gx+1) * gwidth / (readings) - 2 ;
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;

    if(barchart_mode)
    {
      display.fillRect(last_x + 2, y2, (gwidth / readings) - 4, y_pos + gheight - y2 + 2, GxEPD_BLACK);
      //Serial.printf("drawGraph: DataArray[%d]-> %.2f\n",gx,DataArray[gx]); 
    }
    else
    {
      display.drawLine(last_x, last_y - 1, x2, y2 - 1, GxEPD_BLACK); // Two lines for hi-res display
      display.drawLine(last_x, last_y, x2, y2, GxEPD_BLACK);
    }
    last_x = x2;
    last_y = y2;
 }

 //Draw the Y-axis scale
#define number_of_dashes 20
#define y_minor_axis 5      // 5 y-axis division markers
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
      if (spacing < y_minor_axis) drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), GxEPD_BLACK);
    }
    //if ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) < 5 ) {
      //drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
      //drawString(x_pos - 15, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
    //}
    //else
    //{
      //if (Y1Min < 1 && Y1Max < 5) {
      if (Y1Max < 15) {
        //drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
        drawString(x_pos - 24, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
      }
      else {
        //drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
        drawString(x_pos - 15, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
      }
    //}    
  }
}

//-----------------------------------------------------------------------------
void drawDateTime()
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  //display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(5, 5);
  display.print(Date_str);
  display.setCursor(5, 15);
  display.print(Time_str);
  //display.drawLine(0,0,10,100,GxEPD_BLACK);
}

//-----------------------------------------------------------------------------
//void queryInfluxDB()
//{
//  String influxQL = "select min(gas) from log where time > now() - 10m";
  //if (clientdb.query(influxQL) != DB_SUCCESS) {
  //   Serial.print("InfluxDB Query FAILED."); 
  //}
//}
//-----------------------------------------------------------------------------
void loop() {
  char   counterString[30];
  char   gasSpendString[30];
  
  UpdateLocalTime();
  //display.drawPaged(drawDateTime);
  drawDateTime();

  if(pingInFluxDBServer(client)) { 
    
    String URI1 = "/query?db=energydb&q=SELECT+count(gas)+from+log";
    Serial.print("URI1: ");    Serial.println(URI1);
    if(queryInFluxDBServer(client, URI1)) { 
      //Serial.printf("nbr gas records: %d\n", nbrRecords );
      sprintf(counterString, "#records: %d", nbrRecords);
      display.setCursor(5, 30);
      display.print(counterString);
    }

    String URI2 = "/query?db=energydb&q=SELECT+max(gas)+from+log+WHERE+time+>=+now()+-5d+GROUP+BY+time(24h)";
    Serial.print("URI2: ");    Serial.println(URI2);
    float gasSpendToday = queryInFluxDBServer2(client, URI2); 
    Serial.printf("gasSpendToday: %.3f \n",gasSpendToday);
    sprintf(gasSpendString, "Today: %.3f m3", gasSpendToday);
    display.setCursor(5, 45);
    display.print(gasSpendString);

    drawGraph(barchart_on);
    
  }

  display.update();
  //display.powerDown();
  //delay(30000);
  delay(50000);
}
