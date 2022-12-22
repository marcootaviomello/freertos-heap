/*
Challenge

Using FreeRTOS, create two separate tasks. One listens for input over UART (from the Serial Monitor). 
Upon receiving a newline character (‘\n’), the task allocates a new section of heap memory 
(using pvPortMalloc()) and stores the string up to the newline character in that section of heap. 
It then notifies the second task that a message is ready.

The second task waits for notification from the first task. 
When it receives that notification, it prints the message in heap memory to the Serial Monitor. 
Finally, it deletes the allocated heap memory (using vPortFree()).

Recommended reading:

Memory in C - Stack, Heap, and Static: https://craftofcoding.wordpress.com/2015/12/07/memory-in-c-the-stack-the-heap-and-static/
The C Build Process: https://blog.feabhas.com/2012/06/the-c-build-process/
What and where are the stack and heap? https://stackoverflow.com/questions/79923/what-and-where-are-the-stack-and-heap/13326916#13326916
FreeRTOS Memory Management: https://www.freertos.org/a00111.html


*/

/* Macros to use*/
#define ZeroMemory(Dest) memset(Dest, 0, sizeof(Dest))
#define CopyMemory(Dest, Orig) memcpy(Dest, Orig, sizeof(Orig))
#define CopyString(Dest, Orig) memcpy(Dest, Orig, strlen(Orig) + 1)


/* Include libs from FreeRTOS */
#include  "freertos/FreeRTOS.h"
#include "freertos/task.h"


const uint8_t max_zize_buff = 255; /*Define o tamanho máximo da string que pode receber.*/
static char *msg_ptr = NULL; /*Cria um ponteiro que aponta para a mensagem */
uint8_t f_msg_rcvd = 0; /*flag para comunicação entre as threads*/

TaskHandle_t th_watch_serial; /*As tasks são gerenciadas pelos Handles, devemos criar um para cada task que criarmos.*/
TaskHandle_t th_send_serial; 

void watch_serial(void *parameter)
{

    char buf_rcvd[max_zize_buff];
    char c;
    uint8_t indice = 0;
    uint32_t memory_free = 0;

    /*Limpa o buffer iniciado*/
    ZeroMemory(buf_rcvd);

    for(;;)
    {
        while(Serial.available() > 0)
        {
            c = Serial.read();

            //verifica se a quantidade de dados entá dentro do limite para não estourar o buffer
            if (indice < (max_zize_buff-1))
            {
                buf_rcvd[indice] = c;
                indice++;
            }else
            {
                Serial.print("[TASK WATCH SERIAL] Estourou o indice!!!");
            }

            /*Verifica de o cactere atual é a '\n'*/
            if (c == '\n')
            {
                
                /*Subtitui o \n por o caractere de final de string \0 para a funcao de envio entender
                que é uma string.*/
                buf_rcvd[indice - 1] = '\0';

                if (f_msg_rcvd == 0) /*Verifica se a outra thread não está utilizando o flag*/
                {
                    memory_free = xPortGetFreeHeapSize();
                    
                    Serial.print("[TASK WATCH SERIAL] Quantidade de memoria disponivel antes de alocar: ");
                    Serial.println(xPortGetFreeHeapSize());

                    /*Se tem memoria  livre, pode alocar..*/
                    if (memory_free > indice)
                    {
                        msg_ptr = (char *)pvPortMalloc(indice * sizeof(char));
                        configASSERT(msg_ptr);
                        memcpy(msg_ptr, buf_rcvd, indice); /*copia o buffer recebido para a memoria alocada*/
                    }
                    Serial.print("[TASK WATCH SERIAL] Quantidade de memoria disponivel depois de alocar: ");
                    Serial.println(xPortGetFreeHeapSize());

                    f_msg_rcvd = 1; /*incida para segunda thread que recebeu um pacote válido.*/
                }
                                        
                ZeroMemory(buf_rcvd);
                indice = 0;
            }
            
        }

        /*É boa prática inserir um delay, mesmo que mínimo na task
        pois, o FreeRtos executa algumas funçoes básicas na task IDLE
        como por exemplo resetar o WTD, se não tem este delay aqui e 
        esta Task tem uma prioridade maior que a da task IDLE(sempre 
        tem a menor prioridade possível) O MCU vai resetar por não 
        atuar o WTD a tempo.*/
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

void send_serial(void *parameter)
{

    for(;;)
    {
        if(f_msg_rcvd == 1)
        {
            Serial.println(msg_ptr);

            // Give amount of free heap memory (uncomment if you'd like to see it)
            Serial.print("[TASK SEND SERIAL]: Quantidade de memoria disponivel antes de desalocar: ");
            Serial.println(xPortGetFreeHeapSize());

            vPortFree(msg_ptr);

            Serial.print("[TASK SEND SERIAL]: Quantidade de memoria disponivel depois de desalocar: ");
            Serial.println(xPortGetFreeHeapSize());

            msg_ptr = NULL;
            f_msg_rcvd = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }

}

void setup()
{

    Serial.begin(115200);
    xTaskCreatePinnedToCore(watch_serial, "WATCH_SERIAL", 1024, NULL, 1, &th_watch_serial, APP_CPU_NUM);
    xTaskCreatePinnedToCore(send_serial, "SEND_SERIAL", 1024, NULL, 1, &th_send_serial, APP_CPU_NUM);

    // Delete "setup and loop" task
    vTaskDelete(NULL);
}

void loop()
{
	//vTaskDelay(1000);
}
