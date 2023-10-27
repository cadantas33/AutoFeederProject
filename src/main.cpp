#include <Arduino.h>
#include <.env/WifiCredentials.cpp>
#include <LittleFS.h>
#include <pitches.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

int LED = 2;
#define BUZZER_PIN D3
#define SERVO_PIN D4
// notes in the melody:
int melody[] = {
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
    4, 8, 8, 4, 4, 4, 4, 4};

void playSong()
{
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++)
  {
    Serial.println("playing here...");

    // to calculate the note duration, take one second divided by the note type.
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(BUZZER_PIN);
  }
}

void openDoor()
{
  Serial.println("Opening door...");
  digitalWrite(SERVO_PIN, HIGH);
  delay(2000);
  digitalWrite(SERVO_PIN, LOW);
}

enum QUIZ_RESULT
{
  WON,
  LOSE
};

void playMellody(QUIZ_RESULT type)
{
  if (type == QUIZ_RESULT::WON)
  {
    playSong();
  }
}

void quizWon()
{
  openDoor();
  playMellody(QUIZ_RESULT::WON);
}

void handleNotFound()
{
  digitalWrite(LED, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(LED, 0);
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(SERVO_PIN, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/quiz", []()
            { quizWon(); });

  server.on(UriBraces("/users/{}"), []()
            {
    String user = server.pathArg(0);
    server.send(200, "text/plain", "User: '" + user + "'"); });

  server.on(UriRegex("^\\/users\\/([0-9]+)\\/devices\\/([0-9]+)$"), []()
            {
    String user = server.pathArg(0);
    String device = server.pathArg(1);
    server.send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'"); });

  server.serveStatic("/", LittleFS, "/");

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  LittleFS.begin();
}

void loop(void)
{
  server.handleClient();
}