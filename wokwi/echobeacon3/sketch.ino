#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

const char* BROKER_MQTT = "broker.hivemq.com";
const int BROKER_PORT = 1883;
const char* ID_MQTT = "esp32_beacon3";

const char* TOPIC_SUBSCRIBE = "fiap/iot/echobeacon/comando";
const char* TOPIC_PUBLISH = "fiap/iot/echobeacon/status";

const String NUMERO_IDENTIFICACAO_ESPERADO = "3";

const int LED_PIN = 2;
const int BUZZER_PIN = 4;
const int BUTTON_PIN = 15;

int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool localizadorAtivo = false;

String modeloAtual = "";
String numeroIdentificacaoAtual = "";

WiFiClient espClient;
PubSubClient MQTT(espClient);

// ============================
// 🟢 INICIALIZAÇÕES
// ============================
void initWiFi() {
  Serial.print("🔌 Conectando ao Wi-Fi ");
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
    Serial.print("🔁 Conectando ao broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println(" conectado!");
      MQTT.subscribe(TOPIC_SUBSCRIBE);
      Serial.print("📨 Inscrito no tópico: ");
      Serial.println(TOPIC_SUBSCRIBE);
    } else {
      Serial.print("Falha. Código: ");
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

// ============================
// 💬 CALLBACK MQTT
// ============================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n📩 Mensagem MQTT recebida!");

  // Construir string do payload
  String incoming;
  for (unsigned int i = 0; i < length; i++) incoming += (char)payload[i];

  Serial.print("Raw payload: ");
  Serial.println(incoming);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, incoming);

  if (error) {
    Serial.print("❌ Erro no JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // leitura segura dos campos
  const char* comando = doc["comando"] | "";
  const char* numero_identificacao = doc["numero_identificacao"] | "";
  // modelo dentro de moto pode estar ausente — lemos de forma segura
  const char* modeloRecebido = "";
  if (doc.containsKey("moto") && doc["moto"].is<JsonObject>() && doc["moto"]["modelo"].is<const char*>()) {
    modeloRecebido = doc["moto"]["modelo"].as<const char*>();
  }

  Serial.print("🧭 Comando: ");
  Serial.println(comando);

  // Agora aceitamos somente mensagens que contenham numero_identificacao igual ao esperado
  if (!numero_identificacao || strlen(numero_identificacao) == 0) {
    Serial.println("⚠️ Mensagem ignorada: faltando campo 'numero_identificacao'.");
    return;
  }

  String num = String(numero_identificacao);
  Serial.print("🔎 numero_identificacao recebido: ");
  Serial.println(num);

  if (num != NUMERO_IDENTIFICACAO_ESPERADO) {
    Serial.println("⚠️ numero_identificacao NÃO corresponde a este EchoBeacon. Ignorando.");
    return;
  }

  // Se autorizado:
  numeroIdentificacaoAtual = num;
  modeloAtual = String(modeloRecebido);

  if (String(comando) == "ativar") {
    ativarLocalizador();
  } else if (String(comando) == "desativar") {
    desativarLocalizador();
  } else {
    Serial.println("⚠️ Comando desconhecido recebido.");
  }
}

// ============================
// 🔊 FUNÇÕES DE LOCALIZAÇÃO
// ============================
void ativarLocalizador() {
  if (!localizadorAtivo) {
    localizadorAtivo = true;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 500);

    Serial.println("\n🔔 LOCALIZADOR ATIVADO");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.print("ECHOBEACON ID: ");
    Serial.println(numeroIdentificacaoAtual);
    Serial.print(" Modelo: ");
    Serial.println(modeloAtual);
    Serial.println(" Buzzer: LIGADO");
    Serial.println(" LED: LIGADO");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    enviarStatus("ativado");
  }
}

void desativarLocalizador() {
  if (localizadorAtivo) {
    localizadorAtivo = false;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);

    Serial.println("\n🛑 LOCALIZADOR DESATIVADO");
    Serial.println(" Buzzer: DESLIGADO");
    Serial.println(" LED: DESLIGADO\n");

    enviarStatus("desativado");
  }
}

void enviarStatus(const char* status) {
  StaticJsonDocument<128> doc;
  doc["status"] = status;
  doc["numero_identificacao"] = numeroIdentificacaoAtual;

  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUBLISH, buffer, n);
  Serial.print("📤 Status publicado: ");
  Serial.println(buffer);
}

// ============================
// ⚙️ SETUP E LOOP
// ============================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║     ECHOBEACON - INICIANDO        ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.print("Numero identificacao esperado: ");
  Serial.println(NUMERO_IDENTIFICACAO_ESPERADO);
  Serial.println();

  initWiFi();
  initMQTT();

  Serial.println("✅ Sistema EchoBeacon pronto!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  checkWiFIAndMQTT();
  MQTT.loop();

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

  // Padrão de som intermitente
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