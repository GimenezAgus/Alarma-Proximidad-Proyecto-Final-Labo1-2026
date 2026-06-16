// ============================================================
// SISTEMA DE ALARMA DE PROXIMIDAD
// Laboratorio de Computacion I - UNSAM
// Alumno: Giménez Agustín
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Pines ---
// --- SENSOR ULTRASÓNICO ---
#define TRIG            9
#define ECHO            10

//--- LEDS (Salidas digitales) ---
#define LED_VERDE       3
#define LED_AMARILLO    4
#define LED_ROJO        5

// -- PULSADOR/BOTON (Entrada digital) --
#define BOTON           2

// -- BUZZER (SALIDA ANALÓGICA) --
#define BUZZER          6 

// --- Estados ---
#define ESTADO_DESARMADO  0
#define ESTADO_ARMANDO    1
#define ESTADO_ARMADO     2
#define ESTADO_ALERTA     3

// --- Tiempos ---
#define TIEMPO_ARMADO     5000  // ms de cuenta regresiva antes de armarse
#define TIEMPO_PARPADEO   300   // ms de cadencia de parpadeo de LEDs
#define TIEMPO_REBOTE     50    // ms de filtro antirrebote del boton (FLANCOS)
#define TIEMPO_SENSOR     500   // ms entre cada lectura del sensor

// --- Umbral del sensor ---
#define DISTANCIA_UMBRAL  30    // cm — el buzer varia en este rango y el sensor se activa en este rango

// --- Inicialización LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Variables de estado ---
int estado = ESTADO_DESARMADO; // ESTADO_DESARMADO = 0
int contadorPulsaciones = 0;

// --- Variables de tiempo ---

// Millis para la maquina de estados (cuenta de 5 seg de ARMANDO)
unsigned long millisAnteriores = 0;

// Millis para el antirrebote del boton (flancos)
unsigned long millisBoton      = 0;

// -- Millis para led --
unsigned long millisParpadeo   = 0;

// -- milis para el SENSOR
unsigned long millisSensor     = 0;

// --- Variables de boton ---
int estadoBoton         = LOW;
int estadoBotonAnterior = LOW;

// --- Otras variables auxiliares ---

// --- LED ---
int ledEncendido         = LOW;

// --- LCD ---
long distanciaAnteriorLCD = -1;  // para detectar cambio de distancia en ALERTA
int estadoAnteriorLCD    = -1;  // estados posibles según la lógica del 0 al 4, con -1 forzamos que dibuje en la primer pasada
int ultimoSegundo        = -1; // lo mismo que arriba, se fuerza para la primer pasada

long ultimaDistancia     = 999; // Al ser 30 cm la distancia máxima, no se activa la alarma en la primer pasada

// 1ER FUNCION PARA MEDIR DISTANCIA EN EL SENSOR ÚLTRA SÓNICO HC-SR04
// Devuelve valores en cm. 999 = sin respuesta.
long medirDistancia() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long tiempo = pulseIn(ECHO, HIGH, 30000);
    if (tiempo == 0) return 999;
    return tiempo / 58;
}


// 2DA FUNCIÓN: Actualiza el LCD solo cuando algo cambia, si no se queda esperando.
// seg: segundos restantes (se muestra durante ARMANDO)
// dist: distancia actual (se muestra durante ALERTA)

// arranca como estado  = ESTADO_DESARMADO;
// Arranca como estadoAnteriorLCD = -1
void actualizarLCD(int seg, long dist) {
    int debeActualizar = 0;

    //SOLO HAY TRES CASOS EN LOS QUE ACTUALIZA EL LCD

    // Estado es diferentes al anterior
    if (estado != estadoAnteriorLCD) { 
        debeActualizar = 1;
        estadoAnteriorLCD = estado;
        ultimoSegundo = -1;
        distanciaAnteriorLCD = -1;
    }

    //El estado está ARMANDO y los segundos son diferentes a los anteriores
    if (estado == ESTADO_ARMANDO && seg != ultimoSegundo) {
        debeActualizar = 1;
        ultimoSegundo = seg;
    }

    //Si está en alerta y la distancia es diferente a la anterior
    if (estado == ESTADO_ALERTA && dist != distanciaAnteriorLCD) {
        debeActualizar = 1;
        distanciaAnteriorLCD = dist;
    }

  //Si no entra en nninguna de las anteriores finaliza
  if (debeActualizar == 0) return;

    //Limpieza de LCD
    lcd.clear();
    lcd.setCursor(0, 0);

    //Control de estados
    switch (estado) {
        case ESTADO_DESARMADO:
            lcd.print("Sistema: OFF");
            lcd.setCursor(0, 1);
            lcd.print("Pulsaciones: ");
            lcd.print(contadorPulsaciones);
            break;

        case ESTADO_ARMANDO:
            lcd.print("Armando...");
            lcd.setCursor(0, 1);
            lcd.print("Espere: ");
            lcd.print(seg);
            lcd.print("s");
            break;

        case ESTADO_ARMADO:
            lcd.print("Sistema: ARMADO");
            lcd.setCursor(0, 1);
            lcd.print("Pulsaciones: ");
            lcd.print(contadorPulsaciones);
            break;

        case ESTADO_ALERTA:
            lcd.print("!!INTRUSO!!");
            lcd.setCursor(0, 1);
            //Si detecta distancias muy lejanas no la muestra.
            if (dist > 500) {
                lcd.print("Dist: ---");
            } else {
                lcd.print("Dist: ");
                lcd.print(dist);
                lcd.print("cm");
            }
            break;
    }
}

// --- SETUP ---
void setup() {
    pinMode(TRIG,         OUTPUT);
    pinMode(ECHO,         INPUT);
    pinMode(BOTON,        INPUT);
    pinMode(LED_VERDE,    OUTPUT);
    pinMode(LED_AMARILLO, OUTPUT);
    pinMode(LED_ROJO,     OUTPUT);
    pinMode(BUZZER,       OUTPUT);

    lcd.init();
    lcd.backlight();

    //-- Inicialización del serial y mensaje inicial --
    Serial.begin(9600);
    Serial.println("Sistema iniciado - DESARMADO");
}

// --- LOOP ---
void loop() {
    unsigned long millisActuales = millis();

    // LECTURA DE BOTON
    // Detecta el momento exacto en que el boton pasa de LOW a HIGH.
    // El filtro de 50ms evita contar rebotes mecanicos.

    // Leo el botón
    estadoBoton = digitalRead(BOTON);
    int flancoDetectado = 0;

    // Detección de flancos para el botón
    if ((estadoBoton == HIGH) && (estadoBotonAnterior == LOW)) {
        if ((millisActuales - millisBoton) >= TIEMPO_REBOTE) {
            flancoDetectado = 1;
            contadorPulsaciones++;
            estadoBotonAnterior = estadoBoton;
            Serial.print("Pulsacion detectada! Total: ");
            Serial.println(contadorPulsaciones);
        }
    } else {
        estadoBotonAnterior = estadoBoton;
        millisBoton = millisActuales;
    }

    // LECTURA DEL SENSOR cada TIEMPO_SENSOR ms
    // Solo mide cuando el estado lo requiere.

    if ((estado == ESTADO_ARMADO || estado == ESTADO_ALERTA) &&
        (millisActuales - millisSensor) >= TIEMPO_SENSOR) {

        millisSensor = millisActuales;
        ultimaDistancia = medirDistancia();

        // -- ESTADO ALERTA DEVUELVE LOS CM EN TIEMPO REAL --
        if (estado == ESTADO_ALERTA) {
            if (ultimaDistancia > 500) {
                Serial.println("¡Intruso! (sin lectura)");
            } else {
                Serial.print("¡Intruso a ");
                Serial.print(ultimaDistancia);
                Serial.println("cm!");
            }
        } else {
            Serial.print("Dist: ");
            Serial.print(ultimaDistancia);
            Serial.println(" cm");
        }
    }

    // SEGUNDOS RESTANTES para mostrar en LCD durante ARMANDO
    int seg = (TIEMPO_ARMADO - (int)(millisActuales - millisAnteriores)) / 1000 + 1; // lo divide por mil para pasarlo a seg el + 1 es para no mostrar el cero
    if (seg < 0) seg = 0; // Si es menor que cero setea 0


    // TRANSICIONES DE ESTADO
    // Decide si el sistema debe cambiar de un estado a otro.
    switch (estado) {

        case ESTADO_DESARMADO:
            // Si se pulsó el botón pasa a estado ARMANDO
            if (flancoDetectado) {
                estado = ESTADO_ARMANDO;
                millisAnteriores = millisActuales;
                estadoAnteriorLCD = -1;
                Serial.println("-> ARMANDO");
            }
            break;

        case ESTADO_ARMANDO:
            // Si pasó más de 5seg pasa a estado ARMADO
            if ((millisActuales - millisAnteriores) >= TIEMPO_ARMADO) {
                estado = ESTADO_ARMADO;
                millisAnteriores = millisActuales;
                estadoAnteriorLCD = -1;
                Serial.println("-> ARMADO");
            }
            break;

        case ESTADO_ARMADO:
            // Si la última distancia es menor a 30 cm pasa a estado ALERTA
            if (ultimaDistancia < DISTANCIA_UMBRAL) {
                estado = ESTADO_ALERTA;
                millisAnteriores = millisActuales;
                estadoAnteriorLCD = -1;
                Serial.println("-> ALERTA");
            }
            break;

        case ESTADO_ALERTA:
            //Si se precionó el botón pasa a estado DESARMADO
            if (flancoDetectado) {
                estado = ESTADO_DESARMADO;
                millisAnteriores = millisActuales;
                estadoAnteriorLCD = -1;
                analogWrite(BUZZER, 0);
                Serial.println("-> DESARMADO");
            }
            break;
    }


    // SALIDAS POR ESTADO
    // Activa o desactiva cada componente segun el estado actual.
    switch (estado) {

        // DESARMADO: BUZZER APAGADO Y LED VERDE PRENDIDO
        case ESTADO_DESARMADO:
            digitalWrite(LED_VERDE,    HIGH);
            digitalWrite(LED_AMARILLO, LOW);
            digitalWrite(LED_ROJO,     LOW);
            analogWrite(BUZZER, 0);
            break;

        //ARMANDO: LED AMARILLO PARMADEA
        case ESTADO_ARMANDO:
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_ROJO,  LOW);
            // LED amarillo se prende o se apaga por 300ms y el estado de armando dura 5seg
            if ((millisActuales - millisParpadeo) >= TIEMPO_PARPADEO) {
                millisParpadeo = millisActuales;
                ledEncendido = !ledEncendido;
                digitalWrite(LED_AMARILLO, ledEncendido);
            }
            analogWrite(BUZZER, 0);
            break;

        case ESTADO_ARMADO:
            digitalWrite(LED_VERDE,    LOW);
            digitalWrite(LED_AMARILLO, LOW);
            digitalWrite(LED_ROJO,     HIGH);
            analogWrite(BUZZER, 0);
            break;

        case ESTADO_ALERTA: {
            digitalWrite(LED_VERDE,    LOW);
            digitalWrite(LED_AMARILLO, LOW);
            //Misma lógica que el led amarillo pero se apaga cuando se pulsa el boton
            if ((millisActuales - millisParpadeo) >= TIEMPO_PARPADEO) {
                millisParpadeo = millisActuales;
                ledEncendido = !ledEncendido;
                digitalWrite(LED_ROJO, ledEncendido);
            }
            // Intensidad del buzzer varia segun distancia
            // Mas cerca del objeto = mas intensidad (0 a 255)
            int intensidad = map(ultimaDistancia, 0, DISTANCIA_UMBRAL, 255, 80); // map(valor, entradaMin, entradaMax, salidaMin, salidaMax)
            // A mayor intensidad menor es la distancia
            if (intensidad < 0)   intensidad = 0;
            if (intensidad > 255) intensidad = 255;
            analogWrite(BUZZER, intensidad);
            break;
        }
    }


    // ACTUALIZAR LCD
    actualizarLCD(seg, ultimaDistancia);
}
