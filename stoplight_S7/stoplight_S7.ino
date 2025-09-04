/* References: 
https://randomnerdtutorials.com/stepper-motor-esp32-web-server/
Add EDT
*/

//#include <ESP8266WiFi.h>
#include <WiFi.h>
//#include <ESPAsyncTCP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <SPIFFS.h>
#include <LittleFS.h>

#define WIFI_SSID "EngineeringSubNet"
#define WIFI_PASS "password"

#define HOURS 0
#define MINUTES 1

#define WARNING_TIME 5        // Amount of time set for warning before the bell in minutes
#define CLEANING_TIME 13      // Amount of time set for afternoon cleaning in minutes

#define GRN_LIGHT 3   
#define YEL_LIGHT 10  
#define RED_LIGHT 4   
#define NOT_LIGHT 0

// Day light savings variables
#define MAR 3       // Begin EDT
#define NOV 11      // Begin EST
#define SUN 0
#define WEEK 7
#define NOTSET 3
#define MAGIC_NUMBER 8

#define ON 1
#define OFF 0

// System states
#define NOSTATE 0
#define INCLASS 1
#define WARNING 2
#define PASSING 3
#define LAST_PERIOD 4
#define CLEANUP 5
#define AFTERSCHOOL 6
#define PERIOD_RESET 7
#define BEFORESCHOOL 8

// Off set in seconds from GMT to EST and EDT
#define EST -18000
#define EDT -14400

#define MIDNIGHT 0

#define RS 0      // Regular schedule
#define ER 1      // Early release
#define AA 2      // Advisory activity
#define EA 3      // Extended advisory

//bool dst_state;

// The initial state of savings time is unknown
uint8_t est_state = NOTSET;

// Default to EST for the inital savings time
const long utcOffsetInSeconds = EST;

// Should show host name network but this does not work yet
String hostname = "Stoplight V105";

AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char* HTML_FORM_1 = "sch";
//const char* HTML_FORM_2 = "dst";

extern const char* scheduleNames[];
//extern const char* dstState[];
extern const char* stateNames[];

const char* stateNames[] = { "Not Set", "In Class", "Warning", "Passing", "Last Period", "Clean Up", "After School", "Period Reset", "Before School" };

extern const char* schRS[];
extern const char* schER[];
extern const char* schAA[];

String schedule;
//String daylightsavings;

bool newRequest = false;

// For testing
//uint32_t timeThis, timeLast;

uint8_t state = NOSTATE;

//Schedule tracking variables
int8_t sch_index;
uint8_t sch_str = 0;  //Defaults to regular schedule
uint8_t sch_end = 17;

uint16_t day_str;
uint16_t day_end;
uint16_t day_cln;

uint16_t period_str;
uint16_t period_end;
uint16_t period_wrn;
uint16_t period_nxt;

//Changed the old limits to the updated one. 
uint8_t sch_limits[4][2]{
  { 0, 17 },
  { 18, 33 },
  { 34, 53 },
  { 54, 71 }
};

uint8_t hrs, mns, scs;

//Added the 7th period to each bell schedule alongside updating the times for the bell.
uint8_t schedules[72][2]{

  //REGULAR TIME SCHEDULE:
  //Index (0-15)
  //(ADVISORY)
  { 7, 45 },  // #0: 465 <- Index # and the conversion of hours and minutes to total minutes since 12:00 am
  { 7, 48 },  // #1: 469
  //(PERIOD 1)
  { 7, 51 },  // #2: 472
  { 8, 42 },  // #3: 528
  //(PERIOD 2)
  { 8, 45 },  // #4: 531
  { 9, 36 },  // #5: 587
  //(PERIOD 3)
  { 9, 39 },   // #6: 590
  { 10, 30 },  // #7: 640
  //(PERIOD 4)
  { 10, 33 },  // #8: 651
  { 11, 24 },  // #9: 707
  //(PERIOD 5)
  { 11, 27 },  // #10: 710
  { 12, 18 },  // #11: 738
  //(LUNCH)
  { 12, 21 },  // #12: 743
  { 12, 45 },  // #13: 799
  //(PERIOD 6)
  { 12, 48 },  // #14: 802
  { 13, 39 },  // #15: 858
  //(PERIOD 7)
  { 13, 42 },  // #16: 802
  { 14, 33 },  // #17: 858

  //EARLY RELEASE SCHEDULE:
  //Index (16-29)
  //(ADVISORY)
  { 7, 45 },  // #18: 465
  { 7, 48 },  // #19: 470
  //(PERIOD 1)
  { 7, 51 },  // #20: 473
  { 8, 24 },  // #21: 515
  //(PERIOD 2)
  { 8, 27 },  // #22: 518
  { 9, 00 },  // #23: 556
  //(PERIOD 3)
  { 9, 03 },  // #24: 559
  { 9, 36 },  // #25: 597
  //(PERIOD 4)
  { 9, 39 },  // #26: 600
  { 10, 12 },  // #27: 638
  //(PERIOD 5)
  { 10, 15 },  // #28: 641
  { 10, 48 },  // #29: 679
  //(PERIOD 6)
  { 10, 51 },  //#30: 682
  { 11, 24 },  //#31: 720
  //(PERIOD 7)
  { 11, 27 },  // #32: 802
  { 12, 00 },  // #33: 858

  //ADVISORY ACTIVITY SCHEDULE:
  //Index (30-47)
  //(ADVISORY)
  { 7, 45 },  // #34: 465
  { 7, 49 },  // #35: 470
  //(PERIOD 1)
  { 7, 52 },  // #36: 515
  { 8, 36 },  // #37: 521
  //(PERIOD 2)
  { 8, 39 },  // #38: 524
  { 9, 23 },  // #39: 572
  //(PERIOD 3)
  { 9, 26 },   // #40: 575
  { 10, 10 },  // #41: 619
  //(ADVISORY ACTIVITY)
  { 10, 13 },  // #42: 622
  { 10, 57 },  // #43: 670
  //(PERIOD 4)
  { 11, 00 },  // #44: 675
  { 11, 44 },  // #45: 723
  //(PERIOD 5)
  { 11, 47 },  // #46: 726
  { 12, 31 },  // #47: 756
  //(LUNCH)
  { 12, 34 },  // #48: 759
  { 12, 59 },  // #49: 807
  //(PERIOD 6)
  { 13, 02 },  // #50: 810
  { 13, 46 },  // #51: 858
  //(PERIOD 7)
  { 13, 49 },  // #52: 802
  { 14, 33 },  // #53: 858

  //EXTENDED ADVISORY SCHEDULE:
  //Index (48-63)
  //(ADVISORY)
  { 7, 45 },  // #54: 465
  { 8, 15 },  // #55: 495
  //(PERIOD 1)
  { 8, 18 },  // #56: 498
  { 9, 05 },  // #57: 550
  //(PERIOD 2)
  { 9, 8 },   // #58: 553  Leading zero omitted to avoid 08 literal compilation error
  { 9, 55 },  // #59: 605
  //(PERIOD 3)
  { 9, 58 },   // #60: 608
  { 10, 45 },  // #61: 660
  //(PERIOD 4)
  { 10, 48 },  // #62: 663
  { 11, 35 },  // #63: 715
  //(PERIOD 5)
  { 11, 38 },  // #64: 718
  { 12, 25 },  // #65: 748
  //(LUNCH)
  { 12, 28 },  // #66: 751
  { 12, 53 },  // #67: 803
  //(PERIOD 6)
  { 12, 56 },  // #68: 806
  { 13, 43 },  // #69: 858
  //(PERIOD 7)
  { 13, 46 },  // #70: 802
  { 14, 33 },  // #71: 858
};

void turn_on(uint8_t color) {
  digitalWrite(GRN_LIGHT, OFF);
  digitalWrite(YEL_LIGHT, OFF);
  digitalWrite(RED_LIGHT, OFF);
  if (color)
    digitalWrite(color, ON);
}

// Converts hours and minutes into total minutes
uint16_t total_minutes(uint8_t hrs, uint8_t mns) {
  return (hrs * 60) + mns;
};

void setup() {
  Serial.begin(115200);

  //TRIED TO SET HOSTNMAME BUT FAILED
  // WiFi.mode(WIFI_STA);
  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  // WiFi.setHostname(hostname.c_str());

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start file system
  if (!LittleFS.begin()) {
    Serial.println("File system failed to initialize.");
    return;
  } else
    Serial.println("File system initialized.");

/*
  if (!SPIFFS.begin()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }*/

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("html");
    //request->send(SPIFFS, "/index.html", "text/html");
   // request->send(SPIFFS, "/index.html", String(), false, processor);
   request->send(LittleFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("css");
   // request->send(SPIFFS, "/styles.css", "text/css");
    request->send(LittleFS, "/styles.css", "text/css");
  });

  // Load on form submission
  server.on("/", HTTP_POST, [](AsyncWebServerRequest* request) {
    Serial.print("request");
    int params = request->params();
    for (int i = 0; i < params; i++) {
      const AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        if (p->name() == HTML_FORM_1) {
          schedule = p->value().c_str();
          sch_str = sch_limits[schedule.toInt()][0];
          sch_end = sch_limits[schedule.toInt()][1];
          //Serial.println(schedule);
        }
        // if (p->name() == HTML_FORM_2) {
        //   daylightsavings = p->value().c_str();
        //dst_state = daylightsavings.toInt();
        //Serial.println(daylightsavings);
        //  if (daylightsavings == "1")
        //timeClient.setTimeOffset(EDT);
        // else
        //timeClient.setTimeOffset(EST);
        //}
        newRequest = true;
      }
    }
    state = NOSTATE;
   // request->send(SPIFFS, "/index.html", String(), false, processor);
   request->send(LittleFS, "/index.html", String(), false, processor);
  });

  //Start server
  server.begin();
  Serial.println("HTTP server started");

  pinMode(GRN_LIGHT, OUTPUT);
  pinMode(YEL_LIGHT, OUTPUT);
  pinMode(RED_LIGHT, OUTPUT);

  timeClient.begin();

  //while(1);

  // Sets time in minutes for start of school, end of school, and start of clean up.
  // day_str = total_minutes(schedules[sch_str][HOURS], schedules[sch_str][MINUTES]);
  // day_end = total_minutes(schedules[sch_end][HOURS], schedules[sch_end][MINUTES]);
  //day_cln = day_end - CLEANING_TIME;
}

//Changed the char.Sechs to have the correct amount of periods and placement of events (Lunch, Advisory Activity, etc).
String processor(const String& var) {
  const char* scheduleNames[] = { "Regular Schedule", "Early Release", "Advisory Activity", "Extended Advisory" };
  //const char* dstState[] = { "EST", "EDT" };
  const char* estState[] = { "EDT", "EST" };
  const char* schRS[] = { "Advisory", "Period 1", "Period 2", "Period 3", "Period 4", "Period 5", "Lunch", "Period 6", "Period 7" };
  const char* schER[] = { "Advisory", "Period 1", "Period 2", "Period 3", "Period 4", "Period 5", "Period 6", "Period 7" };
  const char* schAA[] = { "Advisory", "Period 1", "Period 2", "Period 3", "Advisory Activity", "Period 4", "Period 5", "Lunch", "Period 6", "Period 7" };
  // const char* stateNames[] = {"Not Set",  "In Class", "Warning", "Passing", "Last Period", "Clean Up", "After School", "Period Reset", "Before School" };

  if (var == "SET_SCHEDULE") {
    return scheduleNames[schedule.toInt()];
  }
  if (var == "SET_DST") {
    // return dstState[daylightsavings.toInt()];
    return estState[est_state];
  }
  if (var == "TIME") {
    char buf[12];
    sprintf(buf, "%02d:%02d:%02d", hrs, mns, scs);
    return buf;
  }
  if (var == "PERIOD") {
    uint8_t period = (sch_index - sch_str) / 2;
    uint8_t schInt = schedule.toInt();
    if ((state != BEFORESCHOOL) && (state != AFTERSCHOOL)) {
      if ((schInt == RS) || (schInt == EA))
        return (schRS[period]);
      else if (schInt == ER)
        return (schER[period]);
      else if (schInt == AA)
        return (schAA[period]);
    }
  }
  if (var == "STATE")
    return stateNames[state];

  return String();
}

void loop() {

// TESTING A RESET OPTION
 // if (WiFi.status() != WL_CONNECTED) ESP.restart();

  // Get time
  timeClient.update();
  hrs = timeClient.getHours();
  mns = timeClient.getMinutes();
  scs = timeClient.getSeconds();

  // Day of week, month and day
  uint8_t dayofweek = timeClient.getDay();
  String formattedDate = timeClient.getFormattedDate();
  uint8_t splitDash = formattedDate.indexOf("-") + 1;
  String mnt = formattedDate.substring(splitDash, splitDash + 2);
  splitDash += 3;
  String dte = formattedDate.substring(splitDash, splitDash + 2);

  uint8_t month = mnt.toInt();
  uint8_t day = dte.toInt();

  // if (((month > MAR) && (month < NOV)) ||
  // ((month == MAR) && (day > WEEK) && (dayofweek == SUN)) ||
  // ((month == MAR) && (day > WEEK * 2)) ||
  // ((month == NOV) && (day < WEEK) && (dayofweek < day)))
  // {

  uint8_t previousSunday = day - dayofweek;

  if (((month > MAR) && (month < NOV)) || ((month == MAR) && (previousSunday >= MAGIC_NUMBER)) || ((month == MAR) && (day > WEEK * 2)) || ((month == NOV) && (previousSunday < 1))) {
    if ((est_state == true) || (est_state == NOTSET)) {
      timeClient.setTimeOffset(EDT);
      est_state = false;
    }
  } else {
    if ((est_state == false) || (est_state == NOTSET)) {
      timeClient.setTimeOffset(EST);
      est_state = true;
    }
  }

  uint16_t minuteTime = total_minutes(hrs, mns);

  delay(500);

  //Serial.printf("%02d:%02d:%02d:%02d:%02d:%s:%02d:%02d\n", hrs, mns, scs, minuteTime, sch_index, stateNames[state], sch_str, sch_end);

  Serial.printf("%02d:%02d:%02d %s\n", hrs, mns, scs, stateNames[state]);

  switch (state) {
    case NOSTATE:
      // Sets time in minutes for start of school, end of school, and start of clean up.
      day_str = total_minutes(schedules[sch_str][HOURS], schedules[sch_str][MINUTES]);
      day_end = total_minutes(schedules[sch_end][HOURS], schedules[sch_end][MINUTES]);
      day_cln = day_end - CLEANING_TIME;

      // Determines which block of the day based on a given time and schedule
      for (sch_index = sch_str; sch_index <= sch_end; sch_index += 2) {
        if (minuteTime <= total_minutes(schedules[sch_index][HOURS], schedules[sch_index][MINUTES])) break;
      }
      sch_index -= 2;

      // Checks to see if we are outside of school time on in last period (special case)
      if (minuteTime < day_str)
        state = BEFORESCHOOL;
      else if (minuteTime > day_end)
        state = AFTERSCHOOL;
      else if (sch_index == (sch_end - 1))  //USE LESS THAN INSTEAD?
        state = LAST_PERIOD;
      else
        state = PERIOD_RESET;
      break;
    // Based on block calculate start, end, warning and next period times in minutes
    case PERIOD_RESET:
      period_str = total_minutes(schedules[sch_index + 0][HOURS], schedules[sch_index + 0][MINUTES]);
      period_end = total_minutes(schedules[sch_index + 1][HOURS], schedules[sch_index + 1][MINUTES]);
      period_wrn = period_end - WARNING_TIME;
      period_nxt = total_minutes(schedules[sch_index + 2][HOURS], schedules[sch_index + 2][MINUTES]);
      state = INCLASS;
      turn_on(GRN_LIGHT);  //Assume green as default. This will get corrected below.
      break;
    case INCLASS:
      if (minuteTime >= period_wrn) {
        state = WARNING;
        turn_on(YEL_LIGHT);
      }
      break;
    case WARNING:
      if (minuteTime >= period_end) {
        state = PASSING;
        turn_on(RED_LIGHT);
      }
      break;
    case PASSING:
      if (minuteTime >= period_nxt) {  // Move to next block and reset all block values
        state = PERIOD_RESET;
        sch_index += 2;
        if (sch_index == (sch_end - 1)) {  // Last period is a special case, since it has no next period time value. //USE LESS THAN INSTEAD?
          state = LAST_PERIOD;
          turn_on(GRN_LIGHT);
        }
      }
      break;
    case LAST_PERIOD:
      if (minuteTime >= day_cln) {
        state = CLEANUP;
        turn_on(YEL_LIGHT);
      }
      break;
    case CLEANUP:
      if (minuteTime >= day_end) {
        state = AFTERSCHOOL;
        turn_on(NOT_LIGHT);
      }
      break;
    case AFTERSCHOOL:
      if (minuteTime == MIDNIGHT) {
        state = BEFORESCHOOL;
      }
      break;
    case BEFORESCHOOL:
      if (minuteTime >= day_str) {
        state = PERIOD_RESET;
        sch_index = sch_str;
      }
      break;
  }/**/
}
