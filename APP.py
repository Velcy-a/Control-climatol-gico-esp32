import serial
import threading
import time
from flask import Flask, render_template, request, redirect, url_for

app = Flask(__name__)

SERIAL_PORT = "COM6"  
BAUD_RATE = 115200
ultimo_promedio = "Sincronizando bus local de datos..."

def lectura_serial():
    global ultimo_promedio
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            while True:
                if ser.in_waiting > 0:
                    linea = ser.readline().decode('utf-8', errors='ignore').strip()
                    if "[PROMEDIO]" in linea:
                        ultimo_promedio = linea
                        print(f"📡 Sistema Activo -> {linea}")
                time.sleep(0.01)
        except serial.SerialException:
            time.sleep(5)

@app.route('/')
def index():
    return render_template('index.html', promedio=ultimo_promedio)

@app.route('/enviar_comando', methods=['POST'])
def enviar_comando():
    comando = request.form.get('comando')
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            ser.write(f"{comando}\n".encode('utf-8'))
    except Exception:
        pass
    return redirect(url_for('index'))

if __name__ == '__main__':
    threading.Thread(target=lectura_serial, daemon=True).start()
    app.run(host='0.0.0.0', port=5000, debug=False)