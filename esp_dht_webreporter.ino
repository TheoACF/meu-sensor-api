#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// ================== CONFIGURAÇÃO DO SERVIDOR DE DESTINO ==================
#define TARGET_REPLIT 1
#define TARGET_VERCEL 2

// ▼▼▼ APENAS MUDE ESTA LINHA PARA ESCOLHER O SERVIDOR ▼▼▼
#define TARGET_SERVER TARGET_VERCEL // Escolha entre TARGET_VERCEL ou TARGET_REPLIT

#if TARGET_SERVER == TARGET_VERCEL
  const char* SERVER_HOST = "seu-projeto.vercel.app"; // <-- COLOQUE SUA URL DA VERCEL AQUI
  const uint16_t SERVER_PORT = 443;
  const char* SERVER_PATH = "/api/sensors/readings"; // Caminho da sua API na Vercel
#elif TARGET_SERVER == TARGET_REPLIT
  const char* SERVER_HOST = "temp-umidade-mon-TheobaldoCordei.replit.app";
  const uint16_t SERVER_PORT = 443;
  const char* SERVER_PATH = "/api/sensors/readings";
#endif
// ========================================================================


// ================== CONFIG RÁPIDA ==================
#define ENABLE_FALLBACK_LOCAL 1      // 1: tenta local se o servidor principal falhar; 0: desliga fallback
const char* SSID = "LIVE TIM_E0A0_2G";
const char* PASS = "a66tphbcmm";

// ================== IP FIXO (opcional) ==================
IPAddress local_IP(192, 168, 1, 50);
IPAddress gateway (192, 168, 1, 1);
IPAddress subnet  (255, 255, 255, 0);
IPAddress dns     (8, 8, 8, 8);

// ================== SENSOR (DHT22) ==================
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================== CHAVE DE SIMULAÇÃO ==================
#define SIM_SWITCH_PIN 3   // RX
bool simMode = false;

// ================== SERVIDOR LOCAL (FALLBACK) ==================
const char* HOST_LOCAL   = "192.168.1.100";
const uint16_t PORT_LOCAL  = 5000;
const char* PATH_LOCAL   = "/api/sensors/readings";

// ================== GERAL ==================
const char* SENSOR_ID = "ESP01_SALA";
const unsigned long SEND_INTERVAL_MS = 15000UL;
const uint8_t  POST_RETRIES = 2;
const uint16_t HTTP_TIMEOUT_MS = 5000;

unsigned long lastSend = 0;
float lastT = NAN, lastH = NAN;

// ================== PROTÓTIPOS ==================
void detectSimMode();
void connectWiFi(bool first=false);
bool ensureWiFi();
bool getReading(float &t, float &h, bool &errorFlag);
bool postJSON(float t, float h, bool errorFlag);
bool postJSON_https(float t, float h, bool errorFlag);
bool postJSON_http (float t, float h, bool errorFlag);
float frand(float a, float b);
void printNetDiag();
bool getHealth();
bool postJSON_bootKick();

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(200);

  detectSimMode();

  Serial.println(F("\n===== ESP-01 → API (POST JSON) ====="));
  Serial.printf("Modo: %s\n", simMode ? "SIMULACAO" : "SENSOR REAL");
  Serial.printf("Servidor Alvo: %s\n", SERVER_HOST);


  if (!simMode) {
    dht.begin();
    delay(2000);
  }

  WiFi.mode(WIFI_STA);
  //WiFi.config(local_IP, gateway, subnet, dns); // Descomente se quiser IP fixo
  connectWiFi(true);

  printNetDiag();
  getHealth();
  postJSON_bootKick();

  WiFi.setSleep(true);
  randomSeed(ESP.getChipId());
  lastSend = millis() - SEND_INTERVAL_MS;
}

// ================== LOOP ==================
void loop() {
  if (!ensureWiFi()) return;

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    float t = 0.0, h = 0.0;
    bool errorFlag = false;

    if (!getReading(t, h, errorFlag)) {
      Serial.println(F("⏭️ Sem dados para enviar (ainda sem leitura válida)."));
      return;
    }

    Serial.printf("📦 Enviando -> T=%.1f °C  H=%.1f %%  error=%s\n",
                  t, h, errorFlag ? "true" : "false");

    if (postJSON(t, h, errorFlag)) {
      Serial.println(F("✅ Enviado com sucesso!"));
    } else {
      Serial.println(F("❌ Falha no envio após retries/fallback."));
    }
  }
}

// ================== AUX / WIFI ==================
void detectSimMode() {
  pinMode(SIM_SWITCH_PIN, INPUT_PULLUP);
  delay(5);
  simMode = (digitalRead(SIM_SWITCH_PIN) == LOW);
}

void connectWiFi(bool first) {
  Serial.printf("Conectando ao WiFi: %s\n", SSID);
  WiFi.begin(SSID, PASS);

  uint16_t tries = first ? 120 : 60;
  while (WiFi.status() != WL_CONNECTED && tries--) { delay(250); Serial.print("."); }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) { Serial.print(F("✅ IP: ")); Serial.println(WiFi.localIP()); }
  else { Serial.print(F("❌ WiFi falhou. Status=")); Serial.println(WiFi.status()); }
}

bool ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.println(F("⚠️ WiFi desconectado, reconectando..."));
  connectWiFi(false);
  return (WiFi.status() == WL_CONNECTED);
}

// ================== LEITURA ==================
bool getReading(float &t, float &h, bool &errorFlag) {
  if (simMode) {
    t = frand(24.0, 30.0);
    h = frand(45.0, 70.0);
    errorFlag = true;
    Serial.printf("🧪 Simulando: T=%.1f, H=%.1f\n", t, h);
    return true;
  }

  Serial.print(F("📥 Lendo DHT... "));
  t = dht.readTemperature();
  h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println(F("erro na leitura!"));
    errorFlag = true;
    if (isnan(lastT) || isnan(lastH)) return false;
    t = lastT; h = lastH;
    return true;
  }

  Serial.printf("ok (T=%.1f, H=%.1f)\n", t, h);
  lastT = t; lastH = h;
  errorFlag = false;
  return true;
}

// ================== POST ==================
bool postJSON(float t, float h, bool errorFlag) {
  for (uint8_t i = 0; i <= POST_RETRIES; i++) {
    if (postJSON_https(t, h, errorFlag)) return true;
    Serial.printf("↻ Retry HTTPS (%u/%u)\n", i + 1, POST_RETRIES);
    delay(300 + 200 * i);
  }

#if ENABLE_FALLBACK_LOCAL
  Serial.println(F("↘ Fallback: servidor LOCAL (HTTP)"));
  for (uint8_t i = 0; i <= POST_RETRIES; i++) {
    if (postJSON_http(t, h, errorFlag)) return true;
    Serial.printf("↻ Retry HTTP local (%u/%u)\n", i + 1, POST_RETRIES);
    delay(300 + 200 * i);
  }
#endif

  return false;
}

bool postJSON_https(float t, float h, bool errorFlag) {
  String url = String("https://") + SERVER_HOST + SERVER_PATH;
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Use com cuidado, desabilita validação de certificado
  HTTPClient http;

  if (!http.begin(client, url)) { Serial.println(F("❌ begin() HTTPS falhou")); return false; }
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setReuse(false);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP01-DHT/1.0");

  String payload;
  payload.reserve(160);
  payload  = "{\"temperature\":";
  payload += String(t, 1);
  payload += ",\"humidity\":";
  payload += String(h, 1);
  payload += ",\"sensorId\":\"";
  payload += SENSOR_ID;
  payload += "\",\"rssi\":";
  payload += String(WiFi.RSSI());
  payload += ",\"uptime\":";
  payload += String(millis()/1000);
  if (errorFlag) payload += ",\"error\":true";
  payload += "}";

  Serial.print(F("📡 POST ")); Serial.println(url);
  Serial.print(F("🧾 ")); Serial.println(payload);

  int code = http.POST(payload);
  if (code <= 0) { Serial.print(F("HTTPS erro: ")); Serial.println(http.errorToString(code)); http.end(); return false; }
  Serial.print(F("↪ HTTP ")); Serial.println(code);
  if (code >= 200 && code < 300) {
    String resp = http.getString();
    if (resp.length()) { Serial.print(F("↪ Body: ")); Serial.println(resp); }
  }
  http.end();
  return (code >= 200 && code < 300);
}

bool postJSON_http(float t, float h, bool errorFlag) {
  String url = String("http://") + HOST_LOCAL + ":" + String(PORT_LOCAL) + PATH_LOCAL;
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, url)) { Serial.println(F("❌ begin() HTTP falhou")); return false; }
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");

  String payload;
  payload.reserve(160);
  payload  = "{\"temperature\":";
  payload += String(t, 1);
  payload += ",\"humidity\":";
  payload += String(h, 1);
  payload += ",\"sensorId\":\"";
  payload += SENSOR_ID;
  payload += "\",\"rssi\":";
  payload += String(WiFi.RSSI());
  payload += ",\"uptime\":";
  payload += String(millis()/1000);
  if (errorFlag) payload += ",\"error\":true";
  payload += "}";

  int code = http.POST(payload);
  if (code <= 0) { Serial.print(F("HTTP erro: ")); Serial.println(http.errorToString(code)); http.end(); return false; }
  Serial.print(F("↪ HTTP ")); Serial.println(code);
  http.end();
  return (code >= 200 && code < 300);
}

float frand(float a, float b) {
  return a + (b - a) * (float)random(0, 10000) / 10000.0f;
}

// =============== FUNÇÕES DE DIAGNÓSTICO ===============
void printNetDiag() {
  Serial.println(F("=== NET DIAG ==="));
  Serial.print(F("IP: "));   Serial.println(WiFi.localIP());
  Serial.print(F("GW: "));   Serial.println(WiFi.gatewayIP());
  Serial.print(F("DNS: "));  Serial.println(WiFi.dnsIP());
  Serial.print(F("RSSI: ")); Serial.print(WiFi.RSSI()); Serial.println(" dBm");

  IPAddress resolved;
  if (WiFi.hostByName(SERVER_HOST, resolved)) {
    Serial.print(F("Host IP: ")); Serial.println(resolved);
  } else {
    Serial.println(F("Host DNS resolve: FALHA"));
  }
  Serial.println(F("================"));
}

bool getHealth() {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String("https://") + SERVER_HOST + "/health";
  if (!http.begin(client, url)) { Serial.println(F("❌ begin() /health")); return false; }
  http.setTimeout(HTTP_TIMEOUT_MS);
  int code = http.GET();
  Serial.print(F("GET /health → HTTP ")); Serial.println(code);
  if (code > 0) { String body = http.getString(); Serial.println(body); }
  http.end();
  return (code >= 200 && code < 300);
}

bool postJSON_bootKick() {
  float t = 25.1, h = 60.3;
  bool errorFlag = false;
  Serial.println(F("🚀 BOOT-KICK: enviando payload inicial..."));
  bool ok = postJSON(t, h, errorFlag);
  Serial.println(ok ? F("BOOT-KICK OK") : F("BOOT-KICK FALHOU"));
  return ok;
}
