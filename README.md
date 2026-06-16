# Alarma de Proximidad — Arduino UNO

TP final de Laboratorio de Computación I · TUPI, UNSAM · 1er cuatrimestre 2026

---

## Descripción

Sistema de alarma activada por proximidad con control por pulsador. El usuario arma o desarma el sistema con un botón. Una vez armada, el sensor mide la distancia en forma continua y activa la alarma si detecta algo a menos de 30 cm. El LCD muestra el estado actual en todo momento.

## Componentes

- Arduino UNO
- Sensor ultrasónico HC-SR04
- LCD 16x2 + módulo I2C (PCF8574, dirección 0x27)
- Buzzer
- LED verde, amarillo y rojo
- Pulsador táctil
- Resistencias 220Ω (LEDs) y 10kΩ (pull-down del botón)
- Breadboard + cables

## Pines

| Componente | Pin Arduino |
|---|---|
| HC-SR04 Trigger | 9 |
| HC-SR04 Echo | 10 |
| Pulsador | 2 |
| LED verde | 3 |
| LED amarillo | 4 |
| LED rojo | 5 |
| Buzzer (PWM) | 6 |
| LCD SDA | A4 |
| LCD SCL | A5 |

## Máquina de estados

```
DESARMADO ──[botón]──► ARMANDO ──[5 seg]──► ARMADO ──[dist < 30cm]──► ALERTA
                                                                           │
                                               ◄──────────[botón]─────────┘
```

| Estado | LED | Buzzer | Sensor | LCD |
|---|---|---|---|---|
| DESARMADO | Verde fijo | Off | Off | `Sistema: OFF` |
| ARMANDO | Amarillo parpadea | Off | Off | `Armando... Xs` |
| ARMADO | Rojo fijo | Off | Activo | `Sistema: ARMADO` |
| ALERTA | Rojo parpadea | Sí (PWM) | — | `!! INTRUSO !!` |

## Librerías necesarias

- [`LiquidCrystal_I2C`](https://github.com/johnrickman/LiquidCrystal_I2C) — para el display LCD

Instalar desde el Library Manager del IDE de Arduino.

## Notas de implementación

- Toda la lógica de tiempo usa `millis()`, sin ningún `delay()`
- El pulsador usa resistencia pull-down externa (10kΩ a GND), no el pull-up interno
- La detección del botón es por flanco de subida (LOW→HIGH) con antirrebote por software (50ms)
- El buzzer está en pin PWM para poder variar la intensidad con `analogWrite()` según la distancia
- Dos bloques `switch` separados: uno para transiciones de estado, uno para las salidas

## Cómo usar

1. Conectar el circuito según la tabla de pines
2. Instalar la librería `LiquidCrystal_I2C`
3. Cargar `alarma_proximidad.ino` al Arduino
4. Con el sistema encendido, presionar el botón para armar
5. El sistema entra en ARMANDO (5 segundos de espera), luego pasa a ARMADO
6. Si algo se acerca a menos de 30 cm, se activa la alerta
7. Presionar el botón para desactivar la alarma y volver a DESARMADO

---

Entorno: Arduino IDE · Lenguaje: C  
Materia: Laboratorio de Computación I · Prof. Pedro Iriso
