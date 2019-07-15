

#include <Arduino.h>
#include "def.h"
#include "lib_str.h"


//-------------------------------------------------------
#define ARDUINO_BOARD 0
#define ESP32_BOARD (!ARDUINO_BOARD)

const String key_version = "CAN_VER 1.7 ";

#define UART_SPEED 115200
#define FLAG_MSG_FULL true

#if ARDUINO_BOARD
#define BOARD_NAME "arduino"
#include "can_arduino.h"
#define packing_can_open    arduino_can_init
#define packing_can_close   
#define packing_can_send    arduino_can_send
#define packing_can_receive arduino_can_get
#endif

#if ESP32_BOARD
#define BOARD_NAME "esp32"
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "unistd.h"
#include "can_esp32.h"
#define packing_can_open    esp32_can_open
#define packing_can_close   esp32_can_close
#define packing_can_send    esp32_can_send_msg
#define packing_can_receive esp32_can_read_msg
#endif


//-------------------------------------------------------
bool flag_can_open;
bool flag_monitor_all;
bool flag_monitor_ss;

#define COM_BUF_LEN 200
char com_index=0;
char com_buf[COM_BUF_LEN + 1];

#define CAN_BUF_LEN 20
struct SYS_CAN_DATA can_tx_buf[CAN_BUF_LEN];
struct SYS_CAN_DATA can_monitor_buf[CAN_BUF_LEN];

struct CAN_DATA can_tx;
struct CAN_DATA can_rx;

void flag_monitor_init()
{
  flag_monitor_all = false;
  flag_monitor_ss = false;
}

//-------------------------------------------------------
void setup() {
  flag_monitor_init();
  buf_clear();
  Serial.begin(UART_SPEED);
  flag_can_open = packing_can_open();
  if (!flag_can_open) {
    Serial.println("???-> can't open can dev");
  }
  output_version();
#if ESP32_BOARD
  thread_init();
#endif
}

void output_version()
{
  if (flag_can_open) {
    Serial.println("------");
    Serial.println(key_version + String(BOARD_NAME));
  }
}

//-------------------------------------------------------
unsigned long time_tag_cur = 0;
unsigned long time_tag_monitor = 0;
unsigned long time_tag_100ms = 0;
unsigned long time_tag_1s = 0;
void loop() {
  time_tag_cur = millis();
#if ARDUINO_BOARD
  can_event(time_tag_cur);
  analyze_com_buf();
#endif
#if ESP32_BOARD
  Serial.event();
  analyze_com_buf();
#endif
  //50ms
  if (time_tag_cur - time_tag_monitor >= 50) {
    time_tag_monitor = time_tag_cur;
    update_monitor_ss();
  }
  //100ms
  if (time_tag_cur - time_tag_100ms >= 100) {
    time_tag_100ms = time_tag_cur;
  }
  //1s
  if (time_tag_cur - time_tag_1s >= 1000) {
    time_tag_1s = time_tag_cur;
  }
}


//-------------------------------------------------------
void com_buf_add_one(char b)
{
    if (com_index < COM_BUF_LEN) {
      com_buf[com_index+1] = 0;
      com_buf[com_index] = b;
      com_index ++;
    } else {
      Serial.println("???-> com buf overflow");
    }
}

void com_buf_del_one()
{
  int i;
  if (com_index > 0) {
    for (i=0;i<com_index;i++) {
      com_buf[i] = com_buf[i+1];
    }
    com_index --;
  }
}


//-------------------------------------------------------
#define CMD_MAX_LEN 80
#define CMD_HEAD_TAG "can"
#define CMD_TIAL_TAG '&'
char tmp_cmd_str[CMD_MAX_LEN];
bool analyze_for_can()
{
  if (is_str_same(com_buf,"???")) {
    output_version();
    com_buf_del_one();
    com_buf_del_one();
    return false;
  }

  if (is_str_same(com_buf,CMD_HEAD_TAG)) {
    if (com_index > CMD_MAX_LEN) {
      return false;
    }
    
    int index = index_of_char(com_buf,CMD_TIAL_TAG);
    if (index < 0) {
      return true;
    }
    com_buf[index] = 0;
    //Serial.println(com_buf);

    char *lp_cmd = &com_buf[4];
    if (is_str_same(lp_cmd,"msg")) {
      int ret = buf_to_can_data(lp_cmd + 4,&can_tx,FLAG_MSG_FULL);
      if (ret >= 0) {
        //Serial.println("can_tx.id " + String(can_tx.id,HEX));
        //Serial.println("can_tx.len " + String(can_tx.len,HEX));
        //Serial.println("can_tx.tm " + String(can_tx.tm,DEC));
        //for (int i=0;i<can_tx.len;i++)
        //  Serial.println("can_tx.data " + String(i,HEX) + " " + String(can_tx.buf[i],HEX));
        output_can_tx_info(&can_tx);
        can_tx.tm /= 10;
        can_tx.tm *= 10;
        add_can_tx_buf(&can_tx);
      } else {
        Serial.println("err-> tx msg " + String(ret,DEC));
      }
    }

    if (is_str_same(lp_cmd,"del tx all")) {
      //Serial.println("clear tx buf");
      clr_can_tx_buf();
    }
    else if (is_str_same(lp_cmd,"del tx")) {
      int id = buf_get_can_id(&lp_cmd[7]);
      //Serial.println("del tx id = " + String(id,HEX));
      del_can_tx_buf(id);
    }

    if (is_str_same(lp_cmd,"monitor all")) {
      flag_monitor_init();
      flag_monitor_all = true;
      clear_monitor_buf();
    }
    else if (is_str_same(lp_cmd,"unmonitor all")) {
      flag_monitor_init();
      clear_monitor_buf();
    }
    else if (is_str_same(lp_cmd,"monitor ss")) {
      flag_monitor_init();
      flag_monitor_ss = true;
      clear_monitor_buf();
    }
    else if (is_str_same(lp_cmd,"monitor")) {
      int id = buf_get_can_id(&lp_cmd[8]);
      //Serial.println("monitor id = " + String(id,HEX));
      add_monitor_buf(id);
      flag_monitor_init();
    }

    if (is_str_same(lp_cmd,"output tx")) {
      Serial.println("--- output tx ---");
      output_all_can_tx();
    }
    if (is_str_same(lp_cmd,"reset")) {
      clr_can_tx_buf();
      flag_monitor_init();
      clear_monitor_buf();
    }
  }
  return false;
}

void analyze_com_buf()
{
  while (Serial.available()) {
    char tmp = Serial.read();
    com_buf_add_one(tmp);
  }
  while(com_index>=3) {
    if (analyze_for_can()) {
      break;
    }
    com_buf_del_one();
  }
}

int buf_get_can_id(char *buf)
{
  unsigned long tmp = hex_buf_to_long(buf);
  return long_to_int(tmp);
}

//-------------------------------------------------------
void buf_clear()
{
  clr_can_tx_buf();
  clear_monitor_buf();
}

void add_tx_msg(struct SYS_CAN_DATA *sys,struct CAN_DATA *can)
{
  can_data_copy(can,&(sys->can));
  sys->can.tm = can->tm;
  sys->tag = millis();
  sys->active = true;
}

void add_can_tx_buf(struct CAN_DATA *can)
{
  struct SYS_CAN_DATA *tmp;
  for (char i=0;i<CAN_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (tmp->active && tmp->can.id == can->id) {
      add_tx_msg(tmp,can);
      return;
    }
  }
  for (char i=0;i<CAN_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (!tmp->active) {
      add_tx_msg(tmp,can);
      return;
    }
  }
  Serial.println("??? can tx buf full");
}

void del_can_tx_buf(int id)
{
  struct SYS_CAN_DATA *tmp;
  for (char i=0;i<CAN_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (tmp->active && tmp->can.id == id) {
      tmp->active = false;
      return;
    }
  }
}

void clr_can_tx_buf()
{
  for (int i=0;i<CAN_BUF_LEN;i++) {
    can_tx_buf[i].active = false;
  }
}

void output_can_tx_info(struct CAN_DATA *can)
{
  can_data_to_buf(tmp_cmd_str,can,FLAG_MSG_FULL);
  Serial.println("can_tx " + String(tmp_cmd_str));
}

void output_all_can_tx()
{
  for (int i=0;i<CAN_BUF_LEN;i++) {
    if (can_tx_buf[i].active) {
      output_can_tx_info(&(can_tx_buf[i].can));
    }
  }
}

void add_monitor_ss(struct CAN_DATA *can)
{
  int i;
  struct SYS_CAN_DATA *lp;
  int ret = 0;
vTaskSuspendAll();
  for (i=0;i<CAN_BUF_LEN;i++) {
    lp = &can_monitor_buf[i];
    if (lp->active && lp->can.id == can->id) {
      can_data_copy(can,&(lp->can));
      lp->can.tm++;
      ret = 1;
      break;
    }
  }
xTaskResumeAll();
  if (ret > 0) {
    return;
  }

  for (i=0;i<CAN_BUF_LEN;i++) {
    lp = &can_monitor_buf[i];
    if (!lp->active) {
      can_data_copy(can,&(lp->can));
      lp->can.tm = 1;
      lp->active = true;
      return;
    }
  }
}

void add_monitor_buf(int id)
{
  struct SYS_CAN_DATA *lp;
  for (int i=0;i<CAN_BUF_LEN; i++) {
    lp = &can_monitor_buf[i];
    if (lp->active && lp->can.id == id) {
      return;
    }
  }
  for (int i=0;i<CAN_BUF_LEN; i++) {
    lp = &can_monitor_buf[i];
    if (!lp->active) {
      lp->can.id = id;
      lp->active = true;
      return;
    }
  }
}

void clear_monitor_buf()
{
  struct SYS_CAN_DATA *lp;
  for (int i=0;i<CAN_BUF_LEN; i++) {
    lp = &can_monitor_buf[i];
    lp->active = false;
  }
}

void output_can_rx_info(struct CAN_DATA *can)
{
  can_data_to_buf(tmp_cmd_str,can,FLAG_MSG_FULL);
  Serial.println("can_rx " + String(tmp_cmd_str));
}

void output_can_ss_info(struct CAN_DATA *can)
{
  can_data_to_buf(tmp_cmd_str,can,FLAG_MSG_FULL);
  Serial.println("can_ss " + String(tmp_cmd_str));
}

void process_rx_msg(struct CAN_DATA *can)
{
  if (flag_monitor_all) {
    output_can_rx_info(can);
  }
  else if (flag_monitor_ss) {
    add_monitor_ss(can);
  }
  else {
    struct SYS_CAN_DATA *lp;
    for (int i=0;i<CAN_BUF_LEN; i++) {
      lp = &can_monitor_buf[i];
      if (lp->active && lp->can.id == can->id) {
        output_can_rx_info(can);
      }
    }
  }
}

void update_monitor_ss()
{
  static int index = 0;
  struct SYS_CAN_DATA *lp;
vTaskSuspendAll();
  index++;
  if (index >= CAN_BUF_LEN) {
    index = 0;
  }
  lp = &can_monitor_buf[index];
  if (lp->active) {
    output_can_ss_info(&(lp->can));
    lp->active = false;
  }
xTaskResumeAll();
}

//-------------------------------------------------------
inline void can_tx_event(unsigned long tm)
{
  for (int i=0;i<CAN_BUF_LEN;i++) {
    if (can_tx_buf[i].active) {
      if (tm - can_tx_buf[i].tag >= can_tx_buf[i].can.tm) {
        can_tx_buf[i].tag = tm;
        packing_can_send(&can_tx_buf[i].can);
        if (can_tx_buf[i].can.tm < 10) {
          can_tx_buf[i].active = false;
        }
      }
    }
  }
}

inline void can_rx_event(unsigned long tm)
{
  if (tm <= 0) {
    tm = millis();
  }
  while(packing_can_receive(&can_rx)) {
    can_rx.tm = tm;
    process_rx_msg(&can_rx);
  }
}

void can_event(unsigned long tm)
{
  can_tx_event(tm);
  can_rx_event(tm);
}

//-------------------------------------------------
#if ESP32_BOARD
SemaphoreHandle_t xSemaphore_Msg = NULL;
void task_tx_msg(void *arg)
{
  while(true) {
    unsigned long tm = millis();
    can_tx_event(tm);
    vTaskDelay(5/portTICK_RATE_MS);
    //thread_output_msg("task_tx_msg " + String(tm,DEC));
    //vTaskDelay(1000/portTICK_RATE_MS);
  }
}

void task_rx_msg(void *arg)
{
  while(true) {
    //unsigned long tm = millis();
    can_rx_event(0);
    //vTaskDelay(2/portTICK_RATE_MS);
    //thread_output_msg("task_rx_msg " + String(tm,DEC));
    //vTaskDelay(1000/portTICK_RATE_MS);
  }
}

void thread_init()
{
  Serial.println("info-> thread_init");
  xSemaphore_Msg = xSemaphoreCreateMutex();
  xTaskCreate(task_tx_msg, "task_tx_msg", 4096, NULL, 7, NULL);
  xTaskCreate(task_rx_msg, "task_rx_msg", 4096, NULL, 8, NULL);
}

void thread_output_msg(String info)
{
  xSemaphoreTake( xSemaphore_Msg, portMAX_DELAY );
  {
    Serial.println(info);
  }
  xSemaphoreGive( xSemaphore_Msg );
}
#endif
