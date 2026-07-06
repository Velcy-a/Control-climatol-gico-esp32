# 🌬️ Sistema IoT de Calidad del Aire con ESP-NOW

## 📝 Descripción
Sistema de monitoreo ambiental distribuido que evalúa Temperatura, Humedad, niveles de CO2 y Compuestos Orgánicos Volátiles (COV). Utiliza una red de microcontroladores ESP32, un semáforo visual para alertas locales, y envía promedios a una base de datos en la nube (Supabase). Además, cuenta con un dashboard local en Flask para monitoreo y control manual.

## 🧰 Hardware y Sensores
* **3x ESP32** (1 Gateway, 2 Nodos)
* **Nodo 1:** Sensores BMP280 (Presión/Temp) y ENS160 (COV).
* **Nodo 2:** Sensor SCD4x (CO2/Temp/Hum).
* **Gateway:** 3x LEDs (Rojo, Amarillo, Verde) para el semáforo físico.

## 🚥 Alerta Visual (Semáforo de Calidad del Aire)
El Gateway promedia los datos recibidos por los nodos y evalúa la calidad del aire para encender el LED correspondiente:

* **Calidad Buena (Verde):** Valores normales de CO2 y COV.
  ![LED Verde](Led%20Verde.jpg)

* **Calidad Regular (Amarillo):** Alerta preventiva. CO2 entre 700-1000 ppm o COV entre 250-500 ppb.
  ![LED Amarillo](Led%20Amarillo.jpg)

* **Calidad Crítica (Rojo):** Preemergencia. CO2 > 1000 ppm o COV > 500 ppb.
  ![LED Rojo](Led%20Rojo.jpg)

## 💻 Monitor Serial y Respaldo en la Nube
Una vez calculados los promedios, el sistema emite los datos por serial para el dashboard local y realiza una petición HTTP POST a **Supabase** (retornando Código HTTP 201 en caso de éxito).

![Vista Serial Monitor](Vista%20Serial%20Monitor.jpg)

## 🏗️ Estructura del Código
* `GateWay.ino`: Maestro. Recibe y promedia datos, controla los LEDs, emite el faro de canal WiFi dinámico y sube las métricas a la BD.
* `Nodo1.ino`: Recopila Presión, Temp/Hum y COV, sintoniza el canal del Gateway y transmite por ESP-NOW.
* `Nodo2.ino`: Recopila CO2 y Temp/Hum, sintoniza el canal y transmite por ESP-NOW.
* `APP.py`: Servidor web Flask que lee el monitor serial de fondo y ofrece botones web para forzar el encendido/apagado de los LEDs.

## ⚙️ Requisitos y Configuración Rápida

**1. Librerías Arduino IDE:**
* `WiFiManager`, `ArduinoJson` (v7+), `Adafruit BMP280`, `Sensirion I2C SCD4x`, `ScioSense_ENS160`.

**2. Base de Datos (Supabase):**
Asegúrate de crear una tabla llamada `mediciones` con las siguientes columnas: `promedio_temperatura` (float), `promedio_humedad` (float), `co2` (int), `cov` (int).

**3. Despliegue Local (Flask):**
Instala las dependencias de Python:
pip install flask pyserial

## 📁 Estructura de Archivos del Proyecto
Para que el sistema funcione correctamente (especialmente el servidor web), asegúrate de organizar los archivos de la siguiente manera en tu directorio de trabajo:

TuProyecto/
├── app.py
└── templates/
    └── index.html