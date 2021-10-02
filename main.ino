#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

int i = 0;
int statusCode;
String st;
String configNetworksOptions;
String content;

bool testWifi(void);
void launchWeb(void);
void setupAP(void);

ESP8266WebServer webServer(80);

/**
 *
 */
void appSetup(void) {
    Serial.println("[App] Setup...");
}

/**
 *
 */
void appLoop(void) {
    Serial.println("[App] Loop...");
}

/**
 *
 */
void setup(void) {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();

    Serial.println("Reading SSID and passphrase from EEPROM");
    EEPROM.begin(512);
    delay(10);
    String ssid = readDataAsString(0, 32);
    String passphrase = readDataAsString(32, 96);
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(passphrase);

    Serial.println("Perform WiFi connection with EEPROM");
    WiFi.begin(ssid.c_str(), passphrase.c_str());
    if (testWifi()) {
        Serial.println("Successfully connected.");
        appSetup();
        return;
    }

    Serial.println("Turning on the config HotSpot.");
    configWebServerRegisterRoutes();
    webServer.begin();
    setupAP();
    while ((WiFi.status() != WL_CONNECTED)) {
        delay(100);
        webServer.handleClient();
    }
}

/**
 *
 */
void loop() {
    if ((WiFi.status() == WL_CONNECTED)) {
        appLoop();
    }
}

/**
 *
 */
bool testWifi(void) {
    int c = 0;
    Serial.println("Waiting for WiFi to connect...");
    while (c < 20) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(500);
        Serial.print(".");
        c++;
    }
    Serial.println("");
    Serial.println("Connection timed out.");
    return false;
}

/**
 *
 */
void launchWeb(void) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
    }
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    configWebServerRegisterRoutes();
    webServer.begin();
    Serial.println("Server started");
}

/**
 *
 */
void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  configScanNetworks();
  delay(100);
  WiFi.softAP("ESP8266-WiFi", "");
  Serial.println("Initializing_Wifi_accesspoint");
  launchWeb();
  Serial.println("over");
}

void configWebServerRegisterRoutes() {
    webServer.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        String configHtmlContent = "<!DOCTYPE html><html lang=en><meta charset=UTF-8><title>Page Title</title><meta name=viewport content=\"width=device-width,initial-scale=1\"><style>*{background:#222;border:0;color:#fff;font:18px monospace}html{margin:0 auto;max-width:480px}input,input:focus,select,select:focus,button,button:focus,textarea,textarea:focus{outline:0;width:100%;background:#000;padding:8px;margin:0 0 16px 0;box-sizing:border-box;cursor:pointer}</style><body><h1>esp8266-wifi</h1><div id=page></div><script>let d=document;let p=p=>fetch(p).then(async resp=>d.getElementById('page').innerHTML=await resp.text());let e=(t,i,c)=>d.addEventListener(t,(e)=>{if(e.target.id==i){e.preventDefault();c(e);}},false);p('login');e('click','login',e=>{console.log('event',e);})</script></body></html>";
        webServer.send(200, "text/html", configHtmlContent);
    });
    webServer.on("/scan", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>go back";
        webServer.send(200, "text/html", content);
    });
    webServer.on("/connect", []() {
        int statusCode;
        bool validData = false
        String ssid = webServer.arg("ssid");
        String passphrase = webServer.arg("passphrase");
        String content;
        if (ssid.length() > 0 && passphrase.length() > 0) {
            dataErase(0, 96)
            dataSaveAsString(0, ssid);
            dataSaveAsString(32, passphrase);
            dataCommit();
            webServerResponse = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
            webServerStatusCode = 200;
            validData = true;
        } else {
            content = "{\"Error\":\"404 not found\"}";
            statusCode = 404;
        }
        webServer.sendHeader("Access-Control-Allow-Origin", "*");
        webServer.send(statusCode, "application/json", content);
        if (validData) {
            delay(500);
            ESP.reset();
        }
    });
}

/**
 * Scan networks and prepare the list for the login form.
 */
void configScanNetworks(void) {
    int countNetworks = WiFi.scanNetworks();
    Serial.println("Networks scan completed.");
    if (countNetworks == 0) {
        Serial.println("No WiFi Networks found");
        configNetworksOptions = "<option value=0>No networks found.</option>";
        configNetworksOptions += "<option value=0>No networks found.</option>";
    } else {
        Serial.print(countNetworks);
        Serial.println(" Networks found");
        configNetworksOptions = "<option value=0>Select a network</option>";
        for (int i = 0; i < countNetworks; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
            delay(10);
            st += "<option value=\"" + WiFi.SSID(i) + "\">";
            st += WiFi.SSID(i);
            st += " (";
            st += WiFi.RSSI(i);
            st += ")";
            st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
            st += "</option>";
        }
    }
}

/**
 * Read data from EEPROM.
 */
String dataReadAsString(int from, int to) {
    String data = "";
    for (int i = from; i < to; ++i) {
        data += char(EEPROM.read(i));
    }
    return data;
}

/**
 * Store data on EEPROM.
 */
void dataSaveAsString(int offset, String value) {
    for (int i = 0; i < value.length(); ++i) {
        EEPROM.write(offset + i, value[i]);
    }
}

/**
 * Erase EEPROM segment.
 */
void dataErase(int from, int to) {
    for (int i = from; i < to; ++i) {
        EEPROM.write(i, 0);
    }
}

/**
 * Commit data on EEPROM.
 */
void dataCommit(void) {
    EEPROM.commit();
}
