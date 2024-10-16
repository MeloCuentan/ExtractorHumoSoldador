/*
  Utilizado un LTG8F328P
  Pines utilizados:
  pinPWM = 9
  pinRPM = 2
  pinPot = 0
  pinSDA = A4
  pinSCL = A5

  Pines del ventilador FFB1212VHE:
  - Amarillo = +12V
  - Negro = GND
  - Azul = PWM (se activa con señal negativa)
  - Verde = Velocidad del ventilador (pulsos negativos. Hay que poner PULLUP)

  Contectar la alimentación de 12V directamente al pin RAW del LGT8F328P
  La alimentacion del OLED y positivo del potenciómetro se tomará del pin 5V
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definir valores del OLED
const uint8_t ANCHO = 128;
const uint8_t ALTO = 64;
const uint8_t ADDR_OLED = 0x3C;

// Definir el pines y resolución del ADC
const uint8_t pinPWM = 9;         // Pin GPIO para la señal PWM
const uint8_t pinRPM = 2;         // Pin GPIO para medir las RPM
const uint8_t pinPot = 0;         // Pin GPIO donde se conecta el potenciómetro
const uint8_t resolucionADC = 8;  // Resolución del ADC

// Variables para el ciclo de trabajo
uint16_t valorPot = 0;              // Valor leído del potenciómetro (0-255 con 8 bits)
uint16_t rpmVentilador = 0;         // Medidmos las rpm del ventilador
const uint8_t pulsosPorVuelta = 2;  // Contador de pulsos por vuelta del ventilador

// Variables de tiempo
uint32_t tiempoActual;                     // Variable para controlar el tiempo actual
uint32_t tiempoLecturaPot;                 // Variable para controlar los tiempos de lectura del potenciómetro
uint32_t tiempoOLED;                       // Variable para controlar el tiempo de refresco del OLED
uint32_t tiempoAnteriorRPM = 0;            // Variable para controlar el conteo de pulsos
const uint32_t intervaloLecturaPot = 100;  // Intervalo de tiempo para lectura del potenciómetro
const uint32_t intervaloOLED = 100;        // Intervalo de tiempo para el refresco del OLED
const uint32_t unMinuto = 60000;           // Valor de un minuto en milisegundos
const uint32_t intervaloRPM = 500;         // Intervalo de tiempo para hacer el conteo de pulsos

// Variable de la interrupción
volatile uint32_t pulsos = 0;

// Creamos objeto del display
Adafruit_SSD1306 display(ANCHO, ALTO);

// Función de interrupción
void contarPulsos() {
  uint32_t temp = pulsos;  // Leer la variable
  temp++;                  // Incrementar el valor
  pulsos = temp;           // Escribir el valor actualizado de vuelta
}

void setup() {
  iniciarOLED();
  configurarPines();
  pantallaFija();
}

void loop() {
  tiempoActual = millis();  // Actualizar valor
  valorPot = analogRead(pinPot);  // Leer valor del potenciómetro

  if (tiempoActual - tiempoLecturaPot >= intervaloLecturaPot) { // Cada tiempo actualizamos el valor PWM
    tiempoLecturaPot = tiempoActual;
    if (valorPot <= 15) valorPot = 0;
    if (valorPot > 253) valorPot = 255;
    analogWrite(pinPWM, valorPot);
  }

  if (tiempoActual - tiempoAnteriorRPM >= intervaloRPM) {// Cada tiempo contamos los pulsos para calcular las RPM
    tiempoAnteriorRPM = tiempoActual;
    noInterrupts();
    uint32_t pulsosMedidos = pulsos;
    pulsos = 0;
    interrupts();
    rpmVentilador = (pulsosMedidos * unMinuto) / (intervaloRPM * pulsosPorVuelta);
  }

  if (tiempoActual - tiempoOLED >= intervaloOLED) {// Cada tiempo mostramos los datos en el display
    tiempoOLED = tiempoActual;
    mostrarOLED();
  }
}

// Iniciar display
void iniciarOLED() {
  display.begin(SSD1306_SWITCHCAPVCC, ADDR_OLED);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setRotation(0);
  display.clearDisplay();
  display.display();
}

// Configuramos los pines del LGT8F328P
void configurarPines() {
  analogReadResolution(resolucionADC);                                    // Configuramos el rango de lectura analógica a 8 bits (0 - 255)
  pinMode(pinPot, INPUT);                                                 // Configuramos el pin como entrada
  pinMode(pinPWM, OUTPUT);                                                // Configura el pin del PWM como salida
  pinMode(pinRPM, INPUT_PULLUP);                                          // Configuramos pin conmo entrada con PULLUP
  attachInterrupt(digitalPinToInterrupt(pinRPM), contarPulsos, FALLING);  // Activamos interrupción
}

// Creamos imagen estática del display
void pantallaFija() {
  display.clearDisplay();
  display.setCursor(12, 8);
  display.print("RPM:");
  display.setCursor(12, 40);
  display.print("POT:");
}

// Mostramos datos en el display
void mostrarOLED() {
  display.setCursor(72, 8);
  if (rpmVentilador < 10) display.print("   ");
  else if (rpmVentilador < 100) display.print("  ");
  else if (rpmVentilador < 1000) display.print(" ");
  display.print(rpmVentilador);

  display.setCursor(72, 40);
  if (valorPot == 0) {
    display.print("OFF ");
  } else {
    uint8_t porcentaje = map(valorPot, 0, 255, 0, 100);
    if (porcentaje < 10) display.print("  ");
    else if (porcentaje < 100) display.print(" ");
    display.print(porcentaje);
    display.print("%");
  }
  display.display();
}
