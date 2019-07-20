

#include <Arduino.h>
#include "def.h"
//#include "lib.h"
#include "lib_str.c"
#include "lib_can.c"
#include "list_can.c"

//-------------------------------------------------------
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "unistd.h"

static void com_process();
static void output_can_tx(struct CAN_DATA *can);
static void output_can_tx_all();
static void output_can_rx_all();
static int buf_get_can_id(char *buf);

void thread_init();


//-------------------------------------------------------
bool flag_can_open;
bool flag_monitor_all;
bool flag_monitor_ss;

#define COM_BUF_LEN 300
#define COM_CMD_MAX (COM_BUF_LEN - 50)
int com_in=0;
char com_buf[COM_BUF_LEN + 1];

void com_buf_del(int num)
{
    //Serial.println(" com_in " + String(com_in,HEX));
    //Serial.println(" com_buf_del " + String(num,HEX));
    if (num >= com_in) {
      com_in = 0;
      com_buf[0] = 0;
    }
    else if (num > 0) {
        for (int i=0;i<com_in-num;i++) {
            com_buf[i] = com_buf[i+num];
        }
        com_in -= num;
        com_buf[com_in] = 0;
    }
}

__inline void flag_monitor_init()
{
	flag_monitor_all = false;
	flag_monitor_ss = false;
}

__inline void buf_clear()
{
	SysList_TxInit();
	SysList_RxInit();
}

__inline void output_version()
{
  if (flag_can_open) {
    Serial.println("------");
    Serial.println(VERSION);
  }
}


//-------------------------------------------------------
void setup() {
  flag_monitor_init();
  buf_clear();
  Serial.begin(UART_SPEED);
  flag_can_open = esp32_can_open();
  if (!flag_can_open) {
    Serial.println("???-> can't open can dev");
  }
  output_version();
  thread_init();
}

//-------------------------------------------------------
unsigned long time_tag_cur = 0;
unsigned long time_tag_100ms = 0;
unsigned long time_tag_1s = 0;
unsigned long time_tag_monitor_ss = 0;
void loop() {
  time_tag_cur = millis();
  analyze_com_buf();
  Serial.event();

  if (time_tag_cur - time_tag_monitor_ss >= 50) {
    time_tag_monitor_ss = time_tag_cur;
    monitor_ss_event();
  }
  //100ms
  if (time_tag_cur - time_tag_100ms >= 100) {
    time_tag_100ms = time_tag_cur;
  }
  //1s
  if (time_tag_cur - time_tag_1s >= 1000) {
    time_tag_1s = time_tag_cur;
  }

  vTaskDelay(2/portTICK_RATE_MS);
}


//-------------------------------------------------------
#define CMD_MAX_LEN 100
char tmp_cmd_str[CMD_MAX_LEN];
void analyze_com_buf()
{
	int index;
	while (com_in < COM_CMD_MAX && Serial.available()) {
		char tmp = Serial.read();
		if (tmp != 0) {
			com_buf[com_in] = tmp;
			com_in++;
      //Serial.println(" com_in " + String(com_in,HEX) + " " + String(tmp,HEX));
		}
	}
	com_buf[com_in] = 0;
	
	index = index_of_str(com_buf,"???");
	if (index >= 0) {
		output_version();
		com_buf_del(index+3);
	}

    index = index_of_str(com_buf,"can");
    if (index >= 0)
    {
        com_buf_del(index);
        index = index_of_char(com_buf,'&');
        if (index > 0) {
            com_process();
            com_buf_del(index+1);
        }
        else {
          if (com_in > CMD_MAX_LEN) {
            com_buf_del(com_in - 3);
          }
        }
    }
    else {
        index = index_of_char(com_buf,'&');
        if (index >= 0) {
          com_buf_del(index+1);
        }
        else {
          com_buf_del(com_in - 3);
        }
    }
}

static void com_process()
{
    char *lp_cmd = &com_buf[4];
    struct CAN_DATA can;
    
    if (is_str_same(lp_cmd,"msg")) {
      int ret = buf_to_can_data(lp_cmd + 4,&can);
      if (ret >= 0) {
        output_can_tx(&can);
        can.tm /= 10;
        can.tm *= 10;
        SysList_TxAdd(&can);
      } else {
        Serial.println("err-> tx msg " + ret);
      }
    }

    if (is_str_same(lp_cmd,"del tx all")) {
      //Serial.println("clear tx buf");
      SysList_TxInit();
    }
    else if (is_str_same(lp_cmd,"del tx")) {
      int id = buf_get_can_id(&lp_cmd[7]);
      //Serial.println("del tx id = " + String(id,HEX));
      SysList_TxDel_Id(id);
    }

    if (is_str_same(lp_cmd,"monitor all")) {
      flag_monitor_init();
      flag_monitor_all = true;
      SysList_RxInit();
    }
    else if (is_str_same(lp_cmd,"unmonitor all")) {
      flag_monitor_init();
      SysList_RxInit();
    }
    else if (is_str_same(lp_cmd,"monitor ss")) {
      flag_monitor_init();
      flag_monitor_ss = true;
      SysList_RxInit();
    }
    else if (is_str_same(lp_cmd,"monitor")) {
      can.id = buf_get_can_id(&lp_cmd[8]);
      //Serial.println("monitor id = " + String(id,HEX));
      if (flag_monitor_all || flag_monitor_ss) {
        SysList_RxInit();
      }
      SysList_RxAdd(&can);
      flag_monitor_init();
    }

    if (is_str_same(lp_cmd,"list tx")) {
      output_can_tx_all();
    }
    if (is_str_same(lp_cmd,"list rx")) {
      output_can_rx_all();
    }
    if (is_str_same(lp_cmd,"reset")) {
      flag_monitor_init();
      SysList_RxInit();
      SysList_TxInit();
    }
}


//-------------------------------------------------------

static int ss_index = 0;;
void monitor_ss_event()
{
    struct CAN_DATA can;
    int num;
    if (!flag_monitor_ss) {
        return;
    }
    ss_index++;
    if (ss_index >= CAN_LIST_LEN) {
        ss_index = 0;
    }
    if (SysList_RxIsActive(ss_index)) {
        num = SysList_RxRead(ss_index,&can);
        SysList_RxDel(ss_index);
        can.tm = num;
        output_can_monitor(&can,num);
    }
}


//-------------------------------------
__inline void output_can_info(char *head,struct CAN_DATA *can)
{
    char tmp_str[CMD_MAX_LEN];
    can_data_to_buf(tmp_str,can);
    Serial.print(head);
    Serial.print(" ");
    Serial.println(tmp_str);
}

static void output_can_tx(struct CAN_DATA *can)
{
    output_can_info("can_tx",can);
}

static void output_can_tx_all()
{
    struct CAN_DATA can;
    Serial.println("--- output tx list---");
    for (int i=0;i<CAN_LIST_LEN;i++)
    {
        if (SysList_TxIsActive(i)) {
            SysList_TxRead(i,&can);
            output_can_tx(&can);
        }
    }
}

void output_can_rx(struct CAN_DATA *can)
{
    output_can_info("can_rx",can);
}

static void output_can_rx_all()
{
    struct CAN_DATA can;
    Serial.println("--- output rx list---");
    for (int i=0;i<CAN_LIST_LEN;i++)
    {
        if (SysList_RxIsActive(i)) {
            SysList_RxRead(i,&can);
            output_can_rx(&can);
        }
    }
}

void output_can_monitor(struct CAN_DATA *can,int num)
{
    output_can_info("can_ss",can);
}

static int buf_get_can_id(char *buf)
{
    unsigned long tmp = hex_buf_to_long(buf);
    return long_to_int(tmp);
}


//-------------------------------------
void tx_process(struct CAN_DATA *pCan,long tm)
{
    struct CAN_DATA can;
    long tag;
    for (int i=0;i<CAN_LIST_LEN;i++) {
        if (SysList_TxIsActive(i)) {
            tag = SysList_TxRead(i,&can);
            if (can.tm == 0) {
                esp32_can_tx_msg(&can);
                //output_can_tx(&can);
                SysList_TxDel(i);
            }
            else if (ABS(tm,tag) > can.tm) {
                esp32_can_tx_msg(&can);
            }
        }
    }
}

void rx_process(struct CAN_DATA *pCan,long tm)
{
    struct CAN_DATA can;
    pCan->tm = tm;
    if (flag_monitor_all) {
        output_can_rx(pCan);
    }
    else if (flag_monitor_ss) {
        SysList_RxAdd(pCan);
    }
    else {
        for (int i=0;i<CAN_LIST_LEN;i++) {
            if (SysList_RxIsActive(i)) {
                SysList_RxRead(i,&can);
                if (pCan->id == can.id) {
                    output_can_rx(pCan);
                }
            }
        }
    }
}


//-------------------------------------------------
struct CAN_DATA tx_can;
struct CAN_DATA rx_can;
//SemaphoreHandle_t xSemaphore_Msg = NULL;
void task_tx_msg(void *arg)
{
	while(true) {
		tx_process(&tx_can,time_tag_cur);
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void task_rx_msg(void *arg)
{
	while(true) {
    if (esp32_can_rx_msg(&rx_can)) {
      rx_process(&rx_can,time_tag_cur);
    }
		//else {
		//  vTaskDelay(2/portTICK_RATE_MS);
		//}
	}
}

void thread_init()
{
  Serial.println("info-> thread_init");
  //xSemaphore_Msg = xSemaphoreCreateMutex();
  xTaskCreate(task_tx_msg, "task_tx_msg", 4096, NULL, 2, NULL);
  xTaskCreate(task_rx_msg, "task_rx_msg", 4096, NULL, 3, NULL);
}
