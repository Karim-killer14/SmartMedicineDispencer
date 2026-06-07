#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

/* ===================== WiFi ===================== */
const char* ssid     = "Bekhiet's wifi";
const char* password = "pfna4193";

/* ===================== CallMeBot WhatsApp ===================== */
String phoneNumber = "+201229871029";
String apiKey      = "7654593";

/* ===================== Settings ===================== */

/*
   1 = send WhatsApp message with the website link when ESP connects.
   0 = disable startup link message.
*/
#define SEND_STARTUP_LINK_WHATSAPP 1

/*
   1 = send WhatsApp message when medicine is confirmed taken.
   0 = only send missed alerts.
*/
#define SEND_TAKEN_CONFIRMATION_WHATSAPP 1

/* ===================== Pins ===================== */
#define LED_PIN LED_BUILTIN

/*
   ESP D6 / GPIO12 receives from STM32 PA9 TX
   ESP D5 / GPIO14 sends to STM32 PA10 RX
*/
#define STM_RX_PIN 12   // D6
#define STM_TX_PIN 14   // D5

SoftwareSerial stmSerial(STM_RX_PIN, STM_TX_PIN);
ESP8266WebServer server(80);

/* ===================== State ===================== */
String currentAlarmTime = "12:01";
bool alarmActive = false;

/* ===================== LED Helpers ===================== */

void LED_On()
{
  digitalWrite(LED_PIN, LOW);   // NodeMCU built-in LED is active LOW
}

void LED_Off()
{
  digitalWrite(LED_PIN, HIGH);
}

void blinkCode(int count, int onMs, int offMs)
{
  for (int i = 0; i < count; i++)
  {
    LED_On();
    delay(onMs);
    LED_Off();
    delay(offMs);
  }

  delay(250);
}

void blinkSuccess()
{
  blinkCode(2, 500, 200);
}

void blinkError()
{
  blinkCode(8, 80, 80);
}

/* ===================== URL Encode ===================== */

String urlEncode(String str)
{
  String encoded = "";

  for (int i = 0; i < str.length(); i++)
  {
    char c = str.charAt(i);

    if      (c == ' ')  encoded += "%20";
    else if (c == '!')  encoded += "%21";
    else if (c == '?')  encoded += "%3F";
    else if (c == '&')  encoded += "%26";
    else if (c == '=')  encoded += "%3D";
    else if (c == '+')  encoded += "%2B";
    else if (c == '#')  encoded += "%23";
    else if (c == ':')  encoded += "%3A";
    else if (c == '/')  encoded += "%2F";
    else if (c == '\n') encoded += "%0A";
    else encoded += c;
  }

  return encoded;
}

/* ===================== WhatsApp ===================== */

bool sendWhatsApp(String message)
{
  Serial.println();
  Serial.println("========== WhatsApp ==========");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[WA] WiFi not connected.");
    blinkError();
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://api.callmebot.com/whatsapp.php?phone="
               + phoneNumber
               + "&text="
               + urlEncode(message)
               + "&apikey="
               + apiKey;

  Serial.println("[WA] Sending request...");

  http.begin(client, url);
  int code = http.GET();

  Serial.print("[WA] HTTP code: ");
  Serial.println(code);

  String body = http.getString();
  Serial.print("[WA] Body: ");
  Serial.println(body);

  http.end();

  if (code == 200)
  {
    Serial.println("[WA] Message request accepted.");
    blinkSuccess();
    return true;
  }
  else
  {
    Serial.println("[WA] Failed.");
    blinkError();
    return false;
  }
}

/* ===================== Time Validation ===================== */

bool isValidTimeCommand(String command)
{
  command.trim();

  if (!command.startsWith("SET ")) return false;
  if (command.length() != 9) return false;
  if (command.charAt(6) != ':') return false;

  int h1 = command.charAt(4) - '0';
  int h2 = command.charAt(5) - '0';
  int m1 = command.charAt(7) - '0';
  int m2 = command.charAt(8) - '0';

  if (h1 < 0 || h1 > 9) return false;
  if (h2 < 0 || h2 > 9) return false;
  if (m1 < 0 || m1 > 9) return false;
  if (m2 < 0 || m2 > 9) return false;

  int hour = h1 * 10 + h2;
  int minute = m1 * 10 + m2;

  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;

  return true;
}

/* ===================== ESP -> STM32 ===================== */

void sendToSTM(String command)
{
  command.trim();

  if (!isValidTimeCommand(command))
  {
    Serial.print("[ESP] Invalid command: ");
    Serial.println(command);
    blinkError();
    return;
  }

  stmSerial.println(command);

  currentAlarmTime = command.substring(4);

  Serial.print("[ESP -> STM] ");
  Serial.println(command);

  blinkCode(3, 120, 120);
}

/* ===================== STM32 Message Handler ===================== */

void handleSTMMessage(String msg)
{
  msg.trim();

  if (msg.length() == 0) return;

  Serial.print("[STM -> ESP] ");
  Serial.println(msg);

  if (msg == "DISPENSE")
  {
    alarmActive = false;
    Serial.println("[EVENT] Medicine dispensing started.");
    blinkCode(2, 150, 150);
    LED_Off();
  }
  else if (msg == "BUZZER")
  {
    alarmActive = true;
    Serial.println("[EVENT] Buzzer active. Waiting before WhatsApp alert.");
    blinkCode(3, 150, 150);
    LED_On();
  }
  else if (msg == "MISSED")
  {
    alarmActive = true;
    Serial.println("[EVENT] Medicine missed. Sending WhatsApp alert.");
    blinkCode(4, 150, 150);
    LED_On();

    sendWhatsApp("Medicine Alert! Your medicine was NOT taken on time. Please take it now.");
  }
  else if (msg == "TAKEN")
  {
    alarmActive = false;
    Serial.println("[EVENT] Medicine confirmed taken.");
    blinkCode(1, 700, 200);
    LED_Off();

#if SEND_TAKEN_CONFIRMATION_WHATSAPP
    sendWhatsApp("Medicine confirmed taken. Thank you.");
#endif
  }
  else if (msg == "TIME_OK")
  {
    Serial.println("[STM] Time update accepted.");
    blinkSuccess();
  }
  else if (msg == "TIME_ERR")
  {
    Serial.println("[STM] Time update rejected.");
    blinkError();
  }
  else if (msg == "CMD_ERR")
  {
    Serial.println("[STM] Command error.");
    blinkError();
  }
  else if (msg == "STM_READY")
  {
    Serial.println("[STM] STM32 is ready.");
    blinkSuccess();
  }
  else
  {
    Serial.print("[ESP] Unknown STM message: ");
    Serial.println(msg);
    blinkError();
  }
}

/* ===================== Web Page ===================== */

String makePage()
{
  String html = "";

  html += "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin:25px;}";
  html += "button{font-size:20px;margin:8px;padding:12px;min-width:220px;}";
  html += "input{font-size:24px;padding:8px;}";
  html += ".box{border:1px solid #ccc;padding:20px;border-radius:10px;display:inline-block;}";
  html += "</style>";
  html += "</head><body>";

  html += "<div class='box'>";
  html += "<h2>Smart Medicine Dispenser</h2>";
  html += "<p>Current Alarm: <b>" + currentAlarmTime + "</b></p>";

  html += "<form action='/set' method='GET'>";
  html += "<input type='time' name='t' required>";
  html += "<br>";
  html += "<button type='submit'>Set Medicine Time</button>";
  html += "</form>";

  html += "<hr>";

  html += "<form action='/wa' method='GET'>";
  html += "<button type='submit'>Test WhatsApp</button>";
  html += "</form>";

  html += "</div>";
  html += "</body></html>";

  return html;
}

void handleRoot()
{
  server.send(200, "text/html", makePage());
}

void handleSet()
{
  if (!server.hasArg("t"))
  {
    server.send(400, "text/plain", "Missing time.");
    return;
  }

  String timeValue = server.arg("t");
  timeValue.trim();

  String command = "SET " + timeValue;

  if (!isValidTimeCommand(command))
  {
    server.send(400, "text/plain", "Invalid time.");
    blinkError();
    return;
  }

  sendToSTM(command);

  String response = "";
  response += "<html><body style='font-family:Arial;text-align:center;margin-top:40px;'>";
  response += "<h2>Medicine Time Updated</h2>";
  response += "<p>Sent to STM32: <b>" + command + "</b></p>";
  response += "<a href='/'>Back</a>";
  response += "</body></html>";

  server.send(200, "text/html", response);
}

void handleWhatsAppTest()
{
  bool ok = sendWhatsApp("Test message from Smart Medicine Dispenser.");

  if (ok)
  {
    server.send(200, "text/plain", "WhatsApp request sent. Check your phone.");
  }
  else
  {
    server.send(500, "text/plain", "WhatsApp failed. Check Serial Monitor.");
  }
}

/* ===================== Setup / Loop ===================== */

void setup()
{
  Serial.begin(9600);
  stmSerial.begin(9600);
  stmSerial.setTimeout(100);

  pinMode(LED_PIN, OUTPUT);
  LED_Off();

  blinkCode(5, 100, 100);

  Serial.println();
  Serial.println("====================================");
  Serial.println("ESP8266 Smart Medicine Final Code");
  Serial.println("With Startup Link + Taken/Missed WhatsApp");
  Serial.println("====================================");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[WiFi] Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    LED_On();
    delay(80);
    LED_Off();
    delay(420);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("[WiFi] Connected. IP: ");
  Serial.println(WiFi.localIP());

  blinkCode(2, 250, 250);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/wa", handleWhatsAppTest);
  server.begin();

  Serial.println("[WEB] Server started.");
  String link = "http://" + WiFi.localIP().toString();
  Serial.print("[WEB] Open from phone: ");
  Serial.println(link);

#if SEND_STARTUP_LINK_WHATSAPP
  sendWhatsApp("Smart Medicine Dispenser is online. Open this link to set medicine time: " + link);
#endif

  Serial.println("Manual Serial commands:");
  Serial.println("SET 12:01");
  Serial.println("WA_TEST");

  LED_Off();
}

void loop()
{
  server.handleClient();

  if (stmSerial.available())
  {
    String incoming = stmSerial.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() > 0)
    {
      handleSTMMessage(incoming);
    }
  }

  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "WA_TEST")
    {
      sendWhatsApp("Manual WhatsApp test from ESP8266.");
    }
    else if (isValidTimeCommand(cmd))
    {
      sendToSTM(cmd);
    }
    else
    {
      Serial.print("[PC] Unknown command: ");
      Serial.println(cmd);
    }
  }
}