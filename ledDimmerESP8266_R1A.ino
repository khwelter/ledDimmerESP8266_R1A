/*
  ESP8266 as wireles dimmer for 12/24 V PWM dimmable LED stripes

  The dimmer registers itself with the wireless network ssid and the given password.
  When the dimmer can't connect it goes into configuration mode, ie. it creates it's own wireless network and serves a small 
  web page which allows to enter:
  - ssid
  - password
  - ip address (empty for DHCP)
  - 5 MQTT topics for on/off, red, green, blue and white
  The dimmer is controlled via MQTT.
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <EEPROM.h>

//
// functions from the EEPROM section
//
extern  void  stringToEEPROM( int, char *) ;
extern  void  stringFromEEPROM( int, char *, int) ;
extern  void  saveValues() ;
extern  void  saveValuesBrightness() ;
extern  void  loadValues() ;

//
//  Parameters for own WiFi network
//
const char *ssidOwn    = "ESPap" ;
const char *passwdOwn  = "changeme" ;

//
//  Parameters of MQTT server to connect to
//
const char* mqtt_server = "192.168.6.163";
int pinRed    = 4 ;
int pinGreen  = 14 ;
int pinBlue   = 12 ;
int pinWW     = 15 ;
int pinCW     = 0;
int movementSensor  = 5 ;

int valRed    = 250 ;
int valGreen  = 250 ;
int valBlue   = 500 ;
int valWW     = 500 ;
int valCW     = 500 ;
int valRedMov = 750 ;
int valGreenMov = 0 ;
int valBlueMov  = 500 ;
int valWWMov  = 500 ;
int valCWMov  = 500 ;
int addrRed             =   0 ;
int addrGreen           =   4 ;
int addrBlue            =   8 ;
int addrWW              =  12 ;
int addrCW              =  16 ;
int addrClientSSID      =  20 ;
int addrClientPasswd    =  52 ;
int addrMqttIPAddr      =  68 ;
int addrMqttUser        =  84;
int addrMqttPasswd      = 100 ;
int addrTopicMain       = 116 ;
int addrTopicOnOff      = 132 ;
int addrTopicRed        = 148 ;
int addrTopicGreen      = 164 ;
int addrTopicBlue       = 180 ;
int addrTopicWarmWhite  = 196 ;
int addrTopicColdWhite  = 212 ;

WiFiClient        espClient;
PubSubClient      clientMQTT(espClient);

//
//  Parameters for own HTPP server
//
ESP8266WebServer  serverHTTP(80);
char  clientSSID[32] ;
char  clientPasswd[16] ;
char  mqttIPAddr[16] ;
char  mqttUser[16] ;
char  mqttPasswd[16] ;
char  topicMain[16] ;
char  topicOnOff[16] ;
char  topicRed[16] ;
char  topicGreen[16] ;
char  topicBlue[16] ;
char  topicWarmWhite[16] ;
char  topicColdWhite[16] ;

long lastMsg = 0;
char msg[50];
int value = 0;
int movement ;
int lastMovement ;
bool wifiClient = false ;

bool setup_wifi() {
  int tryCounter ;
  bool  finalStatus = false ;
  delay( 10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print( "Connecting to ");
  Serial.println( clientSSID);

  WiFi.begin( clientSSID, clientPasswd);

  tryCounter  = 0 ;
  while (WiFi.status() != WL_CONNECTED && tryCounter < 20) {
    delay(500);
    Serial.print(".");
    tryCounter++ ;
  }
  Serial.print(", result ... : ");

  randomSeed( micros());
  if ( WiFi.status() == WL_CONNECTED ) {
    wifiClient  = true ;
    finalStatus = true ;
    Serial.println( "WiFi connected");
    Serial.println( "  IP address ..... : ");
    Serial.println( WiFi.localIP());
  } else {
    WiFi.disconnect() ;
    Serial.println( "WiFi *NOT* connected");
    Serial.println( "  will setup own wifi network for configuration only");
    WiFi.mode( WIFI_STA);
    WiFi.softAP( ssidOwn, passwdOwn);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print( "  AP IP address: ");
    Serial.println( myIP);
  }
}

String  getForm() {
  String  form ;
  form  = "<html>\
  <head>\
    <title>WiFi Dimmer Setup</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>LED Dimmer Configuration</h1><br>" ;
  form  +=  "<fieldset><legend>Configuration</legend><form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/configure/\"><table>";
  form  +=  "<tr><td>Netzwerkname</td><td><input type=\"text\" name=\"clientSSID\" value=\"" ;
  form  +=  clientSSID ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Passwort</td><td><input type=\"password\" name=\"clientPasswd\" autocomplete=\"off\" readonly onfocus=\"this.removeAttribute('readonly');\" value=\"" ;
  form  +=  clientPasswd ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>IP MQTT Server</td><td><input type=\"text\" name=\"mqttIPAddr\" value=\"" ;
  form  +=  mqttIPAddr ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>MQTT User</td><td><input type=\"text\" name=\"mqttUser\" value=\"" ;
  form  +=  mqttUser ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>MQTT Passwort</td><td><input type=\"password\" name=\"mqttPasswd\" autocomplete=\"off\" readonly onfocus=\"this.removeAttribute('readonly');\"  value=\"" ;
  form  +=  mqttPasswd ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Main Topic</td><td><input type=\"text\" name=\"topicMain\" value=\"" ;
  form  +=  topicMain ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic On/Off</td><td><input type=\"text\" name=\"topicOnOff\" value=\"" ;
  form  +=  topicOnOff ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic RED</td><td><input type=\"text\" name=\"topicRed\" value=\"" ;
  form  +=  topicRed ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic GREEN</td><td><input type=\"text\" name=\"topicGreen\" value=\"" ;
  form  +=  topicGreen ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic BLUE</td><td><input type=\"text\" name=\"topicBlue\" value=\"" ;
  form  +=  topicBlue ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic WARM WHITE</td><td><input type=\"text\" name=\"topicWarmWhite\" value=\"" ;
  form  +=  topicWarmWhite ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Topic COLD WHITE</td><td><input type=\"text\" name=\"topicColdWhite\" value=\"" ;
  form  +=  topicColdWhite ;
  form  +=  "\"></td></tr>" ;
  form  +=  "</table>" ;
  form  +=  "<input type=\"submit\" value=\"Save parameters\" name=\"saveData\">" ;
  form  +=  "</form></fieldset>" ;
  form  +=  "<fieldset><legend>Brightness values</legend><form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/configure/\"><table>";
  form  +=  "<tr><td>Red</td><td><input type=\"text\" name=\"valueRed\" value=\"" ;
  form  +=  valRed ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Green</td><td><input type=\"text\" name=\"valueGreen\" value=\"" ;
  form  +=  valGreen ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Blue</td><td><input type=\"text\" name=\"valueBlue\" value=\"" ;
  form  +=  valBlue ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Warm White</td><td><input type=\"text\" name=\"valueWW\" value=\"" ;
  form  +=  valWW ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Cold White</td><td><input type=\"text\" name=\"valueCW\" value=\"" ;
  form  +=  valCW ;
  form  +=  "\"></td></tr>" ;
  form  +=  "<tr><td>Save data ...</td><td><input type=\"text\" name=\"Save\" value=\"0\"></td></tr>" ;
  form  +=  "</table>" ;
  form  +=  "<input type=\"submit\" value=\"Set brightness\" name=\"setBright\">" ;
  form  +=  "<input type=\"submit\" value=\"Set &amp; Save brightness\" name=\"setNSaveBright\">" ;
  form  +=  "</form></fieldset>" ;
  form  +=  "</body>" ;
  form  +=  "</html>" ;
  return form ;
}

void handleRoot() {
  Serial.println( "handleRoot()") ;
  serverHTTP.send( 200, "text/html", getForm());
}

void  printValues() {
  Serial.println( "printValues():") ;
  Serial.println( clientSSID) ;
  Serial.println( clientPasswd) ;
  Serial.println( mqttIPAddr) ;
  Serial.println( mqttUser) ;
  Serial.println( mqttPasswd) ;
  Serial.println( topicMain) ;
  Serial.println( topicOnOff) ;
  Serial.println( topicRed) ;
  Serial.println( topicGreen) ;
  Serial.println( topicBlue) ;
  Serial.println( topicWarmWhite) ;
  Serial.println( topicColdWhite) ;
  Serial.println() ;
}

void handlePlain() {
  Serial.println( "handlePlain()") ;
  if (serverHTTP.method() != HTTP_POST) {
    serverHTTP.send(405, "text/plain", "Method Not Allowed");
  } else {
    serverHTTP.send(200, "text/plain", "POST body was:\n" + serverHTTP.arg( "plain"));
  }
}

void handleForm() {
  bool saveData ;
  char  data[64] ;
  
  saveData  = false ;
  Serial.println( "handleForm()") ;
  if (serverHTTP.method() != HTTP_POST) {
    serverHTTP.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < serverHTTP.args(); i++) {
      message += " " + serverHTTP.argName(i) + ": " + serverHTTP.arg(i) + "\n";
      if ( serverHTTP.argName(i) == "clientSSID") {
        serverHTTP.arg(i).toCharArray( clientSSID, 32) ;
        clientSSID[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "clientPasswd") {
        serverHTTP.arg(i).toCharArray( clientPasswd, 16) ;
        clientPasswd[ serverHTTP.arg(i).length()] = '\0' ;     
      } else if ( serverHTTP.argName(i) == "mqttIPAddr") {
        serverHTTP.arg(i).toCharArray( mqttIPAddr, 16) ;
        mqttIPAddr[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "mqttUser") {
        serverHTTP.arg(i).toCharArray( mqttUser, 16) ;
        mqttUser[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "mqttPasswd") {
        serverHTTP.arg(i).toCharArray( mqttPasswd, 16) ;
        mqttPasswd[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicMain") {
        serverHTTP.arg(i).toCharArray( topicMain, 16) ;
        topicMain[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicOnOff") {
        serverHTTP.arg(i).toCharArray( topicOnOff, 16) ;
        topicOnOff[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicRed") {
        serverHTTP.arg(i).toCharArray( topicRed, 16) ;
        topicRed[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicGreen") {
        serverHTTP.arg(i).toCharArray( topicGreen, 16) ;
        topicGreen[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicBlue") {
        serverHTTP.arg(i).toCharArray( topicBlue, 16) ;
        topicBlue[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicWarmWhite") {
        serverHTTP.arg(i).toCharArray( topicWarmWhite, 16) ;
        topicWarmWhite[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "topicColdWhite") {
        serverHTTP.arg(i).toCharArray( topicColdWhite, 16) ;
        topicColdWhite[ serverHTTP.arg(i).length()] = '\0' ;
      } else if ( serverHTTP.argName(i) == "valueRed") {
        serverHTTP.arg(i).toCharArray( data, 16) ;
        valRed = atoi( data) ;
      } else if ( serverHTTP.argName(i) == "valueGreen") {
        serverHTTP.arg(i).toCharArray( data, 16) ;
        valGreen = atoi( data) ;
      } else if ( serverHTTP.argName(i) == "valueBlue") {
        serverHTTP.arg(i).toCharArray( data, 16) ;
        valBlue = atoi( data) ;
      } else if ( serverHTTP.argName(i) == "valueWW") {
        serverHTTP.arg(i).toCharArray( data, 16) ;
        valWW = atoi( data) ;
      } else if ( serverHTTP.argName(i) == "valueCW") {
        serverHTTP.arg(i).toCharArray( data, 16) ;
        valCW = atoi( data) ;
      } else if ( serverHTTP.argName(i) == "saveData") {
          saveData  = true ;
      } else if ( serverHTTP.argName(i) == "setBright") {
        analogWrite( pinRed, valRed);
        analogWrite( pinGreen, valGreen);
        analogWrite( pinBlue, valBlue);
        analogWrite( pinWW, valWW);
        if ( pinCW != 0) {
          analogWrite( pinCW, valCW);
        }
      } else if ( serverHTTP.argName(i) == "setNSaveBright") {
        analogWrite( pinRed, valRed);
        analogWrite( pinGreen, valGreen);
        analogWrite( pinBlue, valBlue);
        analogWrite( pinWW, valWW);
        if ( pinCW != 0) {
          analogWrite( pinCW, valCW);
        }
        saveValuesBrightness() ;
      }
    }
//    printValues() ;
    if ( saveData) {
      Serial.println( "+---------------------------------------------");
      Serial.print( "Saving values from EEPROM ") ;
      saveValues() ;
      Serial.println() ;
    }
    serverHTTP.send(200, "text/html", getForm());
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += serverHTTP.uri();
  message += "\nMethod: ";
  message += ( serverHTTP.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += serverHTTP.args();
  message += "\n";
  for ( uint8_t i = 0; i < serverHTTP.args(); i++) {
    message += " " + serverHTTP.argName(i) + ": " + serverHTTP.arg(i) + "\n";
  }
  serverHTTP.send( 404, "text/plain", message);
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print( "Message arrived [");
  Serial.print( topic);
  Serial.print( "] ");
  payload[length] = '\0' ;
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ( strncmp( topic, topicOnOff, 6) == 0) {
    if ( strcmp( ( char *) payload, "0") == 0) {
      valRed    = 0;
      valGreen  = 0;
      valBlue   = 0;
      valWW     = 0;
      valCW     = 0;
    } else if ( strcmp( (char *) payload, "1") == 0) {
      loadValues() ;
    } else if ( strcmp( (char *) payload, "2") == 0) {
      saveValues() ;
    }
  } else if ( strncmp( topic, topicRed, strlen( topicRed)) == 0) {
    valRed = atoi( (char *) payload) ;
  } else if ( strncmp( topic, topicGreen, strlen( topicGreen)) == 0) {
    valGreen = atoi( (char *) payload) ;
  } else if ( strncmp( topic, topicBlue, strlen( topicBlue)) == 0) {
    valBlue = atoi( (char *) payload) ;
  } else if ( strncmp( topic, topicWarmWhite, strlen( topicWarmWhite)) == 0) {
    valWW = atoi( (char *) payload) ;
  } else if ( strncmp( topic, topicColdWhite, strlen( topicColdWhite)) == 0) {
    valCW = atoi( (char *) payload) ;
   }
  analogWrite( pinRed, valRed);
  analogWrite( pinGreen, valGreen);
  analogWrite( pinBlue, valBlue);
  analogWrite( pinWW, valWW);
//  analogWrite( pinCW, valCW);
//  printValues() ;
}

void reconnect() {
  // Loop until we're reconnected
  while ( !clientMQTT.connected()) {
    Serial.print( "Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String( random( 0xffff), HEX);
    // Attempt to connect
    Serial.println( mqttUser) ;
    Serial.println( mqttPasswd) ;
    strcpy( mqttUser, "khw") ;
    strcpy( mqttPasswd, "psion0") ;
    if ( clientMQTT.connect( clientId.c_str(), mqttUser, mqttPasswd)) {
      Serial.println( "connected");
      // Once connected, publish an announcement...
      clientMQTT.publish( "outTopic", "hello world");
      // ... and resubscribe
      clientMQTT.subscribe( topicMain);
    } else {
      Serial.print( "failed, rc=");
      Serial.print( clientMQTT.state());
      Serial.println( " try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  ESP.eraseConfig() ;
  pinMode( BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode( pinRed, OUTPUT);
  pinMode( pinGreen, OUTPUT);
  pinMode( pinBlue, OUTPUT);
  pinMode( pinWW, OUTPUT);
//  pinMode( pinCW, OUTPUT);
  pinMode( movementSensor, INPUT); 

  attachInterrupt( digitalPinToInterrupt( movementSensor), doSensor, CHANGE) ; // encoder pin on interrupt 0 (pin 2)

  Serial.begin( 115200);
  Serial.println( "+---------------------------------------------");
  Serial.print( "Starting up ...") ;

  Serial.println( "+---------------------------------------------");
  Serial.print( "Loading values from EEPROM ") ;
  loadValues() ;
  Serial.println() ;
  
  analogWrite( pinRed, valRed);
  analogWrite( pinGreen, valGreen);
  analogWrite( pinBlue, valBlue);
  analogWrite( pinWW, valWW);
//  analogWrite( pinCW, valCW);

  setup_wifi();

  if ( wifiClient) {
    clientMQTT.setServer( mqtt_server, 1883);
    clientMQTT.setCallback( callbackMQTT);
  }

  //
  //  start HTTP server
  //
  serverHTTP.on("/", handleRoot);
  serverHTTP.on("/configure/", handleForm);
  serverHTTP.onNotFound(handleNotFound);
  serverHTTP.begin();
  
  Serial.println("HTTP server started");
}

void loop() {

  if ( wifiClient) {
    if ( ! clientMQTT.connected()) {
      reconnect();
    }
    clientMQTT.loop();
  }

  serverHTTP.handleClient() ;
  
  long now = millis();
  if ( now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    if ( value > 1) {
      value = 0 ;
    }
    snprintf (msg, 75, "%ld", value);
    if ( wifiClient) {
      Serial.print("Publish message: ");
      Serial.println(msg);
      clientMQTT.publish("2/3/20", msg);
    }
  }
  if ( movement != lastMovement) {
    Serial.print( "movement sensor signal ...") ;  // no more debouncing until loop() hits again
    Serial.println( movement) ;
  }
  lastMovement  = movement ;
}

//
//  Interrupt on movementSensor changing state
//
ICACHE_RAM_ATTR void doSensor() {
  movement  = digitalRead( movementSensor) ;
  if ( movement) {
    loadValues() ;
    analogWrite( pinRed, valRedMov);
    analogWrite( pinGreen, valGreenMov);
    analogWrite( pinBlue, valBlueMov);
    analogWrite( pinWW, valWWMov);
  } else {
    analogWrite( pinRed, valRed);
    analogWrite( pinGreen, valGreen);
    analogWrite( pinBlue, valBlue);
    analogWrite( pinWW, valWW);
  }
}

/**
 * EEPROM Section
 */
void  stringToEEPROM( int offs, char *s) {
  int il0 ;
  Serial.print( ".") ;
  il0 = 0 ;
  while ( *s && il0++ < 16) {
    EEPROM.write( offs++, ( byte) *s++) ;
  }
  EEPROM.write( offs++, ( byte) *s++) ;
}

void  stringFromEEPROM( int offs, char *s, int max) {
  char c ;
  int i ;
  Serial.print( ".") ;
  c = EEPROM.read( offs++) ;
  i = 1 ;
  while ( c && i < max) {
    *s++ = c ;
    c = EEPROM.read( offs++) ;
    i++ ;
  }
  *s++ = c ;
}

void  saveValues() {
  EEPROM.begin( 256) ;
  Serial.print( ".") ;
  EEPROM.put( addrRed, valRed);
  Serial.print( ".") ;
  EEPROM.put( addrGreen, valGreen);
  Serial.print( ".") ;
  EEPROM.put( addrBlue, valBlue);
  Serial.print( ".") ;
  EEPROM.put( addrWW, valWW);
  Serial.print( ".") ;
  EEPROM.put( addrCW, valCW);
  stringToEEPROM( addrClientSSID, clientSSID) ;
  stringToEEPROM( addrClientPasswd, clientPasswd) ;
  stringToEEPROM( addrMqttIPAddr, mqttIPAddr) ;
  stringToEEPROM( addrMqttUser, mqttUser) ;
  stringToEEPROM( addrMqttPasswd, mqttPasswd) ;
  stringToEEPROM( addrTopicMain, topicMain) ;
  stringToEEPROM( addrTopicOnOff, topicOnOff) ;
  stringToEEPROM( addrTopicRed, topicRed) ;
  stringToEEPROM( addrTopicGreen, topicGreen) ;
  stringToEEPROM( addrTopicBlue, topicBlue) ;
  stringToEEPROM( addrTopicWarmWhite, topicWarmWhite) ;
  stringToEEPROM( addrTopicColdWhite, topicColdWhite) ;
  EEPROM.commit() ;
  EEPROM.end() ;
}

void  saveValuesBrightness() {
  EEPROM.begin( 256) ;
  Serial.print( ".") ;
  EEPROM.put( addrRed, valRed);
  Serial.print( ".") ;
  EEPROM.put( addrGreen, valGreen);
  Serial.print( ".") ;
  EEPROM.put( addrBlue, valBlue);
  Serial.print( ".") ;
  EEPROM.put( addrWW, valWW);
  Serial.print( ".") ;
  EEPROM.put( addrCW, valCW);
  EEPROM.commit() ;
  EEPROM.end() ;
}

void  loadValues() {
  EEPROM.begin( 256) ;
  Serial.print( ".") ;
  EEPROM.get( addrRed, valRed);
  Serial.print( ".") ;
  EEPROM.get( addrGreen, valGreen);
  Serial.print( ".") ;
  EEPROM.get( addrBlue, valBlue);
  Serial.print( ".") ;
  EEPROM.get( addrWW, valWW);
  Serial.print( ".") ;
  EEPROM.get( addrCW, valCW);
  stringFromEEPROM( addrClientSSID, clientSSID, 32) ;
  stringFromEEPROM( addrClientPasswd, clientPasswd, 16) ;
  stringFromEEPROM( addrMqttIPAddr, mqttIPAddr, 16) ;
  stringFromEEPROM( addrMqttUser, mqttUser, 16) ;
  stringFromEEPROM( addrMqttPasswd, mqttPasswd, 16) ;
  stringFromEEPROM( addrTopicMain, topicMain, 16) ;
  stringFromEEPROM( addrTopicOnOff, topicOnOff, 16) ;
  stringFromEEPROM( addrTopicRed, topicRed, 16) ;
  stringFromEEPROM( addrTopicGreen, topicGreen, 16) ;
  stringFromEEPROM( addrTopicBlue, topicBlue, 16) ;
  stringFromEEPROM( addrTopicWarmWhite, topicWarmWhite, 16) ;
  stringFromEEPROM( addrTopicColdWhite, topicColdWhite, 16) ;
  EEPROM.end() ;
}
