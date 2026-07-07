#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // Requiere versión 7+

#define LED_VERDE    12  
#define LED_AMARILLO 14
#define LED_ROJO     27

// ==========================================
// CONFIGURACIÓN DE CREDENCIALES SUPABASE
// ==========================================
const char* supabase_url = "user";
const char* supabase_key = "password";

// Estructura de datos recibida por ESP-NOW desde los nodos
typedef struct struct_data {
  int id_nodo; float temp1; float hum1; float pres;
  uint16_t co2; uint16_t tvoc; float temp2; float hum2;
} struct_data;

struct_data myData;

// === ESTRUCTURA PARA ENVIAR EL CANAL A LOS NODOS ===
typedef struct struct_canal {
  int canal_actual;
} struct_canal;

struct_canal datosCanal;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Dirección de difusión global

// Variables globales para el cálculo de promedios y almacenamiento
float t1 = 0.0, h1 = 0.0, t2 = 0.0, h2 = 0.0;
float p_t = 0.0, p_h = 0.0; 
uint16_t valor_cov = 0; 
uint16_t valor_co2 = 0; 
bool n1 = false, n2 = false;

// Banderas y temporizadores de control operacional
volatile bool enviar_datos = false; 
unsigned long ultimoBroadcastCanal = 0;
const unsigned long intervaloBroadcast = 200; // Emitir canal cada 200ms para enlace inmediato de los nodos

// Proceso secundario automático basado en la calidad del aire de Santiago
void evaluarCalidadAire(uint16_t co2, uint16_t cov) {
  // Rojo: Mala calidad del aire (Preemergencia / Crítico)
  if (co2 > 1000 || cov > 500) {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARILLO, LOW);
    digitalWrite(LED_ROJO, HIGH);
  }
  // Amarillo: Regular (Alerta preventiva)
  else if ((co2 > 700 && co2 <= 1000) || (cov > 250 && cov <= 500)) {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARILLO, HIGH);
    digitalWrite(LED_ROJO, LOW);
  }
  // Verde: Buena calidad del aire
  else {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_AMARILLO, LOW);
    digitalWrite(LED_ROJO, LOW);
  }
}

// Función Callback de ESP-NOW para Recepción de Datos
void OnDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  
  if (myData.id_nodo == 1) {
    t1 = myData.temp1;
    h1 = myData.hum1;
    valor_cov = myData.tvoc; 
    n1 = true;
  } 
  else if (myData.id_nodo == 2) {
    t2 = myData.temp1;
    h2 = myData.hum1;
    valor_co2 = myData.co2;  
    n2 = true;
  }

  // Cuando ambos nodos entregan datos, se calculan los promedios
  if (n1 && n2) {
    p_t = (t1 + t2) / 2.0;
    p_h = (h1 + h2) / 2.0;
    enviar_datos = true; // Activamos la bandera para enviar en el loop
    
    // Reseteamos las banderas de los nodos para la siguiente ráfaga
    n1 = false;
    n2 = false;
  }
}

// Callback corregido de Transmisión de Canal (Compatible con Core 3.x)
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  // Monitoreo silencioso de la ráfaga de broadcast
}

// Función encargada de realizar la petición HTTP POST a Supabase
void enviarASupabase() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Ignora la verificación estricta del certificado SSL

    HTTPClient http;
    http.begin(client, supabase_url);
    
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabase_key);
    String authHeader = String("Bearer ") + supabase_key;
    http.addHeader("Authorization", authHeader);

    JsonDocument doc;
    doc["promedio_temperatura"] = p_t;
    doc["promedio_humedad"] = p_h;
    doc["co2"] = valor_co2;
    doc["cov"] = valor_cov;
    
    String jsonStr;
    serializeJson(doc, jsonStr);

    int httpResponseCode = http.POST(jsonStr);
    
    if (httpResponseCode > 0) {
      Serial.printf("[Supabase] Datos enviados con éxito. Código HTTP: %d\n", httpResponseCode);
    } else {
      Serial.printf("[Supabase] Error en el envío: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
  } else {
    Serial.println("[WiFi] Error: Desconectado de la red. No se puede subir a Supabase.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  // Inicialización preventiva en apagado
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);

  WiFiManager wm;

  if(!wm.autoConnect("ESP32_Config_Red")) {
    Serial.println("Error de conexión, reiniciando...");
    ESP.restart();
  }

  int canalRouter = WiFi.channel();
  datosCanal.canal_actual = canalRouter; // Guardamos el canal asignado por tu Router
  
  Serial.println("=========================================");
  Serial.printf(">>> EL ROUTER ASIGNÓ EL CANAL WIFI: %d <<<\n", canalRouter);
  Serial.println("=========================================");

  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);
    Serial.println("ESP-NOW Inicializado correctamente.");
    
    // Registrar el enlace de Broadcast para emitir el canal a los nodos
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = canalRouter; 
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error al registrar el par de Broadcast.");
    }
  } else {
    Serial.println("Error inicializando ESP-NOW.");
  }
}

void loop() {
  // --- LÓGICA DE FARO AUTOMÁTICO DE CANAL (200ms) ---
  if (millis() - ultimoBroadcastCanal >= intervaloBroadcast) {
    ultimoBroadcastCanal = millis();
    datosCanal.canal_actual = WiFi.channel(); // Verifica si el router reubicó el canal dinámicamente
    
    // Envía la estructura de sincronización
    esp_now_send(broadcastAddress, (uint8_t *) &datosCanal, sizeof(datosCanal));
  }

  // Manejo de despliegue local y envío a la base de datos
  if (enviar_datos) {
    // Imprimimos la línea limpia solicitada para Python / Consola
    Serial.printf("[PROMEDIO] T:%.1f°C | H:%.1f%% | CO2:%dppm | COV:%dppb\n", 
                  p_t, p_h, valor_co2, valor_cov);
    
    // Proceso automático secundario de alertas en los pines analógicos
    evaluarCalidadAire(valor_co2, valor_cov);

    enviarASupabase(); // Sube las métricas unificadas a Supabase
    enviar_datos = false; // Bajamos la bandera
  }

  // Control manual de LEDs mediante comandos seriales (Anulación web interactiva)
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); 
    if (cmd == "VERDE_ON")         digitalWrite(LED_VERDE, HIGH);
    else if (cmd == "VERDE_OFF")  digitalWrite(LED_VERDE, LOW);
    else if (cmd == "AMARILLO_ON")  digitalWrite(LED_AMARILLO, HIGH);
    else if (cmd == "AMARILLO_OFF") digitalWrite(LED_AMARILLO, LOW);
    else if (cmd == "ROJO_ON")     digitalWrite(LED_ROJO, HIGH);
    else if (cmd == "ROJO_OFF")    digitalWrite(LED_ROJO, LOW);
  }
  delay(20);
}
