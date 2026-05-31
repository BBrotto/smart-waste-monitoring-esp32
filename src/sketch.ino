/*
 * Sistema de Monitoramento Inteligente de Resíduos Urbanos
 * Universidade Presbiteriana Mackenzie - FCI
 * Autoras: Bianca Maciel Alaunes Brotto, Seyedehzahra Mousavi
 *
 * Hardware:
 *   - ESP32 DevKit v1
 *   - Sensor ultrassônico HC-SR04 (Trig: GPIO5, Echo: GPIO18)
 *   - Buzzer passivo (GPIO23, via resistor 100 Ω)
 *
 * Comunicação:
 *   - Wi-Fi + MQTT (broker HiveMQ público)
 *   - Tópicos publicados: lixeira/L01/nivel
 *   - Tópico subscrito:   lixeira/L01/cmd  (para comandos remotos)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* SSID          = "Wokwi-GUEST";
const char* PASSWORD      = "";
const char* MQTT_BROKER   = "broker.hivemq.com";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT   = "lixeira_L01_mackenzie";
const char* TOPIC_NIVEL   = "lixeira/L01/nivel";
const char* TOPIC_CMD     = "lixeira/L01/cmd";

const int   TRIG_PIN      = 5;
const int   ECHO_PIN      = 18;
const int   BUZZ_PIN      = 23;

const float DIST_VAZIA_CM = 30.0f;
const float DIST_CHEIA_CM = 2.0f;
const int   LIMIAR_ALERTA = 80;
const unsigned long INTERVALO_MS = 2000UL;

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long ultimaLeitura       = 0UL;
unsigned long ultimaTentativaMQTT = 0UL;
bool buzzerAtivo = false;

float lerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 60000UL);
  if (dur == 0) return 15.0f;
  return (float)dur / 58.0f;
}

int calcularNivel(float d) {
  if (d < 0.0f) return -1;
  float n = ((DIST_VAZIA_CM - d) / (DIST_VAZIA_CM - DIST_CHEIA_CM)) * 100.0f;
  if (n < 0.0f)   n = 0.0f;
  if (n > 100.0f) n = 100.0f;
  return (int)n;
}

void controlarBuzzer(bool ligar) {
  if (ligar == buzzerAtivo) return;
  if (ligar) {
    ledcSetup(0, 2000, 8);
    ledcAttachPin(BUZZ_PIN, 0);
    ledcWrite(0, 128);
    Serial.println("[BUZZER] ATIVADO");
  } else {
    ledcWrite(0, 0);
    ledcDetachPin(BUZZ_PIN);
    Serial.println("[BUZZER] desativado");
  }
  buzzerAtivo = ligar;
}

void callbackMQTT(char* topic, byte* msg, unsigned int len) {
  String s = "";
  for (unsigned int i = 0; i < len; i++) s += (char)msg[i];
  Serial.printf("[MQTT] Recebido: %s\n", s.c_str());
  if (s == "beep_on")  controlarBuzzer(true);
  if (s == "beep_off") controlarBuzzer(false);
}

void tentarConectarMQTT() {
  if (mqttClient.connected()) return;
  if (millis() - ultimaTentativaMQTT < 10000UL) return;
  ultimaTentativaMQTT = millis();
  Serial.print("[MQTT] Tentando conectar...");
  if (mqttClient.connect(MQTT_CLIENT)) {
    Serial.println(" OK!");
    mqttClient.subscribe(TOPIC_CMD);
  } else {
    Serial.printf(" falhou rc=%d\n", mqttClient.state());
  }
}

void publicarMQTT(float dist, int nivel, bool alerta) {
  if (!mqttClient.connected()) return;
  StaticJsonDocument<200> doc;
  doc["lixeira_id"]   = "L01";
  doc["nivel_pct"]    = nivel;
  doc["distancia_cm"] = dist;
  doc["alerta"]       = alerta;
  char payload[200];
  serializeJson(doc, payload);
  bool ok = mqttClient.publish(TOPIC_NIVEL, payload, true);
  Serial.printf("[MQTT] %s -> %s\n", payload, ok ? "OK" : "FALHOU");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Monitoramento de Lixeiras - Mackenzie ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(BUZZ_PIN, LOW);

  WiFi.begin(SSID, PASSWORD);
  Serial.print("[WiFi] Conectando");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000UL) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Sem conexao - continuando offline");
  }

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callbackMQTT);
  tentarConectarMQTT();

  Serial.println("[Sistema] Pronto!\n");
}

void loop() {
  tentarConectarMQTT();
  if (mqttClient.connected()) mqttClient.loop();

  unsigned long agora = millis();
  if (agora - ultimaLeitura >= INTERVALO_MS) {
    ultimaLeitura = agora;

    float dist  = lerDistancia();
    int   nivel = calcularNivel(dist);

    Serial.printf("[Sensor] dist=%.1fcm  nivel=%d%%  t=%lums\n", dist, nivel, agora);

    bool alerta = (nivel >= LIMIAR_ALERTA);
    controlarBuzzer(alerta);
    publicarMQTT(dist, nivel, alerta);
  }
}
