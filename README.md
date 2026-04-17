# ESP32 Karaoke Player (ESPoke)

Reproductor de karaoke para ESP32 con salida de video compuesto NTSC.

## Características

- **Video**: Salida composite NTSC (336x240) usando la librería ESP32CompositeColorVideo
- **Formatos**: Soporta CDG (Compact Disc Graphics) - archivos MP3+G, OGG+G
- **Audio**: Reproducción de archivos MP3/OGG desde tarjeta SD
- **Control**: Botones para Play/Pause, Siguiente, Anterior, Volumen +/-

## Hardware Necesario

### Placa
- ESP32 (cualquier variante con DAC)

### Diagrama Completo de Conexiones

```
                    ┌─────────────────┐
                    │     ESP32       │
                    │                 │
                    │  ┌───────────┐  │
                    │  │   GPIO    │  │
                    │  │    25     │──┼────────────────────┐
                    │  │   DAC1    │  │                    │
                    │  │   GND     │──┼────────────────────┤
                    │  │    0      │  │                    │
                    │  │    2      │  │                    │
                    │  │    4      │  │                    │
                    │  │   13      │  │                    │
                    │  │   15      │  │                    │
                    │  │   18      │──┼────────────────────┤
                    │  │   19      │  │                    │
                    │  │   23      │  │                    │
                    │  │    5      │  │                    │
                    │  └───────────┘  │                    │
                    └─────────────────┘                    │
                                                           │
         ┌─────────────────────────────────────────────────┤
         │                    RCA JACK                      │
         │              (Video Compuesto)                  │
         │                                                 │
         │    ┌──────────────┐                             │
         │    │   1 Centre   │◄── GPIO 25 (DAC1)            │
         │    │              │                             │
         │    │  2 Outer     │◄── GND                       │
         │    └──────────────┘                             │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │              AUDIO OUTPUT (GPIO 18)              │
         │                                                  │
         │    GPIO 18 ──┬──[ 1kΩ ]──┬───► Speaker           │
         │              │           │                       │
         │              │           ├───[ 10nF ]── GND       │
         │              │           │                       │
         │              └───────────┘                       │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │                    SD CARD                      │
         │              (Lector SPI)                       │
         │                                                  │
         │    ESP32      Lector SD                         │
         │    GPIO 23 ──► MOSI                             │
         │    GPIO 19 ◄── MISO                             │
         │    GPIO 18 ──► CLK                              │
         │    GPIO 5  ──► CS                               │
         │    GND      ──► GND                             │
         │    3.3V     ──► VCC (si soporta 3.3V)          │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │                   BOTONES                       │
         │            (Todos con Pull-Up interno)          │
         │                                                  │
         │    ESP32      Función         Botón             │
         │    GPIO  0 ──► Play/Pause ◄───► [SW1]           │
         │    GPIO  2 ──► Siguiente   ◄───► [SW2]          │
         │    GPIO  4 ──► Anterior    ◄───► [SW3]          │
         │    GPIO 13 ──► Vol-        ◄───► [SW4]          │
         │    GPIO 15 ──► Vol+        ◄───► [SW5]          │
         │    GND      ──► GND (común)                     │
         │                                                  │
         │    [SW1]                    ▲                    │
         │      │                     │                    │
         │      └─────────[ GND ]─────┘                    │
         │                                                  │
         │    (Los botones conectan GPIO a GND)             │
         └─────────────────────────────────────────────────┘
```

### Resumen de Conexiones

| ESP32 GPIO | Función | Conexión Física |
|------------|---------|-----------------|
| 25 (DAC1) | Video | RCA centro (video) |
| GND | Masa | RCA exterior (masa) |
| 18 | Audio Out | Filtro RC → Speaker |
| 23 | SD MOSI | SD lector MOSI |
| 19 | SD MISO | SD lector MISO |
| 18 | SD CLK | SD lector CLK |
| 5 | SD CS | SD lector CS |
| 0 | Botón Play/Pause | Botón → GND |
| 2 | Botón Siguiente | Botón → GND |
| 4 | Botón Anterior | Botón → GND |
| 13 | Botón Vol- | Botón → GND |
| 15 | Botón Vol+ | Botón → GND |

**Nota**: GPIO 18 se usa para audio PWM Y clock SD. El filtro RC para audio está en serie, no afecta la señal SPI.

## Instalación

### Requisitos
- [PlatformIO](https://platformio.org/) instalado
- ESP32 toolchain

### Compilación
```bash
# Instalar PlatformIO si no lo tienes
pip install platformio

# Compilar el proyecto
pio run

# Subir al ESP32
pio run --target upload
```

## Archivos de Karaoke

### Estructura de archivos
```
/ karaoke/
   ├── song1.cdg
   ├── song1.mp3
   ├── song2.cdg
   └── song2.ogg
```

### Formatos soportados
- **CDG + MP3**: Archivos .cdg con audio .mp3
- **CDG + OGG**: Archivos .cdg con audio .ogg
- **CDG + WAV**: Archivos .cdg con audio .wav

Los archivos de video (.cdg) deben tener el mismo nombre que el audio correspondiente.

## Detalles Técnicos

### Video Compuesto (APLL)

El proyecto usa el **Audio Phase Locked Loop (APLL)** del ESP32 para generar portadoras de color NTSC precisas. Esta técnica proporciona color estable en lugar del método DAC+DDS más común.

**Frecuencias del portador de color:**
| Estándar | Frecuencia Objetivo | Frecuencia APLL |
|----------|---------------------|-----------------|
| NTSC     | 14.318182 MHz       | 14.318180 MHz   |
| PAL      | 17.734476 MHz       | 17.734476 MHz   |

El APLL permite un control de frecuencia muy preciso necesario para generar color NTSC estable en la mayoría de TVs.

### Audio PWM

El audio se genera usando el periférico **LED PWM** del ESP32:
- Frecuencia: 20 MHz
- Resolución: 7 bits
- Pin de salida: GPIO 18

Esta técnica produce audio de calidad suficiente para sonidos clásicos de los 80s.

### Comandos CDG Soportados

El parser implementa los siguientes comandos del formato CDG:
- `Memory Preset` (1): Limpiar pantalla con color
- `Border Preset` (2): Configurar color del borde
- `Tile Block` (6): Dibujar bloques de 6x12 pixels
- `Tile Block XOR` (38): Dibujar con operación XOR
- `Scroll Preset` (20): Desplazamiento con relleno
- `Scroll Copy` (24): Desplazamiento con copia
- `Define Transparent` (28): Color transparente
- `Load Static Color Table` (30): Paleta de colores
- `Load Static Data` (38): Datos de color adicionales

## Estructura del Proyecto

```
ESPoke/
├── SPEC. md               # Especificación técnica
├── platformio.ini         # Configuración de PlatformIO
├── README.md              # Este archivo
└── src/
    ├── main. cpp          # Programa principal
    ├── CDGParser.h        # Parser CDG (cabecera)
    ├── CDGParser.cpp      # Implementación del parser CDG
    ├── Player.h           # Reproductor (cabecera)
    ├── Player.cpp         # Lógica del reproductor
    ├── AudioOutput.h      # Salida de audio PWM
    └── AudioOutput.cpp    # Implementación de audio
```

## Créditos y Referencias

### Librerías y Proyectos Base
- **Video**: [ESP32CompositeColorVideo](https://github. com/marciot/ESP32CompositeColorVideo) de marciot
- **Técnica APLL**: Basada en [esp_8_bit](https://github.com/CornN64/esp_8_bit) de rossumur/CornN64
- **Formato CDG**: Especificación pública de CD+Graphics
- **Referencia karaoke**: [PyKaraoke](https://github. com/kelvinlawson/pykaraoke)

### Cómo funciona el video NTSC

El principio clave para generar color NTSC de buena calidad es la precisión y estabilidad de la portadora de color sintetizada. El método DAC + DDS a 13.33 MHz produce una forma de onda hermosa pero un color muy intermittent si funciona.

El ESP32 tiene una herramienta excelente para crear portadoras de color sólidas: el **Audio Phase Locked Loop (APLL)**. Este PLL fraccional-N de ultra-bajo ruido puede producir frecuencias de muestreo DAC de hasta ~20 MHz con control de frecuencia muy preciso.

### Alternativas de Audio

El proyecto usa LED PWM para audio pero hay otras opciones:
- **PDM (Pulse Density Modulation)**: Mayor calidad, más complejo
- **I2S**: Requiere hardware adicional
- **DAC**:Limitado cuando se usa APLL para video

## Solución de Problemas

### Sin color en la TV
- Verificar conexiones RCA
- Probar con otra TV (algunas TVs son más tolerantes)
- Ajustar sincronización

### Sin archivos detectados
- Verificar que la SD card esté formateada en FAT32
- Los archivos .cdg deben estar en la raíz o subcarpeta
- Nombres de archivo en formato 8.3 (sin espacios)

### Audio con ruido
- Verificar el filtro RC
- Reducir volumen si hay distorsión

## Licencia

MIT License - Libre para usar y modificar.
