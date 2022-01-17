//------------------------------
// Grupo: Goonies
// Autores: Álvaro Pimentel, Andreu Marqués Valerià, Gregori Serra
//---------------------------------

// Només fem servir el nucli app_cpu per simplicitat
// i tenint en compte que alguns esp32 són unicore
// unicore    -> app_cpu = 0
// 2 core  -> app_cpu = 1 (prog_cpu = 0)
//#include <Arduino.h>  //quitar el comentario si se quiere editar en c++, en lugar de en arduino

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif
//#define INCLUDE_vTaskSuspend    1    //ja està posat per defecte sino descomentar es el temps de espera

/*************************** Variables Globals i definicions **************************************/

#define NUM_OF_PHILOSOPHERS 5                         // Nombre de filòsofs
#define MAX_NUMBER_ALLOWED (NUM_OF_PHILOSOPHERS - 1)  // Màxim nombre de filòsofs a l'habitació  (un menys que el total per evitar deadlock)
#define ESPERA 200                                    // interval d'espera de vTaskDelay
#define INFINITE 1                                    // Si INFINITE = 0, solo comen una vez

// These define how many bytes and how many ticks per millisecond respectively should be allocated for each stack of tasks waiting on input from the user.

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
    char buff[60];  // Buffer que se empleará para imprimir todos los mensajes enviados y recividos del serial port

    // 3. antes de decidirse a comer, todos los filósofos pasan
    // un tiempo aleatorio pensando entre 0 y ESPERA.
    // Se lee el valor analogico del pin A0 para que cada vez que se generan los numeros aleatorios estos tengan diferente semilla
    randomSeed(analogRead(A0));
    int randomTime1 = random(0, ESPERA);
    int randomTime2 = random(0, ESPERA);
    int randomTime3 = random(0, ESPERA);

    // Se copia el parámetro que identifica a cada filósofo y se actualiza el semaforo
    num = *(int *)param;

    xSemaphoreGive(sem_param);

    // El filósofo i quiere entrar a comer
    sprintf(buff, "Filósofo %i: Quiere comer", num);
    Serial.println(buff);

    // El filósofo i se ha sentado a comer
    sprintf(buff, "Filósofo %i: Se sienta", num);
    Serial.println(buff);

    // El filósofo i coge el palillos izquierdo
    xSemaphoreTake(palillos[num], portMAX_DELAY);
    sprintf(buff, "Filósofo %i: Coge palillo IZQ", num);
    Serial.println(buff);

    // 2. Cuando un filósofo ha cogido el palillo de la izquierda,
    // pasa un tiempo aleatorio pensando en sus cosas de entre 0 y ESPERA
    //(el tiempo de espera definido en el código) hasta coger el de su derecha.
    vTaskDelay(randomTime1 / PTP);

    // El filósofo i coge el palillo derecho
    xSemaphoreTake(palillos[(num + 1) % NUM_OF_PHILOSOPHERS], portMAX_DELAY);
    sprintf(buff, "Filósofo %i: Coge palillo DER", num);
    Serial.println(buff);

    // 3. Antes de decidirse a comer,
    // todos los filósofos pasan un tiempo aleatorio pensando entre 0 y ESPERA.
    vTaskDelay(randomTime2 / PTP);

    // El filósofo i come
    sprintf(buff, "Filósofo %i: COME", num);
    Serial.println(buff);
    // 4. Los filósofos pasan un tiempo aleatorio (de entre 0 y ESPERA) comiendo.

    vTaskDelay(randomTime3 / PTP);

    // El filósofo i deja el palillos derecho
    xSemaphoreGive(palillos[(num + 1) % NUM_OF_PHILOSOPHERS]);
    sprintf(buff, "Filósofo %i: Suelta palillo DER", num);
    Serial.println(buff);

    // El filósofo i deja el palillos izquierdo
    xSemaphoreGive(palillos[num]);
    sprintf(buff, "Filósofo %i: Suelta palillo IZQ", num);
    Serial.println(buff);

    // Notificar que ha acabado y eliminarse
    if (xSemaphoreGive(sem_done)) {
        sprintf(buff, "Filósofo %i: Se levanta de la mesa", num);
        Serial.println(buff);
    }
    vTaskDelete(NULL);
}

// Main

void setup() {
    char tarea[30];
    // Configurar serial
    Serial.begin(9600);

    // Esperar un momento para no perder ningún dato
    vTaskDelay(1000 / PTP);
    Serial.println("------ Cena de filósofos ------");

    // Mensaje de llegada de los filósofos
    for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
        sprintf(tarea, "Llega filósofo: %i ",i);
        Serial.println(tarea);
    }

    // Crear los objetos del kernel antes de iniciar las tareas
    sem_param = xSemaphoreCreateBinary();                        // Semáforo binario para si se ha leído el numero de filósofo o no
    sem_done = xSemaphoreCreateCounting(MAX_NUMBER_ALLOWED, 0);  // Semaforo contador para gestionar los fiósofos
    for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
        palillos[i] = xSemaphoreCreateMutex();  // Semáforo mutex para gestionar el control de los palillos
    }

#if INFINITE
    while (MAX_NUMBER_ALLOWED) {
#endif
        // Los filósofos empiezan a comer
        for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++) {
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
#if INFINITE
    }
#endif
    // Print para saber que no se ha producido deadlock en todo el programa
    Serial.println("\nTodos los filósfos han terminado de comer, no ha ocurrido deadlock");
}

void loop() {
    // No es necesario poner nada
}