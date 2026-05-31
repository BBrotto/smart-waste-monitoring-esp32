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

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


const char* SSID     = "Wokwi-GUEST"; 
const char* PASSWORD = "";


const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT   = "lixeira_L01_mackenzie";
const char* TOPIC_NIVEL   = "lixeira/L01/nivel";
const char* TOPIC_CMD     = "lixeira/L01/cmd";


const int TRIG_PIN  = 5;
const int ECHO_PIN  = 18;
const int BUZZ_PIN  = 23;


const float DIST_VAZIA_CM  = 30.0;
const float DIST_CHEIA_CM  =  2.0;
const int   LIMIAR_ALERTA  = 80;
const int   INTERVALO_MS   = 10000;


WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long ultimaLeitura = 0;
bool buzzerAtivo = false;


float lerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracao == 0) return -1.0;

  return duracao / 58.0;
}

int calcularNivel(float distancia) {
  if (distancia < 0) return -1;
  float nivel = ((DIST_VAZIA_CM - distancia) / (DIST_VAZIA_CM - DIST_CHEIA_CM)) * 100.0;
  if (nivel < 0)   nivel = 0;
  if (nivel > 100) nivel = 100;
  return (int)nivel;
}

void controlarBuzzer(bool ligar) {
  if (ligar && !buzzerAtivo) {
    ledcAttach(BUZZ_PIN, 2000, 8);
    ledcWrite(BUZZ_PIN, 128);
    buzzerAtivo = true;
    Serial.println("[BUZZER] ALERTA SONORO ATIVADO");
  } else if (!ligar && buzzerAtivo) {
    ledcWrite(BUZZ_PIN, 0);
    ledcDetach(BUZZ_PIN);
    buzzerAtivo = false;
    Serial.println("[BUZZER] alerta sonoro desativado");
  }
}

void publicarMQTT(float distancia, int nivel, bool alerta) {
  StaticJsonDocument<200> doc;
  doc["lixeira_id"]   = "L01";
  doc["nivel_pct"]    = nivel;
  doc["distancia_cm"] = distancia;
  doc["alerta"]       = alerta;

  char payload[200];
  serializeJson(doc, payload);

  bool ok = mqttClient.publish(TOPIC_NIVEL, payload, true); // retain=true
  Serial.printf("[MQTT] Publicado em %s: %s  [%s]\n",
                TOPIC_NIVEL, payload, ok ? "OK" : "FALHOU");
}

void callbackMQTT(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)message[i];

  Serial.printf("[MQTT] Recebido em %s: %s\n", topic, msg.c_str());

  if (msg == "beep_on")  controlarBuzzer(true);
  if (msg == "beep_off") controlarBuzzer(false);
}


void conectarWiFi() {
  Serial.printf("\n[WiFi] Conectando a %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] FALHOU - verificar credenciais");
  }
}


void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.printf("[MQTT] Conectando ao broker %s...", MQTT_BROKER);
    if (mqttClient.connect(MQTT_CLIENT)) {
      Serial.println(" conectado!");
      mqttClient.subscribe(TOPIC_CMD);
      Serial.printf("[MQTT] Inscrito no tópico: %s\n", TOPIC_CMD);
    } else {
      Serial.printf(" falhou (rc=%d). Tentando em 5s...\n", mqttClient.state());
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Sistema de Monitoramento de Lixeiras - Mackenzie ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(BUZZ_PIN, LOW);

  conectarWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callbackMQTT);
  conectarMQTT();

  Serial.println("[Sistema] Inicializado. Aguardando leituras...\n");
}


void loop() {

  if (!mqttClient.connected()) conectarMQTT();
  mqttClient.loop();

  unsigned long agora = millis();
  if (agora - ultimaLeitura >= INTERVALO_MS) {
    ultimaLeitura = agora;

    float distancia = lerDistancia();
    Serial.printf("[HC-SR04] Distância medida: %.1f cm\n", distancia);

    int nivel = calcularNivel(distancia);
    Serial.printf("[Sistema] Nível de preenchimento: %d%%\n", nivel);

    bool alerta = (nivel >= LIMIAR_ALERTA);
    controlarBuzzer(alerta);

    if (distancia > 0) {
      publicarMQTT(distancia, nivel, alerta);
    } else {
      Serial.println("[Sistema] Leitura inválida - não publicado");
    }

    Serial.println("─────────────────────────────────────");
  }
  Serial.begin(115200);
  Serial.println("Hello, I'm in a terminal!");
  Serial.println();
}
