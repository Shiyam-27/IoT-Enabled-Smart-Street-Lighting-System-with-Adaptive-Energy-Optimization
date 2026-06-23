// ============================================================
// Smart Street Lighting System — with Blynk Integration
// ============================================================

#define BLYNK_TEMPLATE_ID   "Your_Template_ID"       // Get from Blynk console
#define BLYNK_TEMPLATE_NAME "Your_Template_Name"      // Get from Blynk console
#define BLYNK_AUTH_TOKEN    "Your_Auth_Token"         // Get from Blynk device settings

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Your_WiFi_Name";       // Replace with your WiFi name
char pass[] = "Your_WiFi_Password";   // Replace with your WiFi password

#define LDR_PIN       32
#define PIR_PIN       27

#define RELAY1_PIN    23
#define RELAY2_PIN    25

#define LDR_THRESHOLD     2800
#define HOLD_TIME         1000   // 1 second

#define RELAY_ON          HIGH
#define RELAY_OFF         LOW

unsigned long lastMotionTime = 0;
bool motionDetectedBefore = false;

bool manualMode = false;     // V5: false = Automatic, true = Manual
bool manualLedA = false;     // V6: manual control LED Set A
bool manualLedB = false;     // V7: manual control LED Set B

BlynkTimer timer;

// ---------- Blynk Write Handlers ----------

BLYNK_WRITE(V5)
{
  manualMode = param.asInt();   // 0 = Automatic, 1 = Manual
  Serial.println(manualMode ? "Switched to MANUAL mode" : "Switched to AUTOMATIC mode");
}

BLYNK_WRITE(V6)
{
  manualLedA = param.asInt();
  Serial.print("V6 received: ");
  Serial.println(manualLedA);
}

BLYNK_WRITE(V7)
{
  manualLedB = param.asInt();
  Serial.print("V7 received: ");
  Serial.println(manualLedB);
}

void setup()
{
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Serial.println("Smart Street Lighting Started");
}

void loop()
{
  Blynk.run();
  timer.run();

  int ldrValue = analogRead(LDR_PIN);
  bool motion = digitalRead(PIR_PIN);

  bool isNight = (ldrValue > LDR_THRESHOLD);

  Serial.print("LDR: ");
  Serial.print(ldrValue);

  Serial.print(" | Motion: ");
  Serial.print(motion ? "YES" : "NO");

  // Send motion status to Blynk (V1)
  Blynk.virtualWrite(V1, motion ? "Motion" : "No Motion");
  Blynk.virtualWrite(V0, isNight ? "Night" : "Day");

  // ================= MANUAL MODE =================
  if (manualMode)
  {
    digitalWrite(RELAY1_PIN, manualLedA ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY2_PIN, manualLedB ? RELAY_ON : RELAY_OFF);

    Blynk.virtualWrite(V3, manualLedA ? 1 : 0);
    Blynk.virtualWrite(V4, manualLedB ? 1 : 0);

    int brightness = 0;
    if (manualLedA && manualLedB) brightness = 100;
    else if (manualLedA || manualLedB) brightness = 50;
    Blynk.virtualWrite(V2, brightness);

    Serial.print(" | MANUAL MODE | Set A: ");
    Serial.print(manualLedA ? "ON" : "OFF");
    Serial.print(" | Set B: ");
    Serial.println(manualLedB ? "ON" : "OFF");

    delay(200);
    return;
  }

  // ================= AUTOMATIC MODE =================

  // ================= DAY MODE =================
  if (!isNight)
  {
    digitalWrite(RELAY1_PIN, RELAY_OFF);
    digitalWrite(RELAY2_PIN, RELAY_OFF);

    motionDetectedBefore = false;

    Serial.println(" | DAY MODE");

    // Blynk updates
    Blynk.virtualWrite(V2, 0);     // Brightness 0%
    Blynk.virtualWrite(V3, 0);     // LED Set A OFF
    Blynk.virtualWrite(V4, 0);     // LED Set B OFF

    delay(200);
    return;
  }

  // ================= NIGHT MODE =================

  // Relay1 always ON at night
  digitalWrite(RELAY1_PIN, RELAY_ON);
  Blynk.virtualWrite(V3, 1);   // LED Set A ON

  if (motion)
  {
    lastMotionTime = millis();
    motionDetectedBefore = true;

    digitalWrite(RELAY2_PIN, RELAY_ON);
    Blynk.virtualWrite(V4, 1);      // LED Set B ON
    Blynk.virtualWrite(V2, 100);    // Brightness 100%

    Serial.println(" | NIGHT + MOTION");
  }
  else
  {
    if (motionDetectedBefore &&
        (millis() - lastMotionTime < HOLD_TIME))
    {
      digitalWrite(RELAY2_PIN, RELAY_ON);
      Blynk.virtualWrite(V4, 1);
      Blynk.virtualWrite(V2, 100);

      Serial.println(" | HOLD TIMER ACTIVE");
    }
    else
    {
      digitalWrite(RELAY2_PIN, RELAY_OFF);
      Blynk.virtualWrite(V4, 0);
      Blynk.virtualWrite(V2, 50);     // Brightness 50% (Set A only)

      Serial.println(" | NIGHT + NO MOTION");
    }
  }

  delay(200);
}
