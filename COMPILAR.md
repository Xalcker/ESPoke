# Guía de Compilación - ESPoke

Reproductor de karaoke para ESP32 con video compuesto.

## Requisitos Previos

### Hardware
- Placa ESP32 (cualquier variante)
- Cable USB para programación
- Lector de tarjetas SD
- Componentes para conexiones (ver README.md)

### Software
- Python 3.x
- PlatformIO Core (CLI)

---

## Instalación

### 1. Instalar Python

**Windows:**
Descargar de https://www.python.org/downloads/
(marcar "Add Python to PATH" durante instalación)

**Linux:**
```bash
sudo apt update
sudo apt install python3 python3-pip
```

**macOS:**
```bash
brew install python3
```

### 2. Instalar PlatformIO Core

```bash
pip install platformio
```

O usando el instalador oficial:
```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py | python3
```

### 3. Verificar instalación

```bash
pio --version
```

Debería mostrar algo como `PlatformIO Core, version x.x.x`

---

## Compilar el Proyecto

### Navegar al directorio del proyecto

```bash
cd ruta/a/ESPoke
```

### Compilar

```bash
pio run
```

Esto descargará las dependencias necesarias y compilará el proyecto.

**Primera compilación puede tomar varios minutos** (descarga de toolchain, librerías, etc.)

### Ver salida de compilación

Si todo sale bien, verás algo como:

```
Parsing XML...
...
Linking .pio/build/esp32dev/firmware.elf
Calculating size .pio/build/esp32dev/firmware.elf
RAM:   [=         ]  10.2% (used 33408 bytes from 327680 bytes)
Flash: [==        ]  18.5% (used 484128 bytes from 1310720 bytes)
```

---

## Subir al ESP32

### 1. Conectar el ESP32

Conectar el ESP32 al ordenador mediante cable USB.

### 2. Identificar el puerto

**Windows:**
```bash
pio device list
```
Busca algo como `COM3`, `COM4`, etc.

**Linux/macOS:**
```bash
pio device list
```
Busca algo como `/dev/ttyUSB0`, `/dev/cu.usbserial-xxx`, etc.

### 3. Subir el firmware

```bash
pio run --target upload
```

O especificar el puerto:
```bash
pio run --target upload --upload-port COM3
```

### 4. Monitor serie (opcional)

Para ver los mensajes del programa:

```bash
pio device monitor
```

---

## Solución de Problemas

### Error: "python not found"

Agregar Python al PATH o usar `python3` en lugar de `python`.

### Error: "Permission denied" al subir

**Linux/macOS:**
```bash
sudo usermod -a -G dialout $USER
# Cerrar sesión y volver a entrar
```

### Error: "Failed to connect"

- Verificar que el cable USB funciona
- Presionar botón BOOT del ESP32 mientras se conecta
- Seleccionar el puerto correcto

### Error de compilación de librerías

Limpiar y recompilar:
```bash
pio run --target clean
pio run
```

---

## Personalización

### Cambiar pines

Editar `src/main. cpp`:
```cpp
#define BTN_PLAY 0      // Cambiar GPIO
#define BTN_NEXT 2
// etc.

#define PIN_DAC 25      // Pin de video
#define AUDIO_PIN 18    // Pin de audio
```

### Cambiar velocidad de frames

En `src/main.cpp`:
```cpp
const int FRAME__RATE = 10;  // Frames por segundo para CDG
```

---

## Comandos Útiles de PlatformIO

| Comando | Descripción |
|---------|-------------|
| `pio run` | Compilar |
| `pio run --target upload` | Compilar y subir |
| `pio device list` | Listar puertos serie |
| `pio device monitor` | Abrir monitor serie |
| `pio run --target clean` | Limpiar archivos de compilación |
| `pio lib list` | Listar librerías instaladas |
| `pio pkg list` | Listar dependencias del proyecto |

---

## Siguiente Paso

Una vez subido el firmware:
1. Conectar el hardware (ver README.md)
2. Colocar archivos .cdg y .mp3 en la SD card
3. Alimentar el ESP32 y conectar a TV

Para más detalles técnicos, ver README.md.
