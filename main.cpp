// Només fem servir el nucli app_cpu per simplicitat
// i tenint en compte que alguns esp32 són unicore
// unicore    -> app_cpu = 0
// 2 core  -> app_cpu = 1 (prog_cpu = 0)

#include <arduino.h>
// You'll likely need this on vanilla FreeRTOS
//include <semphr.h>

#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif
//#define INCLUDE_vTaskSuspend    1    //ja està posat per defecte sino descomentar es el temps de espera 		//infinit als semàfors

/*************************** Variables Globals i definicions **************************************/
                              //interval d'espera de vTaskDelay



#define NUM_FILOSOFOS 6                       //Nombre de filòsofs
#define MAX_FILOSOFOS (NUM_FILOSOFOS-1)  // Màxim nombre de filòsofs a l'habitació  (un menys que el total per evitar deadlock)
#define ESPERA 200                                  //interval d'espera de vTaskDelay


// Settings
enum { TASK_STACK_SIZE = 2048 };  // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;   // Wait for parameters to be read
static SemaphoreHandle_t done_sem;  // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_FILOSOFOS];

//*****************************************************************************
// Tasks

// The only task: eating
void eat(void *parameters) {

  int num;
  char buffer[50];
  int idx_1;
  int idx_2;

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Assign priority: pick up lower-numbered chopstick first
  if (num < (num+1) % NUM_FILOSOFOS) {
    idx_1 = num;
    idx_2 = (num+1) % NUM_FILOSOFOS;
  } else {
    idx_1 = (num+1) % NUM_FILOSOFOS;
    idx_2 = num;
  }

  // Take lower-numbered chopstick
  xSemaphoreTake(chopstick[idx_1], portMAX_DELAY);
  sprintf(buffer, "Philosopher %i took chopstick %i", num, num);
  Serial.println(buffer);

  // Add some delay to force deadlock
  vTaskDelay(1 / portTICK_PERIOD_MS);

  // Take higher-numbered chopstick
  xSemaphoreTake(chopstick[idx_2], portMAX_DELAY);
  sprintf(buffer, "Philosopher %i took chopstick %i", num, (num+1)%NUM_FILOSOFOS);
  Serial.println(buffer);

  // Do some eating
  sprintf(buffer, "Philosopher %i is eating", num);
  Serial.println(buffer);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Put down higher-numbered chopstick
  xSemaphoreGive(chopstick[idx_2]);
  sprintf(buffer, "Philosopher %i returned chopstick %i", num, (num+1)%NUM_FILOSOFOS);
  Serial.println(buffer);

  // Put down lower-numbered chopstick
  xSemaphoreGive(chopstick[idx_1]);
  sprintf(buffer, "Philosopher %i returned chopstick %i", num, num);
  Serial.println(buffer);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  char nombretarea[20];

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Dining Philosophers Hierarchy Solution---");

  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_FILOSOFOS, 0);
  for (int i = 0; i < NUM_FILOSOFOS; i++  ) {
    chopstick[i] = xSemaphoreCreateMutex();
    Serial.println("Semaforo creado");
  }

  // Have the philosphers start eating
  for (int i = 0; i < NUM_FILOSOFOS; i++ ) {
    sprintf(nombretarea, "Philosopher %i", i);
    xTaskCreatePinnedToCore(eat,
                            nombretarea,
                            TASK_STACK_SIZE,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }


  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_FILOSOFOS; i++ ) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  // Say that we made it through without deadlock
  Serial.println("Todos los filófos han terminado");
}

void loop() {
  // Do nothing in this task
}