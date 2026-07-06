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
  ![LED Verde](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/fotos/Led%20Verde.jpg)

* **Calidad Regular (Amarillo):** Alerta preventiva. CO2 entre 700-1000 ppm o COV entre 250-500 ppb.
  ![LED Amarillo](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/fotos/Led%20Amarillo.jpg)

* **Calidad Crítica (Rojo):** Preemergencia. CO2 > 1000 ppm o COV > 500 ppb.
  ![LED Rojo](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/fotos/Led%20Rojo.jpg)

## 💻 Monitor Serial y Respaldo en la Nube
Una vez calculados los promedios, el sistema emite los datos por serial para el dashboard local y realiza una petición HTTP POST a **Supabase** (retornando Código HTTP 201 en caso de éxito).

![Vista Serial Monitor](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/fotos/Vista%20Serial%20Monitor.jpg)
![Vista Supabase](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/foto%20supabase.jpg)
![Vista Página Flask](https://github.com/Velcy-a/Control-climatol-gico-esp32/blob/main/templates/Vista%20de%20P%C3%A1gina.png)

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
```text
TuProyecto/
├── APP.py
└── templates/
    └── index.html
```
    
## 🎮 Comandos Seriales de Control Manual

El panel web de Flask (APP.py) interactúa con el Gateway despachando instrucciones a través del puerto serial para forzar el estado de la alerta visual. Los comandos que interpreta el microcontrolador son:

* VERDE_ON / VERDE_OFF: Enciende o apaga de forma manual el LED Verde.

* AMARILLO_ON / AMARILLO_OFF: Enciende o apaga de forma manual el LED Amarillo.

* ROJO_ON / ROJO_OFF: Enciende o apaga de forma manual el LED Rojo.

## 📡 Mecanismo de Sintonización Inalámbrica

Debido a que el router doméstico puede alterar dinámicamente el canal de radiofrecuencia de la red, el sistema incluye un protocolo automático de emparejamiento:

* El Gateway evalúa su canal asignado y emite un faro de sincronización (Broadcast) cada 200ms.

* Al encenderse, tanto el Nodo 1 como el Nodo 2 efectúan un barrido cíclico por las frecuencias de los canales 1 al 13 en busca de este faro.

* Al capturar la señal, los nodos configuran su hardware en dicho canal de transmisión definitivo e inician el envío periódico de las métricas por ESP-NOW.

## Integrantes del Grupo
Jaime Fuentes, Joaquín Fuentes, Ricardo Toro.






## Trabajo para la asignatura Desarrollo de Software para Hardware (DCSH01)

Queremos agradecer a nuestro profesor, José Fernando Poblete Cabezas por acompañarnos a lo largo de la carrera.

  ![Foto Kirby](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTQjUreEyPZzDjWjUg0vDb7AbjTBvEvTiCWTXDOC_41bW6FbVyk7mmDYN8&s=10)
