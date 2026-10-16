# STM32 FreeRTOS IoT Web Server

Nodo sensor IoT (*Edge Node*) basado en STM32L475 (ARM Cortex-M4) que ejecuta el sistema operativo en tiempo real **FreeRTOS** para gestionar de forma concurrente la captura de variables ambientales, el procesamiento de datos inerciales y la comunicación en red. El sistema levanta un servidor TCP/IP sobre una interfaz Wi-Fi para exponer la telemetría en tiempo real mediante una API RESTful, devolviendo cargas útiles en formato JSON.

---

## Estructura del Repositorio

La arquitectura del proyecto separa estrictamente la abstracción del hardware, el sistema operativo y la lógica de aplicación en tareas independientes:

├── BSP/                               \# Board Support Packages: Drivers de nivel de placa

│   ├── stm32l475e\_iot01\_\*.c/.h        \# APIs para giroscopio, humedad, temperatura, QSPI, etc.

│   └── (Otras librerías de sensores dependientes de placa)

│

├── Components/                        \# Drivers de bajo nivel de los ICs de los sensores

│   ├── hts221/                        \# Sensor de Temperatura y Humedad

│   ├── lis3mdl/                       \# Magnetómetro 3D

│   ├── lps22hb/                       \# Sensor de Presión Barométrica absoluto

│   └── lsm6dsl/                       \# Acelerómetro y Giroscopio 3D MEMS

│

├── LibWIFI/                           \# Abstracción de red y módulo de comunicaciones TCP/IP

│

├── Middlewares/

│   └── Third\_Party/

│       └── FreeRTOS/                  \# Kernel del Sistema Operativo en Tiempo Real

│

└── Core/

    ├── Inc/

    │   ├── FreeRTOSConfig.h           \# Ticks, Heap, Mutex y configuración del Scheduler

    │   └── Sensores.h                 \# Estructuras de datos de telemetría

    └── Src/

        ├── Tareas.c                   \# Definición de hilos (Threads), Colas y Semáforos

        ├── Sensores.c                 \# Lógica de interrogación a sensores I2C/SPI

        ├── web\_server.c               \# Máquina de estados TCP y formateo HTTP/JSON

        ├── printf.c                   \# Redirección de syscalls (\_io\_putchar) a UART1

        ├── main.c                     \# Punto de entrada e inicialización de RTOS

        └── stm32l4xx\_it.c             \# Rutinas de interrupción (ISRs) de periféricos

---

## Arquitectura de Software y RTOS

El proyecto está orquestado por **FreeRTOS**, configurado con el API `CMSIS_V2`. Se han implementado mecanismos avanzados de sincronización y comunicación Inter-Process Communication (IPC) para mantener el sistema determinista y evitar condiciones de carrera (race conditions):

1. **Gestión de Interrupciones (ISR vs Tasks):**  
     
   - El sistema emplea un enfoque asíncrono para las comunicaciones. Las interrupciones hardware (como `EXTI1_IRQHandler` para la antena WiFi o interrupciones UART) ceden rápidamente el control y despiertan a las tareas correspondientes mediante el mecanismo `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`.  
   - Se utiliza un **Semáforo Binario (`xSemaphore`)** para sincronizar la transmisión de la UART, asegurando que la redirección de `printf()` no bloquee la CPU con esperas activas mientras el periférico de hardware transfiere datos.

   

2. **Tareas Principales (Threads):**  
     
   * **`TareaSensores` (128 words, Prioridad 1):** Permanece bloqueada en estado `Blocked` sin consumir CPU (`ulTaskNotifyTake`) hasta que recibe un *Task Notification*. Al despertar, sondea los sensores inerciales y ambientales a través de los BSPs, agrupa los valores en un struct y los envía a la cola de datos.  
   * **`TareaLED` (128 words, Prioridad 1):** Tarea reactiva bloqueada a la espera en una cola (`xQueueReceive`). Cuando recibe un evento binario, cambia el estado del GPIO físico del LED.  
   * **`TareaWebServer` (256 words, Prioridad 1):** Alberga la lógica de red pesada. Instancia el socket, negocia la conexión TCP/IP vía la librería `es_wifi` y procesa peticiones HTTP entrantes.

   

2. **Colas de Mensajes (Queues) para Telemetría:** Se utilizan colas en lugar de variables globales para pasar datos de forma segura entre tareas, lo que proporciona exclusión mutua de serie:  
     
   - `xQueueSensors`: Pasa estructuras complejas (temperatura, presión, humedad, aceleración en 3 ejes, etc.) desde la capa de adquisición hacia el servidor web.  
   - `xQueueLED`: Pasa instrucciones asíncronas desde el intérprete HTTP (cuando recibe peticiones POST) hacia el controlador físico del LED.

---

## Características de Red y Sensores

* **Adquisición I2C Multisensor:** Integración de la capa HAL y BSP para leer un ecosistema completo de sensores integrados: `HTS221` (Humedad), `LPS22HB` (Presión), `LSM6DSL` (Acelerómetro y Giroscopio) y `LIS3MDL` (Magnetómetro).  
* **Conectividad Wi-Fi WPA2:** El módulo se conecta a una red WLAN configurada, obteniendo una dirección IP por DHCP.  
* **Servidor Web y API JSON:** Implementa un listener TCP en el puerto 80 capaz de parsear cabeceras HTTP.  
  - **Peticiones `GET`:** Invoca a las tareas de sensado asíncronas y devuelve un payload puro de tipo `application/json` con los 12 valores de la telemetría listos para ser consumidos por cuadros de mando en la nube.  
  - **Peticiones `POST`:** Analiza comandos incrustados (e.g., `radio=0`, `radio=1`) inyectándolos en el scheduler del RTOS para manipular los relés/LEDs físicos del sistema empotrado desde un navegador externo.

---

## Requisitos de Hardware y Compilación

* **Hardware:** Kit de descubrimiento B-L475E-IOT01A equipado con el módulo Wi-Fi Inventek ISM43362.  
* **Configuración del Proyecto:**  
  - Habilitar el compilador GCC ARM (versión de coma flotante por hardware: `-mfloat-abi=hard`).  
  - Activar la bandera del enlazador `Use float with printf from newlib-nano` (`-u_printf_float`) necesaria para el formato de cadenas que contienen temperatura y aceleración.  
* Antes de la compilación, editar las macros `SSID` y `PASSWORD` en el archivo `web_server.c` con las credenciales de la red inalámbrica objetivo.