//*****************************************/
//Universidad del Valle de Guatemala 
//BE3029 - ELectrónica Digital 2
//Pablo Gómez - 24412
//Laboratorio No2
//*****************************************/

// Librerías
#include <Arduino.h>
#include <driver/gpio.h>
#include <stdint.h>

#define alarma 250000 // 250 ms para el Timer0 
#define antireb 200 // Anti-rebote para botones

// Definición para el Sensor Capacitivo 
#define cable T3 // Corresponde al GPIO 15 
#define umbral 30 // Punto de corte = menor a 30 significa que se tocó el pin

// Definiciones de Salidas (LEDs de Arriba - Contador Manual)
#define ledPinRA 26 // LED 1 (Rojo arriba)
#define ledPinBA 25 // LED 2 (Azul arriba)
#define ledPinWA 33 // LED 3 (Blanco arriba)
#define ledPinYA 32 // LED 4 (Amarillo arriba)

// Definiciones de Salidas (LEDs de Abajo - Contador Timer)
#define ledPinRB 4 // LED 1 (Rojo abajo)
#define ledPinBB 5 // LED 2 (Azul abajo)
#define ledPinWB 18 // LED 3 (Blanco abajo)
#define ledPinYB 19 // LED 4 (Amarillo abajo)

#define ledPinRso 27 // LED sola (Alarma por Coincidencia)

// Definiciones de Entradas (Botones)
#define retro 22 // Decrementar el contador
#define avan 23 // Incrementar el contador

// Variables Globales
hw_timer_t *Timer0_Cfg = NULL; // Puntero a la estructura del Timer0

// Variables volátiles para interrupciones (por ser void del timer)
volatile int8_t contb = 0;
volatile int8_t alarmapt1 = -1; // Inicia en -1 (LEDs superiores apagados)

// Variable entera para controlar el LED de alarma (0 = Apagado, 1 = Encendido)
volatile uint8_t estadoLedSola = 0; 

// Variables para el Anti-rebote
volatile uint32_t ultimoTiempoAvan = 0;
volatile uint32_t ultimoTiempoRetro = 0;

// Variable de estado anterior para el sensor capacitivo (0 = no tocado, 1 = tocado)
uint8_t tocantes = 0;

// Prototipos de funciones
void configTimer(void);
void configurarPuertos(void);
void mostrarBinarioArriba(void);
void verificarTouch(void);
void IRAM_ATTR ISR_Avan(void);
void IRAM_ATTR ISR_Retro(void);

// ISR del Timer0: Incrementa el contador contb y actualiza los LEDs de abajo
void IRAM_ATTR Timer0_ISR() {
  contb++;

  if (contb > 15) {
    contb = 0; // Comportamiento circular
  }

  // Mostrar estado del contador contb en los LEDs de abajo
  digitalWrite(ledPinRB, bitRead(contb, 0)); // Bit 0 
  digitalWrite(ledPinBB, bitRead(contb, 1)); // Bit 1
  digitalWrite(ledPinWB, bitRead(contb, 2)); // Bit 2
  digitalWrite(ledPinYB, bitRead(contb, 3)); // Bit 3 

  // Comparar arriba y abajo para activar el LED de alarma (ledPinRso)
  if (alarmapt1 >= 0 && contb == alarmapt1) {
    estadoLedSola = 1 - estadoLedSola; // Alterna numéricamente entre 0 y 1
    digitalWrite(ledPinRso, estadoLedSola); // Aplica el entero (0 o 1) directamente al pin
    contb = -1; // Reinicia el contador de abajo
  }
}

// ISR para el botón de incremento (avan)
void IRAM_ATTR ISR_Avan() {
  uint32_t tiempoActual = millis();

  if (tiempoActual - ultimoTiempoAvan > antireb) {
    alarmapt1++;
    if (alarmapt1 > 15) {
      alarmapt1 = -1; // Reinicia a -1 (apagado)
    }
    mostrarBinarioArriba();
    ultimoTiempoAvan = tiempoActual;
  }
}

// ISR para el botón de decremento (retro)
void IRAM_ATTR ISR_Retro() {
  uint32_t tiempoActual = millis();

  if (tiempoActual - ultimoTiempoRetro > antireb) {
    alarmapt1--;
    if (alarmapt1 < -1) {
      alarmapt1 = 15; // Regresa a 15
    }
    mostrarBinarioArriba();
    ultimoTiempoRetro = tiempoActual;
  }
}

// Configuración de Hardware
void setup() {
  configurarPuertos();
  configTimer();
  mostrarBinarioArriba(); // Apaga los LEDs superiores al inicio por estar en -1
  
  // Asignación directa de interrupciones externas 
  attachInterrupt(avan, ISR_Avan, FALLING);
  attachInterrupt(retro, ISR_Retro, RISING);
}

void loop() {
  // Para ver lo del Sensor Capacitivo
  verificarTouch();
}
 // ISR para ver el estado de tocar el cable (Sensor Capacitivo)
void verificarTouch(void) {
  int valorTouch = touchRead(cable); // Lee la capacidad del pin capacitivo

  // Evaluación con valor entero: 1 si es menor que el umbral (tocado), 0 si es mayor (suelto)
  uint8_t tocadoActual = (valorTouch < umbral) ? 1 : 0;

  // Detecta el flanco de subida (transición de 0 a 1)
  if (tocadoActual == 1 && tocantes == 0) {
    contb = -1; // Reinicia el contador del timer a 0
  }

  tocantes = tocadoActual; // Guarda el estado entero actual
}

// Configuración de Pines
void configurarPuertos(void) {
  pinMode(ledPinRA, OUTPUT);
  pinMode(ledPinBA, OUTPUT);
  pinMode(ledPinWA, OUTPUT);
  pinMode(ledPinYA, OUTPUT);

  pinMode(ledPinRB, OUTPUT);
  pinMode(ledPinBB, OUTPUT);
  pinMode(ledPinWB, OUTPUT);
  pinMode(ledPinYB, OUTPUT);

  pinMode(ledPinRso, OUTPUT);

  pinMode(avan, INPUT_PULLUP);
  pinMode(retro, INPUT);
}

// Configuración de Timers
void configTimer(void) {
  Timer0_Cfg = timerBegin(0, 80, true); // Timer0, prescaler 80
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);

  // Timer0: Alarma a 250,000 us (250 ms)
  timerAlarmWrite(Timer0_Cfg, alarma, true); 
  timerAlarmEnable(Timer0_Cfg);
}

// Muestra el valor de alarmapt1 en los LEDs de arriba
void mostrarBinarioArriba(void) {
  if (alarmapt1 < 0) {
    digitalWrite(ledPinRA, LOW);
    digitalWrite(ledPinBA, LOW);
    digitalWrite(ledPinWA, LOW);
    digitalWrite(ledPinYA, LOW);
  } else {
    digitalWrite(ledPinRA, bitRead(alarmapt1, 0)); // Bit 0
    digitalWrite(ledPinBA, bitRead(alarmapt1, 1)); // Bit 1
    digitalWrite(ledPinWA, bitRead(alarmapt1, 2)); // Bit 2
    digitalWrite(ledPinYA, bitRead(alarmapt1, 3)); // Bit 3
  }
}