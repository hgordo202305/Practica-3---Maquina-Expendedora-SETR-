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

### **2.- Servicio**

#### **a) Modo espera del cliente**
- Medimos lass distancia mediante el utrasonidos.
- Si la distancia del cliente respecto a la máquian es de **mas de 1 metro**, entonces el LCD mostrará el mensaje **"ESPERANDO CLIENTE"**.
- Si la distancia del cliente respecto a la máquina es de **menos de 1 metro**, el programa pasa al menu.
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

#### **Preparación de bebida**
Durante la preparación de la bebdia el LCD va a mostrarnos un mensajes que diga **"Preparando X...."** siendo X la bebida que hemos seleccionado, este proceso durara un tiempo aleatorio entre los 4 y 8 segundos en los que el LD2 aumentará de intensidad de manera progresiva indicando el avance de la preparación. Una vez acabada la "preparación" el LCD mostrará por pantalla el mensaje **“RETIRE BEBIDA”** (3 segundos), y el programa volverá al estado Servicio.

#### **Reinicio del estado**
- Si el usuario pulsa el botón **entre 2 y 3 segundos**, se reinicia el estado Servicio.

### **3-. Modo Admin**

#### Acceso
- Mantener botón **≥ 5 segundos** para entrar en Admin.
- Ambos LEDs se encienden permanemente.

#### Menú Admin (navegable con joystick)
1. **Ver temperatura**
2. **Ver distancia sensor**
3. **Ver contador de tiempo**
4. **Modificar precios**

#### Detalle de cada sección
- **Temperatura/Humedad** → valores dinámicos del DHT11  
- **Distancia** → valores dinámicos del ultrasonido  
- **Contador** → tiempo total desde que la placa arrancó, incrementa en tiempo real  
- **Modificar precios**:
  - Selección de producto  
  - Subir/bajar precio en **0.05 €** (joystick arriba/abajo)  
  - Confirmar con botón del joystick  
  - Cancelar con movimiento **izquierda** del joystick  
  - Los precios **no persisten** tras reiniciar la placa

#### Salida del modo Admin
- Mantener botón **≥ 5 segundos** → volver al estado Servicio

---
## Técnicas y Librerias usadas

### Interrupciones hardware  
Falta completar: (explicar uso y mostrar una snippet donde se muestre un ejemplo de uso del código)

### Watchdog  
Falta completar: (explicar uso y mostrar una snippet donde se muestre un ejemplo de uso del código)

### Temporización (millis / TimerOne)  
Falta completar: (explicar uso y mostrar una snippet donde se muestre un ejemplo de uso del código) _Explica cómo evitas `delay()` y por qué es importante para tiempo real._

### Multitarea / Threads (si aplica)  
Falta completar: (explicar uso y mostrar una snippet donde se muestre un ejemplo de uso del código)

### Otras librerías  
- LiquidCrystal  
- DHT-sensor-library  
- TimerOne  
- ArduinoThread (opcional)

---

## Explicación detallada de mi solución  
Falta completar: _Aquí explicas tu arquitectura, organización del código, cómo gestionas los estados, etc._

---

## Esquema del circuito  
<img width="2968" height="1882" alt="MaquinaExpendedora_bb" src="https://github.com/user-attachments/assets/56d21cf1-fd5f-436d-ac9c-7d78e6f560f8" />
 



















