

#include "def.h"
#include "sys.h"
#include "can_list.h"
#include "lib_str.h"
#include "lib_can.h"
#include "lib_uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp_timer.h"
//#include "esp_task_wdt.h"

void thread_init();

static int64_t sys_tm = 0;
static int64_t sys_tm_second;

//------------------------------------
void main_event()
{
    sys_tm = esp_timer_get_time()/1000;
    com_event();
    if (ABS(sys_tm_second,sys_tm) >= 1000) {
        sys_tm_second = sys_tm;
    }
    //vTaskDelay(2/portTICK_RATE_MS);
}

void app_main()
{
    sys_uart_open();
    SysList_TxInit();
    SysList_RxInit();
    Sys_Init();
    if (!esp32_can_open()) {
        printf("err-> esp32_can_open \n");
    }
    //if (esp_timer_init() != ESP_OK) {
    //    printf("err-> esp_timer_init \n");
    //}

    thread_init();
    //main_event();
}


//------------------------------------
static struct CAN_DATA tx_can;
static struct CAN_DATA rx_can;
void task_delay(int ms)
{
    vTaskDelay(ms/portTICK_RATE_MS);
}

void task_tx_msg(void *arg)
{
    while(true) {
        tx_process(&tx_can,sys_tm);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void task_rx_msg(void *arg)
{
    while(true) {
        if (esp32_can_rx_msg(&rx_can)) {
            rx_process(&rx_can,sys_tm);
        }
        monitor_ss_process(sys_tm);
    }
}

void task_main(void *arg)
{
    while(true) {
        main_event();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}


void thread_init()
{
    printf("info-> thread_init");
    //protocomm_console_stop();

    xTaskCreate(task_tx_msg, "task_tx_msg", 4096, NULL, 5, NULL);
    xTaskCreate(task_rx_msg, "task_rx_msg", 4096, NULL, 7, NULL);
    xTaskCreate(task_main, "task_main", 4096, NULL, 3, NULL);
}



