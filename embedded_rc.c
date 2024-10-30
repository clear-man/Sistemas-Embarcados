#include "BluetoothSerial.h"
#include "ESP32Servo.h"
//Servo myservo;

//O "if" a seguir serve para testar a conexão com o bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

#define echoPin 17
#define trigPin 16
#define buzzer 2
#define controle_de_sentido1 22
#define controle_de_sentido2 23
#define controle_de_direcao1 27
#define controle_de_direcao2 32
#define enB 21

int distance;
unsigned long duration;
int teste;
SemaphoreHandle_t xSerialSemaphore;

char lastDirectionCommand = 'Q'; // Stop
char lastMovementCommand = 'Q'; // Stop
char lastBuzzerCommand = 'Q'; // Stop

void vTarefa1(void *pvParameters); // controle de direção
void vTarefa2(void *pvParameters); // controle de sentido
void vTarefa3(void *pvParameters); // controle de distancia
void vTarefa4(void *pvParameters); // controle de buzina
void vTarefaBluetooth(void *pvParameters); // Tarefa para ler comandos Bluetooth

void setupHardware();
void createTasks();

void setup() {
  Serial.begin(115200);
  SerialBT.begin("RedesESP32"); // Nome do dispositivo Bluetooth

  setupHardware();
  createTasks();

  //Serial.println("Setup completed");
}

void loop() {
  // O loop principal fica vazio, as tarefas são gerenciadas pelo FreeRTOS
}

void setupHardware() {
  pinMode(controle_de_sentido1, OUTPUT);
  pinMode(controle_de_sentido2, OUTPUT);
  pinMode(controle_de_direcao1, OUTPUT);
  pinMode(controle_de_direcao2, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(enB, OUTPUT);
  analogWrite(enB, 90);

  xSerialSemaphore = xSemaphoreCreateMutex();
  if (xSerialSemaphore == NULL) {
    //Serial.println("Erro ao criar o semáforo");
    while (1);
  }
}

void createTasks() {
  BaseType_t result;

  result = xTaskCreate(vTarefa1, "Tarefa1", 1024, NULL, 1, NULL);
  if (result != pdPASS) {
    //Serial.println("Erro ao criar Tarefa1");
    while (1);
  }

  result = xTaskCreate(vTarefa2, "Tarefa2", 1024, NULL, 1, NULL);
  if (result != pdPASS) {
    //Serial.println("Erro ao criar Tarefa2");
    while (1);
  }

  result = xTaskCreate(vTarefa3, "Tarefa3", 1024, NULL, 1, NULL);
  if (result != pdPASS) {
    //Serial.println("Erro ao criar Tarefa3");
    while (1);
  }

  result = xTaskCreate(vTarefa4, "Tarefa4", 1024, NULL, 1, NULL);
  if (result != pdPASS) {
   
    while (1);
  }
  
  result = xTaskCreate(vTarefaBluetooth, "TarefaBluetooth", 1024, NULL, 1, NULL);
  if (result != pdPASS) {
    
    while (1);
  }
}

//tarefa do servo motor direcao
void vTarefa1(void *pvParameters) {
  while (1) {
   if (lastDirectionCommand == 'L'){
      digitalWrite(controle_de_direcao1, HIGH);
      digitalWrite(controle_de_direcao2, LOW);
  
    } 
    else if (lastDirectionCommand == 'R') {
      digitalWrite(controle_de_direcao1, LOW);
      digitalWrite(controle_de_direcao2, HIGH);
    }
    else{
      digitalWrite(controle_de_direcao1, HIGH);
      digitalWrite(controle_de_direcao2, HIGH);

    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//tarefa para sentido ponte H
void vTarefa2(void *pvParameters) {
  while (1) {
    if (lastMovementCommand == 'S'){
      digitalWrite(controle_de_sentido1, HIGH);
      digitalWrite(controle_de_sentido2, LOW);
    } 
    else if (lastMovementCommand == 'X') {
      digitalWrite(controle_de_sentido1, LOW);
      digitalWrite(controle_de_sentido2, HIGH);
    }
    else{
      digitalWrite(controle_de_sentido1, HIGH);
      digitalWrite(controle_de_sentido2, HIGH);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
//tarefa para o sonar:

void vTarefa3(void *pvParameters) {
  while (1) {
    digitalWrite(trigPin, LOW);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    digitalWrite(trigPin, HIGH);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration / 58.2;
    teste = distance;
   // Serial.println(distance);

    if (distance <= 10 && distance > 0) {
      lastBuzzerCommand = 'H'; // Aciona o buzzer
      //digitalWrite(led, HIGH);
      
    } 
    else {
      lastBuzzerCommand = 'M'; // Desliga o buzzer
      //digitalWrite(led, LOW);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

//tarefa para condição de parametros do sonar e ver se o buzzer liga
void vTarefa4(void *pvParameters) {
  while (1) {
    if (lastBuzzerCommand == 'H') {
      digitalWrite(buzzer, HIGH); 
    } 
    else {
      digitalWrite(buzzer, LOW);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void vTarefaBluetooth(void *pvParameters) {
  while (1) {
    char recebido;
    if (SerialBT.available()) {
      if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 10) == pdTRUE) {
        recebido = SerialBT.read();
        

        if (recebido == 'R' || recebido == 'L') {
          lastDirectionCommand = recebido; // Atualiza o último comando de direção
        } 

        else if (recebido == 'S' || recebido == 'X') {
          
          lastMovementCommand = recebido; // Atualiza o último comando de movimento
        }
        else{
          lastMovementCommand = recebido;
          lastDirectionCommand = recebido;
        }

        xSemaphoreGive(xSerialSemaphore);
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
