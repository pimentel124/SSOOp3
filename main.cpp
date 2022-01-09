/**
   Significado mensajes:
  TOC TOC → quiere entrar a comer
  |▄|    → se ha sentado a comer
  ¡o      → coge palillos izquierdo
  ¡o¡     → coge palillos derecho
  /o\ ÑAM → está comiendo
  ¡o_     → suelta palillos derecho
  _o      → suelta palillos izquierdo
  |_|     → sale de comer
*/

// Només fem servir el nucli app_cpu per simplicitat
// i tenint en compte que alguns esp32 són unicore
// unicore    -> app_cpu = 0
// 2 core  -> app_cpu = 1 (prog_cpu = 0)
#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif
//#define INCLUDE_vTaskSuspend    1    //ja està posat per defecte sino descomentar es el temps de espera                //infinit als semàfors

/*************************** Variables Globals i definicions **************************************/

#define NUM_OF_PHILOSOPHERS 5                         // Nombre de filòsofs
#define MAX_NUMBER_ALLOWED (NUM_OF_PHILOSOPHERS - 1)  // Màxim nombre de filòsofs a l'habitació  (un menys que el total per evitar deadlock)
#define ESPERA 200                                    // interval d'espera de vTaskDelay
#define PTP portTICK_PERIOD_MS
// Settings
enum { TASK_STACK_SIZE = 2048 };  // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t sem_param;  // Esperar a que se lean los parámetros
static SemaphoreHandle_t sem_done;   // Notifica cuando termina la tarea principal
static SemaphoreHandle_t palillos[NUM_OF_PHILOSOPHERS];

//*****************************************************************************
// Tareas

// Método para comer
void comer(void *param) {
    vTaskDelay(ESPERA / PTP);
    int num;
    char buffersalida[50];
    // 3. antes de decidirse a comer, todos los filósofos pasan
    // un tiempo aleatorio pensando entre 0 y ESPERA.
    int randomTime1 = random(0, ESPERA);
    int randomTime2 = random(0, ESPERA);
    int randomTime3 = random(0, ESPERA);

    // Se copia el parámetro que identifica a cada filósofo y se actualiza el semaforo
    num = *(int *)param;
    xSemaphoreGive(sem_param);

    // El filósofo i quiere entrar a comer
    sprintf(buffersalida, "Filósofo %i: TOC TOC", num);
    Serial.println(buffersalida);

    // El filósofo i se ha sentado a comer
    sprintf(buffersalida, "Filósofo %i: |▄|", num);
    Serial.println(buffersalida);

    // El filósofo i coge el palillos izquierdo
    xSemaphoreTake(palillos[num], portMAX_DELAY);
    sprintf(buffersalida, "Filósofo %i: ¡o", num);
    Serial.println(buffersalida);

    // 2. Cuando un filósofo ha cogido el palillo de la izquierda,
    // pasa un tiempo aleatorio pensando en sus cosas de entre 0 y ESPERA
    //(el tiempo de espera definido en el código) hasta coger el de su derecha.
    vTaskDelay(randomTime1 / PTP);

    // El filósofo i coge el palillo derecho
    xSemaphoreTake(palillos[(num + 1) % NUM_OF_PHILOSOPHERS], portMAX_DELAY);
    sprintf(buffersalida, "Filósofo %i: ¡o¡", num);
    Serial.println(buffersalida);

    // 3. Antes de decidirse a comer,
    // todos los filósofos pasan un tiempo aleatorio pensando entre 0 y ESPERA.
    vTaskDelay(randomTime2 / PTP);

    // El filósofo i come
    sprintf(buffersalida, "Filósofo %i: ÑAM", num);

    // 4. Los filósofos pasan un tiempo aleatorio (de entre 0 y ESPERA) comiendo.
    vTaskDelay(randomTime3 / PTP);
    Serial.println(buffersalida);

    // El filósofo i deja el palillos derecho
    xSemaphoreGive(palillos[(num + 1) % NUM_OF_PHILOSOPHERS]);
    sprintf(buffersalida, "Filósofo %i: ¡o_", num);
    Serial.println(buffersalida);

    // El filósofo i deja el palillos izquierdo
    xSemaphoreGive(palillos[num]);
    sprintf(buffersalida, "Filósofo %i: _o", num);
    Serial.println(buffersalida);

    // Notificar que ha acabado y eliminarse
    if (xSemaphoreGive(sem_done)) {
        sprintf(buffersalida, "Filósofo %i: |_|", num);
        Serial.println(buffersalida);
    }
    vTaskDelete(NULL);
}

// Main

void setup() {
    char tarea[20];
    // Configurar serial
    Serial.begin(9600);

    // Esperar un momento para no perder ningún dato
    vTaskDelay(1000 / PTP);
    Serial.println("------ Cena de filósofos ------");
    Serial.println("@ filósofo 0");
    Serial.println("@ filósofo 1");
    Serial.println("@ filósofo 2");
    Serial.println("@ filósofo 3");
    Serial.println("@ filósofo 4");

    // Crear los objetos del kernel antes de iniciar las tareas
    sem_param = xSemaphoreCreateBinary();
    sem_done = xSemaphoreCreateCounting(MAX_NUMBER_ALLOWED, 0);
    for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
        palillos[i] = xSemaphoreCreateMutex();
    }

    while (MAX_NUMBER_ALLOWED) {
        // Los filósofos empiezan a comer
        for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
            sprintf(tarea, "Philosopher %i", i);
            xTaskCreatePinnedToCore(comer,
                                    tarea,
                                    TASK_STACK_SIZE,
                                    (void *)&i,
                                    1,
                                    NULL,
                                    app_cpu);
            xSemaphoreTake(sem_param, portMAX_DELAY);
        }

        // Esperar hasta que todos los filósfos hayan comido
        for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
            xSemaphoreTake(sem_done, portMAX_DELAY);
        }
    }
    // Print para saber que no se ha producido deadlock en todo el programa
    Serial.println("\nNO ha habido deadlock, el programa ha finalizado!");
}

void loop() {
    // No hacer nada aquí
}
