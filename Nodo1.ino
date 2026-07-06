#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> 
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SensirionI2CScd4x.h> 
#include <ScioSense_ENS160.h> 

// Dirección MAC física de tu Gateway
uint8_t macGateway[] = {0x1C, 0xC3, 0xAB, 0xD2, 0xF0, 0xD4};

// Estructura para el envío de métricas ambientales
typedef struct struct_data {
  int id_nodo; float temp1; float hum1; float pres;
  uint16_t co2; uint16_t tvoc; float temp2; float hum2;
} struct_data;

struct_data datos;
esp_now_peer_info_t peerInfo;

// === ESTRUCTURA PARA DETECTAR EL CANAL DEL GATEWAY ===
typedef struct struct_canal {
  int canal_actual;
} struct_canal;

struct_canal canalRecibido;
volatile bool canalEncontrado = false;
int canalWiFiReceptor = 1; // Se actualizará dinámicamente

Adafruit_BMP280 bmp;
SensirionI2CScd4x scd4x; 
ScioSense_ENS160 ens160(ENS160_I2CADDR_0); 

// Callback temporal exclusivo para capturar el canal enviado por el Gateway
void OnCanalRecv(const esp_now_recv_info *recvInfo, const uint8_t *incomingData, int len) {
  if (len == sizeof(canalRecibido)) {
    memcpy(&canalRecibido, incomingData, sizeof(canalRecibido));
    canalWiFiReceptor = canalRecibido.canal_actual;
    Serial.printf("\n[SINCRO] ¡Gateway detectado en el canal: %d!\n", canalWiFiReceptor);
    canalEncontrado = true;
  }
}

// Callback normal para verificar el estatus del envío de datos
void OnDataSent(const wifi_tx_info_t *txInfo, esp_now_send_status_t status) {
  Serial.print("Envío Nodo 1: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK 🚀" : "FALLIDO ❌");
}

// Proceso de barrido de radiofrecuencia (Canales 1 al 13)
void escanearCanalGateway() {
  Serial.println("[SINCRO] Buscando señal del Gateway...");
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW en etapa de escaneo.");
    return;
  }
  
  // Registramos el escucha temporal del canal
  esp_now_register_recv_cb(OnCanalRecv);

  while (!canalEncontrado) {
    for (int canal = 1; canal <= 13; canal++) {
      if (canalEncontrado) break;
      
      Serial.printf("[SINCRO] Escaneando frecuencia en Canal %d...\n", canal);
      
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
      
      // Espera de ventana de escucha (400ms por canal)
      delay(400); 
    }
    if (!canalEncontrado) {
      Serial.println("[SINCRO] Ciclo completo sin éxito. Reintentando barrido...");
    }
  }

  // Una vez hallado el canal, limpiamos el callback de recepción para liberar memoria
  esp_now_unregister_recv_cb();
  esp_now_deinit();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  // Forzar modo Estación para el control de canales de radio
  WiFi.mode(WIFI_STA);
  
  // 1. Ejecutar el proceso secundario automático de sintonización
  escanearCanalGateway();
  
  // 2. Aplicar el canal definitivo encontrado en el hardware del ESP32
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(canalWiFiReceptor, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // 3. Inicializar ESP-NOW de forma definitiva para transmisión
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error relanzando ESP-NOW para datos.");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  
  // 4. Registrar emparejamiento con la MAC fija usando el canal dinámico aprendido
  memcpy(peerInfo.peer_addr, macGateway, 6);
  peerInfo.channel = canalWiFiReceptor;   
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("[OPERACIÓN] Enlace con Gateway configurado con éxito.");
  }

  // 5. Inicialización estándar de tu bus de sensores I2C
  if (!bmp.begin(0x76)) { bmp.begin(0x77); }
  scd4x.begin(Wire);
  scd4x.startPeriodicMeasurement();
  ens160.begin();
  ens160.setMode(ENS160_OPMODE_STD);
}

void loop() {
  datos.id_nodo = 1;
  datos.temp1 = bmp.readTemperature();
  datos.pres = bmp.readPressure() / 100.0F;

  uint16_t co2_val = 0; float s_t = 0, s_h = 0;
  bool dataReady = false;
  scd4x.getDataReadyFlag(dataReady);
  if (dataReady) {
    scd4x.readMeasurement(co2_val, s_t, s_h);
    datos.hum1 = s_h;
  }
  
  ens160.measure(true);
  datos.tvoc = ens160.getTVOC(); // Captura exclusiva de COV
  datos.co2 = 0; // Nodo 1 no reporta CO2
  
  datos.temp2 = 0.0; datos.hum2 = 0.0; 

  // Envía los datos directamente en el canal sintonizado
  esp_now_send(macGateway, (uint8_t *) &datos, sizeof(datos));
  delay(10000); 
}