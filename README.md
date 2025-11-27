# Practica 3 - Maquina-Expendedora (SETR)

---
## Descripción de la práctica

Para esta práctica debemos diseñar e implementar un controlador para una máquina expendedora basado en **Arduino UNO** y haciendo uso de los distintos sensores/actuadores que se nos proporcionan en el kit de la asignatura. La máquina debe funcionar con un sistema de estados (Arranque -> Servicio -> Admin), gestionando sensores en tiempo real, evitando bloqueos y usando de manera correcta interrupciones elementos no bloqueantes.

---
## Hardware Integrado

- Arduino UNO
- Pantalla LCD
- Joystick
- Sensor Temperatura/Humedad **DHT11**  
- Sensor Ultrasonidos HC-SR04  
- Botón  
- 2 LEDs (LED1, LED2)

---
## Funcionalidad del sistema

### **1.- Arranque del sistema**
- El LED1 va a parpadear 3 veces con intervalos de  1 segundo.
- LCD muestra el mensaje **"CARGANDO ..."**.
- Una vez ha arrancado se pasa al estado de servicio.
  ```c++
  case ARRANQUE:
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    ledOn = !ledOn;
    digitalWrite(LED1, ledOn);
    if (!ledOn) blinkCount++;
  }
  lcd.setCursor(0,0);
  lcd.print("CARGANDO...   ");
  if (blinkCount >= 3) {
    estadoActual = ESPERANDO;
    digitalWrite(LED1, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Servicio");
    lastService = millis();
  }
  break;

  ```

### **2.- Servicio**

#### **a) Modo espera del cliente**
- Medimos lass distancia mediante el utrasonidos.
- Si la distancia del cliente respecto a la máquian es de **mas de 1 metro**, entonces el LCD mostrará el mensaje **"ESPERANDO CLIENTE"**.
- Si la distancia del cliente respecto a la máquina es de **menos de 1 metro**, el programa pasa al menu.
  ```c++
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
  
  ```
#### **b) Menú y selección de productos**
1. LDC mostrar la temperatura y humedad actual durante 5 segundos.
2. Mostramos una lista de los prodcutos:
   - Café Solo — 1 €
   - Café Cortado — 1.10 €
   - Café Doble — 1.25 €
   - Café Premium — 1.50 €
   - Chocolate — 2.00 €
3. Podremos navegar por los productos haciendo uso del **joystick**
4. Selección con **botón del joystick**
```c++
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
```

#### **Preparación de bebida**
Durante la preparación de la bebdia el LCD va a mostrarnos un mensajes que diga **"Preparando Cafe...."**, este proceso durara un tiempo aleatorio entre los 4 y 8 segundos en los que el LD2 aumentará de intensidad de manera progresiva indicando el avance de la preparación. Una vez acabada la "preparación" el LCD mostrará por pantalla el mensaje **“RETIRE BEBIDA”** (3 segundos), y el programa volverá al estado Servicio.
```c++
float progreso = float(millis() - inicioPrep) / duracionPreparacion;
if (progreso > 1.0f) progreso = 1.0f;
float curva = pow(progreso, 1.6f); // Curva suavizada
analogWrite(LED2, int(curva*255));
```
#### **Reinicio del estado**
- Si el usuario pulsa el botón **entre 2 y 3 segundos**, se reinicia el estado Servicio.
```c++
if (tiempo >= 2000 && tiempo < 3000) botonReinicio();
```
### **3-. Modo Admin**

#### Acceso
- Mantener botón **≥ 5 segundos** para entrar en Admin.
- Ambos LEDs se encienden permanemente.
```c++
else if (tiempo >= 5000) botonAdmin(); // >5s entra admin
.
.
.
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

```

#### Menú Admin (navegable con joystick)
1. **Ver temperatura**
2. **Ver distancia sensor**
3. **Ver contador de tiempo**
4. **Modificar precios**
Ejemplo de navegacion admin con joystick
```c++
if (valX < UMBRAL_MIN && millis() - lastMove > 200) {
   if (opcion < NUM_OPC - 1) opcion++;
   lastMove = millis();
   adminMenu();
}
```
#### Detalle de cada sección
- **Temperatura/Humedad** → valores dinámicos del DHT11  
- **Distancia** → valores dinámicos del ultrasonido  
- **Contador** → tiempo total desde que la placa arrancó, incrementa en tiempo real  
- **Modificar precios**:
  - Selección de producto  
  - Subir/bajar precio en **0.05 €** (joystick arriba/abajo)  (en el codigo para hacerlo más sencillo usamos los precios en centimos)
  - Confirmar con botón del joystick  
  - Cancelar con movimiento **izquierda** del joystick  
  - Los precios **no persisten** tras reiniciar la placa

#### Salida del modo Admin
- Mantener botón **≥ 5 segundos** → volver al estado Servicio

---
## Técnicas y Librerias usadas

### Interrupciones hardware  
- Usamos las interrupciones para el boton físico normal, detectando pulsaciones cortas y largas
- Nos va a permitir reiniciar el servicio o entrar/salir del modo Admin sin bloquear el programa.
- Configurado con CHANGE nos va a permitir detectar los cambios entre HIGH y LOW
```c++
attachInterrupt(digitalPinToInterrupt(BUTTON), interruptBoton, CHANGE);
```

### Watchdog  
- Usamos el watchdog de 2s para **evitar bloqueos** del microcontrolador, este elemento va a ser reiniciado periodicamente con wdt_reset() en el loop().
```c++
wdt_enable(WDTO_2S); // Activa watchdog 2s
wdt_reset();          // Reset periódicamente
```
### Temporización (millis)
- Haciendo uso de millis() como lo hacemos en nuestro codigo evitamos el uso de delay() que lo que hará será bloquear nuestro sistema, lo que nos mantiene en un sistema reactivo para poder leer sensores y joystick de manera simultanea.
- Ejemplo: usamos el LED2 como una barra de progreso sin bloquear nada en nuestro sistema.
```c++
if (millis() - lastPWM >= 30) { // Actualiza LED2 como barra de progreso
   lastPWM = millis();
   float progreso = float(millis() - inicioPrep) / duracionPreparacion;
   if (progreso > 1.0f) progreso = 1.0f;
   float curva = pow(progreso, 1.6f); // Curva suavizada
   analogWrite(LED2, int(curva*255));
}
```

### Multitarea / Threads
- El uso de estos threads es imprescindible ya que nos va a permitir hacer lecturas independientes y periódicas, manteniendo LCD y LEDs actualizados sin interferencias. Tendremos threads en nuestro código para el ultrasonido, para el joystick y para el DHT
- Ejemplo:
```c++
threadUltrasonido.onRun(medirUltrasonido);
threadUltrasonido.setInterval(150);
controller.add(&threadUltrasonido);
```

### Otras librerías  
- LiquidCrystal  
- DHT-sensor-library  
- TimerOne  
- ArduinoThread

---
## PARTICULARIDADES, DETALLES DE IMPLEMENTACION Y DECISIONES DE DISEÑO

1. *Varianles inicializadas a -1*
   - Veremos a lo largo del código variables como **lastShown** indicandonos que no se ha mostrado ningún producto todavñia lo que nos permite actualizar el LCD solo si la selección cambia.
     
2. *Pines con salida PWN*
   - LED2 irá conectado a un pin PWM para simular una "barra de progreso" que se enciende gradualmente con la preparacion.
   - En el LED1 no vamos a usar un pin PWM porque solo necesitamos que se apague o encienda.
     
3. *Gestion de estado y variables de control*
   - La gestion de los estados y la transición entre ellos ha sido una de las cosas más complejas del programa, para facilitar la tarea se han usado variables como estadoActual o adminSubEstado que nos han ayudado a la transición entre los estados de nuestro programa. También se han usado variables como inicioPrep y duracionPreparacion que controlan los tiempos relativos de preparación de bebida.
     
4. *Anti-rebote y control de los eventos*
   - Usando lastMove, lastPressed, ultimoCambio vamos a evitar activaciones indeseadas y garantizar navegación estable de menús y lectura de botón.
     
5. *Precios y productos*
   - Los precios los hemos guardado en centimos para evitar posibles errores de cálculo con el float, además en el LCD lo mostraremos como euros diviendo la cifra entre 100.

6. *Mejoras posibles y errores*
   - Aunque el sistema en principio haya funcionado de manera corecta en su mayoria, hay un punto del sistema que no esta bien optimizado ya que los elementos del estado Admin, salvo el de cambiar los precios, presentan un bug visual, causado por el continuo reseteo de la pantalla que no se ha podido solucionar, y que no deja ver bien los elementos que se muestran en ese momento en el LCD.
---
## Explicación detallada de mi solución  
Mi proyecto funciona como una **máquina de estados**, donde se han separado de manera más o menos clara cada fase del sistema lo que nos va a permitir un control ordenado de este. Usaremos los **threads** para leer algunos de los sensores y actuadores sin bloquear el programa mientras que con una interrupción vamos a gestionar el botón. El haber añadido un watchdog nos asegura que en caso de que falle algo todo se reinicie.
Nuestras distintas funciones peqeuñas y específicas hace que se encapsulen bien las tareas lo que mejora la legibilidad del programa y nos facilita la reutilizacion de varios puntos en el programa.

---
## Esquema del circuito  
<img width="2968" height="1882" alt="MaquinaExpendedora_bb" src="https://github.com/user-attachments/assets/56d21cf1-fd5f-436d-ac9c-7d78e6f560f8" />

### Tabla de pines del circuito

| Componente | Señal / Función | Pin Arduino | Comentarios |
|-----------|----------------|-------------|-------------|
| **LCD 16x2 (modo 4 bits)** | RS | 12 | Control de registro |
| | EN | 11 | Enable del LCD |
| | D4 | 5 | Datos LCD |
| | D5 | 4 | Datos LCD |
| | D6 | A0 | Datos LCD |
| | D7 | A1 | Datos LCD |
| **Ultrasonido HC-SR04** | TRIG | 9 | Salida disparo |
| | ECHO | 8 | Entrada eco |
| **Joystick** | VRx | A3 | Movimiento horizontal |
| | VRy | A2 | Movimiento vertical |
| | Botón (SW) | 3 | Detecta pulsación LOW |
| **Botón Admin / Reset** | Botón principal | 2 | Interrupción INT0 |
| **LEDs** | LED1 | 7 | Indicador general |
| | LED2 | 6 | PWM, progreso bebida |
| **Sensor DHT11** | DATA | 10 | Entrada digital |

---
## VIDEO EXPLICATIVO

https://youtu.be/n0yBbpHk1j8
---


















