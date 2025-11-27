#include <LiquidCrystal.h>
#include <Thread.h>
#include <ThreadController.h>
#include <DHT.h>
#include <avr/wdt.h>

// ----- LCD -----
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = A0, d7 = A1;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // Configura LCD 16x2

// ----- Ultrasonido -----
const int TRIG_PIN = 9;
const int ECHO_PIN = 8;
const float V_SOUND_AIR = 0.0343f; // Velocidad del sonido en cm/us
float distance = 0.0f; // Distancia medida por el sensor
long duration  = 0;     // Tiempo de pulso del ultrasonido

// ----- Joystick -----
const int pinX = A3; // Eje X del joystick
const int pinY = A2; // Eje Y del joystick
const int UMBRAL_MIN = 400; // Limite inferior/izquierda para detectar movimiento
const int UMBRAL_MAX = 600; // Limite superior/derecha para detectar movimiento

int seleccion = 0; // Producto actualmente seleccionado
const int NUM_PROD = 5; // Número de productos disponibles
unsigned long lastMove = 0; // Tiempo del último movimiento para anti-rebote
const byte JOY_BUTTON = 3;  // Pin del botón del joystick

// ----- Boton -----
const byte BUTTON = 2; 
volatile bool botonPresionado = false; // Flag si el botón está presionado
volatile unsigned long buttonPressStart = 0; // Tiempo en que se empezó a presionar
bool adminON = false; // Indica si estamos en modo Admin

// ----- LEDs -----
const int LED1 = 7; // LED indicador general / parpadeo
const int LED2 = 6; // LED progreso preparación bebida

// ----- Temperatura -----
const int DHT_PIN = 10;
#define DHTTYPE DHT11
DHT dht(DHT_PIN,DHTTYPE); // Sensor DHT11

float humedad = 0.0; // Humedad actual
float temperatura = 0.0; // Temperatura actual
unsigned long lastDHT=0; // Tiempo de última lectura
const unsigned long DHT_INTERVAL = 2000; // Intervalo de lectura del DHT

// ----- Admin -----
const int NUM_OPC = 4; // Numero de opciones del menu de Admin
const char* opciones[NUM_OPC] = {"1.- Temp/Hum","2.- Distancia","3.- Contador","4.- Modf Precios"};
int opcion = 0; // Opcion seleccionada en menu admin
enum AdminSubEstado { MENU_PRINCIPAL, SUBMENU };
AdminSubEstado adminSubEstado = MENU_PRINCIPAL; // Sub-estado de menu admin
int subSeleccion = 0; // Para modificar precios de productos

// ----- Contadores -----
unsigned long timer = 0; // Contador general para admin
unsigned long lastTimer = 0; // Última actualización del contador
unsigned long lastPressed = 0; // Anti-rebote botón del joystick

// Array de precios de productos en céntimos
int precios[NUM_PROD] = {100,110,125,150,200};

// ----- Threads -----
ThreadController controller; // Controlador de threads
Thread threadUltrasonido; // Thread para leer ultrasonido
Thread threadJoystick;    // Thread para leer joystick
Thread threadDHT;         // Thread para leer temperatura/humedad

// Lista de productos
const char* productos[NUM_PROD] = {"1 Solo","2 Cortado","3 Doble","4 Premium","5 Choco"};

// Máquina de estados principales
enum Estado { 
  ARRANQUE,            // Estado inicial de arranque
  ESPERANDO,           // Esperando cliente
  MOSTRAR_TEMP_HUM,    // Muestra temperatura y humedad
  PRODUCTOS,           // Muestra productos
  PREPARANDO,          // Preparando bebida
  RETIRAR,             // Retirar bebida
  ADMIN                // Modo administrador
};
Estado estadoActual = ARRANQUE;
Estado estadoPrevio; // Guarda estado previo al entrar en admin

// ----- Variables -----
unsigned long inicioPrep = 0; // Tiempo inicio preparación bebida
unsigned long duracionPreparacion = 0; // Duración aleatoria preparación
bool detectado = false; // Si cliente detectado
int lastShown = -1; // Último producto mostrado en LCD
unsigned long lastPWM = 0; // Tiempo del último update del LED2
unsigned long lastService=0; // Tiempo de último servicio mostrado

const unsigned long LCD_delay = 1000; // Tiempo mínimo de refresco LCD

// Variables parpadeo inicial
unsigned long lastBlink = 0;
int blinkCount = 0;
bool ledOn = false;

// Variables para actualizar LCD solo si hay cambio de temperatura/humedad
float ultimaTemp = -1;
float ultimaHum = -1;

// ---------------- Funciones ----------------

// Lee temperatura y humedad del sensor DHT11
void leerTemperatura(){
  if (millis() - lastDHT < DHT_INTERVAL) return; // Solo cada 2s
  lastDHT = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) { // Si lectura correcta
    humedad = h;
    temperatura = t;
  }
}

// Mide la distancia con el sensor de ultrasonido
void medirUltrasonido(){
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10); // Pulso ultrasonido
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH); // Tiempo de eco
  distance = duration * V_SOUND_AIR / 2.0f; // Calcula distancia
}

// Muestra productos en el LCD con precio y selección actual
void mostrarProductos() {
  lcd.clear();
  lcd.setCursor(0,0);
  int fila0 = seleccion;
  int fila1 = seleccion + 1;
  if(fila1 >= NUM_PROD){
    fila1 = -1; // No hay segunda fila
  } 

  lcd.print(">");
  lcd.print(productos[fila0]);
  lcd.print(" ");
  lcd.print(precios[fila0] / 100.0, 2); // Precio en euros
  lcd.print("E");

  lcd.setCursor(0,1);
  if (fila1 != -1) { // Si hay siguiente producto
    lcd.print(" ");
    lcd.print(productos[fila1]);
    lcd.print(" ");
    lcd.print(precios[fila1] / 100.0, 2);
    lcd.print("E");
  } else {
    lcd.print("                "); // Limpia segunda fila
  }
}

// Muestra menú del modo administrador
void adminMenu() {
  lcd.clear();
  lcd.setCursor(0,0);

  if (adminSubEstado == MENU_PRINCIPAL) {
    // Muestra dos opciones principales
    lcd.print(">");
    lcd.print(opciones[opcion]);
    lcd.setCursor(0,1);
    int next_op = opcion + 1;
    if (next_op < NUM_OPC) {
      lcd.print(" ");
      lcd.print(opciones[next_op]);
    } else {
      lcd.print("                "); // Limpia fila
    }
  } 
  else if (adminSubEstado == SUBMENU) {
    // Submenu según opción seleccionada
    switch(opcion) {
      case 0: // Temp/Hum
        leerTemperatura();
        lcd.setCursor(0,0);
        lcd.print("Temp: ");
        lcd.print(temperatura);
        lcd.print("C");
        lcd.setCursor(0,1);
        lcd.print("Hum: ");
        lcd.print(humedad);
        lcd.print("%");
        break;

      case 1: // Distancia
        medirUltrasonido();
        lcd.setCursor(0,0);
        lcd.print("Distancia:");
        lcd.setCursor(0,1);
        lcd.print(distance);
        lcd.print(" cm");
        break;

      case 2: // Contador
        lcd.setCursor(0,0);
        lcd.print("Contador s:");
        lcd.setCursor(0,1);
        lcd.print(timer);
        break;

      case 3: // Modificar precios
        lcd.setCursor(0,0);
        lcd.print("Modificar:");
        lcd.setCursor(0,1);
        lcd.print(productos[subSeleccion]);
        lcd.print(" ");
        lcd.print(precios[subSeleccion]/100.0,2);
        lcd.print("E");
        break;
    }
  }
}

// Lee movimientos del joystick y navega según estado
void lecturaJoystick(){
  int valX = analogRead(pinX);
  int valY = analogRead(pinY);

  // Navegación productos
  if (estadoActual == PRODUCTOS) {
    if (valX < UMBRAL_MIN && millis() - lastMove > 200) { // Izquierda
      if (seleccion < NUM_PROD - 1) seleccion++;
      lastMove = millis();
    } else if (valX > UMBRAL_MAX && millis() - lastMove > 200) { // Derecha
      if (seleccion > 0) seleccion--;
      lastMove = millis();
    }
  }

  // Navegación admin
  if (estadoActual == ADMIN) {
    if (adminSubEstado == MENU_PRINCIPAL) {
      if (valX < UMBRAL_MIN && millis() - lastMove > 200) {
        if (opcion < NUM_OPC - 1) opcion++;
        lastMove = millis();
        adminMenu();
      } else if (valX > UMBRAL_MAX && millis() - lastMove > 200) {
        if (opcion > 0) opcion--;
        lastMove = millis();
        adminMenu();
      }

      if (digitalRead(JOY_BUTTON) == LOW && millis() - lastPressed > 250) {
        lastPressed = millis();
        adminSubEstado = SUBMENU; // Entramos al submenu
        subSeleccion = 0;
        adminMenu();
      }

    } else if (adminSubEstado == SUBMENU) {
      // Submenu modificar precios
      if (opcion == 3) {
        if (valX < UMBRAL_MIN && millis() - lastMove > 200) {
          precios[subSeleccion] += 5; // Incrementa precio
          lastMove = millis();
          adminMenu();
        } else if (valX > UMBRAL_MAX && millis() - lastMove > 200) {
          precios[subSeleccion] -= 5; // Decrementa precio
          if (precios[subSeleccion] < 0) precios[subSeleccion] = 0;
          lastMove = millis();
          adminMenu();
        }
        if (digitalRead(JOY_BUTTON) == LOW && millis() - lastPressed > 250) {
          lastPressed = millis();
          subSeleccion++;
          if (subSeleccion >= NUM_PROD) subSeleccion = 0;
          adminMenu();
        }
      }

      // Salir submenu
      if (valY > UMBRAL_MAX && millis() - lastMove > 200) {
        adminSubEstado = MENU_PRINCIPAL;
        lastMove = millis();
        adminMenu();
      }
    }
  }
}

// Refresca pantalla admin según subestado
void refresh() {
  if (estadoActual != ADMIN) return; // Solo si estamos en admin

  digitalWrite(LED1,HIGH);
  digitalWrite(LED2,HIGH);

  // Actualiza submenus Temp/Hum, Distancia o Contador
  if (adminSubEstado == SUBMENU && opcion != 3) adminMenu();
}

// Resetea máquina a estado esperando cliente
void botonReinicio() {
  if (estadoActual == PRODUCTOS || estadoActual == MOSTRAR_TEMP_HUM) {
    estadoActual = ESPERANDO;
    detectado = false;
    lastShown = -1;
    if (!adminON) analogWrite(LED2,0); // Apaga LED2 si no admin
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Servicio");
    lastService = millis();
  }
}

// Activa o desactiva modo admin
void botonAdmin() {
  if (!adminON) { // Activamos admin
    estadoPrevio = estadoActual; // Guardamos estado previo
    estadoActual = ADMIN;
    adminON = true;
    adminSubEstado = MENU_PRINCIPAL;
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    adminMenu();
  } else { // Desactivamos admin
    estadoActual = estadoPrevio;
    adminON = false;
    digitalWrite(LED1,LOW);
    digitalWrite(LED2,LOW);
    if (estadoActual == PRODUCTOS) mostrarProductos();
  }
}

// ----- Interrupción para el botón -----
void interruptBoton() {
  static unsigned long ultimoCambio = 0;
  unsigned long ahora = millis();

  if (ahora - ultimoCambio < 50) return; // Anti-rebote
  ultimoCambio = ahora;

  if (digitalRead(BUTTON) == LOW) { // Pulsado
    buttonPressStart = ahora;
    botonPresionado = true;
  } else { // Soltado
    if (!botonPresionado) return;
    unsigned long tiempo = ahora - buttonPressStart;
    if (tiempo >= 2000 && tiempo < 3000) botonReinicio(); // 2-3s reinicia
    else if (tiempo >= 5000) botonAdmin(); // >5s entra admin
    botonPresionado = false;
  }
}

void setup() {
  wdt_enable(WDTO_2S); // Watchdog 2s
  lcd.begin(16,2); // LCD 16x2
  lcd.clear();
  Serial.begin(9600);

  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(JOY_BUTTON, INPUT_PULLUP);

  dht.begin();

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), interruptBoton, CHANGE);

  // Configura threads
  threadUltrasonido.onRun(medirUltrasonido);
  threadUltrasonido.setInterval(150);
  controller.add(&threadUltrasonido);

  threadJoystick.onRun(lecturaJoystick);
  threadJoystick.setInterval(100);
  controller.add(&threadJoystick);

  threadDHT.onRun(leerTemperatura);
  threadDHT.setInterval(2000);
  controller.add(&threadDHT);
}

void loop() {
  wdt_reset(); // Reset watchdog
  controller.run(); // Ejecuta threads

  switch(estadoActual) {
    case ARRANQUE:
      if (millis() - lastBlink >= 1000) { // Parpadeo LED1
        lastBlink = millis();
        ledOn = !ledOn;
        digitalWrite(LED1, ledOn);
        if (!ledOn) blinkCount++;
      }
      lcd.setCursor(0,0);
      lcd.print("CARGANDO...   ");
      if (blinkCount >= 3) { // Tras 3 parpadeos
        estadoActual = ESPERANDO;
        digitalWrite(LED1, LOW);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Servicio");
        lastService = millis();
      }
      break;

    case ESPERANDO:
      if (millis()-lastService < LCD_delay) break; // Espera mínima
      if (distance > 100.0f && !detectado) { // Cliente lejos
        static unsigned long lastEsperandoUpdate = 0;
        if (millis() - lastEsperandoUpdate > 500) {
          lastEsperandoUpdate = millis();
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("ESPERANDO CLIENTE");
        }
      } else { // Cliente detectado
        estadoActual = MOSTRAR_TEMP_HUM;
        lcd.clear();
        lcd.setCursor(0,0);
        inicioPrep = millis();
      }
      break;

    case MOSTRAR_TEMP_HUM:
      leerTemperatura();
      if (temperatura != ultimaTemp || humedad != ultimaHum) { // Solo si cambia
        ultimaTemp = temperatura;
        ultimaHum = humedad;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Temp: ");
        lcd.print(temperatura);
        lcd.print("C");
        lcd.setCursor(0,1);
        lcd.print("Hum: ");
        lcd.print(humedad);
        lcd.print("%");
      }
      if (millis() - inicioPrep >= 5000) { // Tras 5s
        estadoActual = PRODUCTOS;
        lcd.clear();
        mostrarProductos();
        lastShown = seleccion;
      }
      break;

    case PRODUCTOS:
      if (seleccion != lastShown) { // Cambió selección
        mostrarProductos();
        lastShown = seleccion;
      }
      if (digitalRead(JOY_BUTTON) == LOW) { // Pulsado para preparar bebida
        duracionPreparacion = random(4000,8001);
        inicioPrep = millis();
        estadoActual = PREPARANDO;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Preparando Cafe...");
      }
      break;

    case PREPARANDO:
      if (millis() - lastPWM >= 30) { // Actualiza LED2 como barra de progreso
        lastPWM = millis();
        float progreso = float(millis() - inicioPrep) / duracionPreparacion;
        if (progreso > 1.0f) progreso = 1.0f;
        float curva = pow(progreso, 1.6f); // Curva suavizada
        analogWrite(LED2, int(curva*255));
      }
      if (millis() - inicioPrep >= duracionPreparacion) { // Fin preparación
        estadoActual = RETIRAR;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("RETIRAR BEBIDA");
        analogWrite(LED2,255); // LED completo
        inicioPrep = millis();
      }
      break;

    case RETIRAR:
      if (millis()-inicioPrep >= 3000) { // Tras 3s, vuelve a esperar
        estadoActual = ESPERANDO;
        analogWrite(LED2,0);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Servicio");
      }
      break;

    case ADMIN:
      if (millis() - lastTimer >= 1000) { // Incrementa contador
        timer++;
        lastTimer = millis();
      }
      refresh(); // Refresca pantalla admin
      break;
  }
}
