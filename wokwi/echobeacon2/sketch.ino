#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ====== Configurações WiFi ======
const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

// ====== Configurações MQTT ======
const char* BROKER_MQTT = "broker.hivemq.com";
const int BROKER_PORT = 1883;
const char* ID_MQTT = "esp32_beacon2";  // ID único para Beacon 2
const char* TOPIC_SUBSCRIBE = "fiap/iot/echobeacon/comando";

// ====== Identificação da Moto (BEACON 2) ======
const String PLACA_ESPERADA = "CDE5678";
const String MODELO_ESPERADO = "MOTTU_POP";
const String CHASSI_ESPERADO = "9BWZZZ377VT004252";

// ====== Pinos ======
const int LED_PIN = 2;
const int BUZZER_PIN = 4;
const int BUTTON_PIN = 15;

// ====== Controle ======
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool localizadorAtivo = false;

String placaAtual = "";
String modeloAtual = "";

// ====== Objetos WiFi e MQTT ======
WiFiClient espClient;
PubSubClient MQTT(espClient);

// ====== Funções ======
void initWiFi() {
  Serial.print("🔴 BEACON 2 - Conectando ao Wi-Fi ");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado!");
  Serial.print("📡 IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqttCallback);
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("🔌 Conectando ao broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println(" conectado!");
      MQTT.subscribe(TOPIC_SUBSCRIBE);
      Serial.print("📬 Inscrito no tópico: ");
      Serial.println(TOPIC_SUBSCRIBE);
    } else {
      Serial.print(" ❌ falha. Código: ");
      Serial.print(MQTT.state());
      Serial.println(" Tentando novamente em 2s");
      delay(2000);
    }
  }
}

void checkWiFIAndMQTT() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
  if (!MQTT.connected()) reconnectMQTT();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n📩 Mensagem MQTT recebida!");
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("❌ Erro no JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char* comando = doc["comando"];
  String placaRecebida = doc["moto"]["placa"] | "";
  String modeloRecebido = doc["moto"]["modelo"] | "";
  String chassiRecebido = doc["moto"]["chassi"] | "";

  Serial.print("📦 Comando: ");
  Serial.println(comando);
  Serial.print("🏍️  Placa recebida: ");
  Serial.println(placaRecebida);

  // ✅ FILTRO: Só processa se for a placa deste beacon
  if (placaRecebida != PLACA_ESPERADA) {
    Serial.println("⚠️  Esta mensagem é para outra moto. Ignorando.");
    return;
  }

  Serial.println("✅ Mensagem para ESTE beacon!");
  placaAtual = placaRecebida;
  modeloAtual = modeloRecebido;

  if (String(comando) == "ativar") {
    ativarLocalizador();
  }
}

void ativarLocalizador() {
  if (!localizadorAtivo) {
    localizadorAtivo = true;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 500);

    Serial.println("\n🚨🚨🚨 LOCALIZADOR ATIVADO 🚨🚨🚨");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.print("🔴 BEACON 2");
    Serial.print(" | Placa: ");
    Serial.println(placaAtual);
    Serial.print("📍 Modelo: ");
    Serial.println(modeloAtual);
    Serial.println("🔊 Buzzer: LIGADO");
    Serial.println("💡 LED: LIGADO");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  }
}

void desativarLocalizador() {
  if (localizadorAtivo) {
    localizadorAtivo = false;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);

    Serial.println("\n✅ LOCALIZADOR DESATIVADO");
    Serial.println("🔇 Buzzer: Desligado");
    Serial.println("💡 LED: Desligado\n");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║   🔴 ECHOBEACON 2 - INICIANDO   ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.print("🏍️  Placa: ");
  Serial.println(PLACA_ESPERADA);
  Serial.print("📋 Modelo: ");
  Serial.println(MODELO_ESPERADO);
  Serial.print("🔢 Chassi: ");
  Serial.println(CHASSI_ESPERADO);
  Serial.println();

  initWiFi();
  initMQTT();
  
  Serial.println("✅ Sistema EchoBeacon 2 pronto!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  checkWiFIAndMQTT();
  MQTT.loop();

  // Lógica do botão físico para desligar
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW && localizadorAtivo) {
        desativarLocalizador();
      }
    }
  }

  lastButtonState = reading;

  // Controle do buzzer intermitente
  if (localizadorAtivo) {
    static unsigned long previousMillis = 0;
    static bool buzzerOn = false;
    unsigned long currentMillis = millis();

    if (buzzerOn && currentMillis - previousMillis >= 300) {
      noTone(BUZZER_PIN);
      buzzerOn = false;
      previousMillis = currentMillis;
    } else if (!buzzerOn && currentMillis - previousMillis >= 700) {
      tone(BUZZER_PIN, 400);  
      buzzerOn = true;
      previousMillis = currentMillis;
    }
  }

  delay(10);
}