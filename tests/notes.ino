const char* ssid = "text";
const char* passphrase = "text";

"" + WiFi.localIP();
IPAddress ip = WiFi.softAPIP();
String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
