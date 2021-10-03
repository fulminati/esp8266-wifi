#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>

const String appTitle = "ESP8266 WiFi";
const String hostname = "esp8266.localhost.net";
const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;

String configErrorMessage;
String configNetworksOptions;

ESP8266WebServer webServer(80);

/**
 * Application bootstrap.
 */
void appSetup(void) {
    Serial.println("[App] Setup...");
}

/**
 * Application runtime loop.
 */
void appLoop(void) {
    //Serial.println("[App] Loop...");
}

/**
 * Application runtime loop.
 */
void appRoutes(void) {
    Serial.println("[App] Routes...");
}

/**
 * System bootstrap.
 */
void setup(void) {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();
    delay(200);
    WiFi.hostname(hostname);
    delay(300);

    Serial.println("Reading SSID and passphrase from EEPROM");
    EEPROM.begin(512);
    delay(10);
    String ssid = dataReadAsString(0, 32);
    String passphrase = dataReadAsString(32, 96);
    Serial.println("- SSID: " + ssid);
    Serial.println("- Passphrase: " + passphrase);

    Serial.println("Perform WiFi connection with EEPROM");
    WiFi.begin(ssid.c_str(), passphrase.c_str());
    WiFi.config(0U, 0U, 0U);
    if (testWifi()) {
        Serial.println("Successfully connected.");
        if (!MDNS.begin(hostname)) {
            Serial.println("Error setting up MDNS responder!");
        }
        defaultWebServerRegisterRoutes();
        webServer.begin();
        appSetup();
        return;
    }

    Serial.println("Turning on the config HotSpot.");
    configWebServerRegisterRoutes();
    webServer.begin();
    configSetupHotSpot();
    while ((WiFi.status() != WL_CONNECTED)) {
        delay(100);
        dnsServer.processNextRequest();
        webServer.handleClient();
    }
}

/**
 * System main loop.
 */
void loop(void) {
    if ((WiFi.status() == WL_CONNECTED)) {
        appLoop();
        delay(100);
        webServer.handleClient();
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
 * Setup the HotSpot to access on config area.
 */
void configSetupHotSpot(void) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    delay(100);
    dnsServer.start(DNS_PORT, "*", apIP);
    delay(100);
    configScanNetworks();
    delay(100);
    WiFi.softAP(appTitle + " " + ESP.getChipId(), "");
    launchWeb();
}

/**
 *
 */
void configWebServerRegisterRoutes(void) {
    webServer.on("/", []() {
        String configIndexHtml = "<!DOCTYPE html><html lang=en><meta charset=UTF-8><title>"+ appTitle +"</title><meta name=viewport content=\"width=device-width,initial-scale=1\"><link rel=\"shortcut icon\" type=image/x-icon href=data:image/x-icon;,><style>*{background:#222;border:0;color:#fff;font:18px monospace}html{margin:0 auto;max-width:480px}input,input:focus,select,select:focus,button,button:focus,textarea,textarea:focus{outline:0;width:100%;background:#000;padding:8px;margin:0 0 16px 0;box-sizing:border-box;cursor:pointer}#mask{position:fixed;display:none;width:100%;height:100%;text-align:center;padding-top:100px;top:0;left:0;right:0;bottom:0;background-color:rgba(0,0,0,0.8);z-index:2;cursor:pointer}h1{font-size:30px;text-align:center}</style><body><h1>"+ appTitle +"</h1><div id=page></div><div id=mask></div><script>let d=document;let g=i=>d.getElementById(i);let p=(p,f)=>fetch(p,{method:'POST',body:f?new FormData(g(f)):''}).then(async resp=>g('page').innerHTML=await resp.text());let e=(t,i,c)=>d.addEventListener(t,(e)=>{if(e.target.id==i){e.preventDefault();c(e.target);}},false);let o=a=>g('mask').style.display=a=='show'?'block':'none';let r=u=>{window.location.href=u};let t=(t,c)=>setTimeout(c,t);;p('config');e('click','connect',e=>{o('show');p('connect','config').then(()=>{o('hide');t(5000,()=>r('http://"+hostname+"/welcome'));});});e('change','network',e=>{if(e.value==-1){o('show');p('scan').then(()=>{o('hide')});}});e('click','test',e=>{o('show');setTimeout(()=>{o('hide')},3000)});</script></body></html>";
        webServer.send(200, "text/html", configIndexHtml);
    });
    webServer.on("/config", []() {
        webServer.send(200, "text/html", configFormHtml());
    });
    webServer.on("/scan", []() {
        configScanNetworks();
        webServer.send(200, "text/html", configFormHtml());
    });
    webServer.on("/connect", []() {
        int statusCode;
        bool validData = false;
        String ssid = webServer.arg("ssid");
        String passphrase = webServer.arg("passphrase");
        String content;
        if (ssid.length() > 0 && passphrase.length() > 0) {
            dataErase(0, 96);
            dataSaveAsString(0, ssid);
            dataSaveAsString(32, passphrase);
            dataCommit();
            content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
            statusCode = 200;
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
        configNetworksOptions = "<option value=0>No networks found</option>";
        configNetworksOptions += "<option value=-1>Scan for networks</option>";
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
            configNetworksOptions += "<option value=\"" + WiFi.SSID(i) + "\">";
            configNetworksOptions += WiFi.SSID(i);
            configNetworksOptions += " (";
            configNetworksOptions += WiFi.RSSI(i);
            configNetworksOptions += ")";
            configNetworksOptions += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
            configNetworksOptions += "</option>";
        }
    }
}

/**
 *
 */
String configFormHtml(void) {
    String configFormHtml = "<form id=config> SSID <select id=network name=ssid> "+ configNetworksOptions +" </select> Password <input type=password name=passphrase required> <button id=connect type=button>Connect</button></form>";
    return configFormHtml;
}

/**
 *
 */
void defaultWebServerRegisterRoutes(void) {
    webServer.on("/welcome", []() {
        String welcomeHtml = "<h1>Welcome</h1>";
        webServer.send(200, "text/html", welcomeHtml);
    });
    webServer.on("/reset", []() {
        dataErase(0, 96);
        dataCommit();
        ESP.reset();
        webServer.send(200, "text/html", "<h1>Reset ok!</h1>");
    });
    appRoutes();
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
