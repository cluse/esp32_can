
#include <Arduino.h>
#include "def.h"
#include "str.h"

//-------------------------------------------------------
#define ARDUINO_BOARD
//#define ESP32_BOARD

#define UART_SPEED 115200
const String key_version = "CAN_VER 1.1";

#ifdef ARDUINO_BOARD
#include "can_arduino.h"
#define packing_can_open    can_init
#define packing_can_close   
#define packing_can_send    can_send
#define packing_can_receive can_get
#endif

#if 0//def ESP32_BOARD
#include "esp_pthread.h"
#include "unistd.h"
#include "can_esp32.h"
#define packing_can_open    class_can_open
#define packing_can_close   class_can_close
#define packing_can_send    class_can_send_msg
#define packing_can_receive class_can_read_msg
#endif


//-------------------------------------------------------
bool flag_can_open;
bool flag_monitor_all;

#define COM_BUF_LEN 200
char com_index=0;
char com_buf[COM_BUF_LEN + 1];

#define CAN_BUF_LEN 10
struct SYS_CAN_DATA can_tx_buf[CAN_BUF_LEN];
struct SYS_CAN_DATA can_monitor_buf[CAN_BUF_LEN];

struct CAN_DATA can_tx;
struct CAN_DATA can_rx;


//-------------------------------------------------------
void setup() {
  flag_monitor_all = false;
  buf_clear();
  Serial.begin(UART_SPEED);
  flag_can_open = packing_can_open();
  if (!flag_can_open) {
    Serial.println("???-> can't open can dev");
  }
  output_version();
#ifdef ESP32_BOARD
  thread_init();
#endif
}

void output_version()
{
  if (flag_can_open) {
    Serial.println("------");
    Serial.println(key_version);
  }
}

//-------------------------------------------------------
unsigned long time_tag_cur = 0;
unsigned long time_tag_100ms = 0;
unsigned long time_tag_1s = 0;
void loop() {
  time_tag_cur = millis();
#ifdef ARDUINO_BOARD
  can_event(time_tag_cur);
#endif
  analyze_com_buf();
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
      com_buf[com_index + 1] = 0;
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
    //Serial.println(com_buf);

    char *lp_cmd = &com_buf[4];
    if (is_str_same(lp_cmd,"msg")) {
      int ret = buf_to_can_data(lp_cmd,&can_tx);
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
      flag_monitor_all = true;
      clear_monitor_buf();
    }
    else if (is_str_same(lp_cmd,"unmonitor all")) {
      flag_monitor_all = false;
      clear_monitor_buf();
    }
    else if (is_str_same(lp_cmd,"monitor")) {
      int id = buf_get_can_id(&lp_cmd[8]);
      //Serial.println("monitor id = " + String(id,HEX));
      add_monitor_buf(id);
      flag_monitor_all = false;
    }

    if (is_str_same(lp_cmd,"output tx")) {
      Serial.println("--- output tx ---");
      output_all_can_tx();
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
  can_data_to_buf(tmp_cmd_str,can);
  Serial.println("can_tx" + String(tmp_cmd_str));
}

void output_all_can_tx()
{
  for (int i=0;i<CAN_BUF_LEN;i++) {
    if (can_tx_buf[i].active) {
      output_can_tx_info(&(can_tx_buf[i].can));
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
  can_data_to_buf(tmp_cmd_str,can);
  Serial.println("can_rx" + String(tmp_cmd_str));
}

void process_rx_msg(struct CAN_DATA *can)
{
    if (is_msg_in_monitor(can->id)) {
      output_can_rx_info(can);
    }
}

bool is_msg_in_monitor(int id)
{
  if (flag_monitor_all) {
    return true;
  }
  struct SYS_CAN_DATA *lp;
  for (int i=0;i<CAN_BUF_LEN; i++) {
    lp = &can_monitor_buf[i];
    if (lp->active) {
      if (lp->can.id == id) {
        return true;
      }
    }
  }
  return false;
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
#ifdef ESP32_BOARD
void sleep_ms(unsigned long ms)
{
  usleep(ms*1000);
}

//typedef void (*thread_fun)(void *);
pthread_t lp_tx_msg;
pthread_t lp_rx_msg;
void *thread_tx_msg(void *p)
{
  while(true) {
    unsigned long tm = millis();
    can_tx_event(tm);
    sleep_ms(2);
  }
}

void *thread_rx_msg(void *p)
{
  while(true) {
    unsigned long tm = millis();
    can_rx_event(tm);
    sleep_ms(2);
    Serial.println("info-> thread_rx_msg");
  }
}

void thread_init()
{
  Serial.println("info-> thread_init");
  //esp_pthread_cfg_t cfg = esp_create_default_pthread_config();
  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (4 * 1024);
  cfg.inherit_cfg = true;
  esp_pthread_set_cfg(&cfg);

  int ret;
  ret = pthread_create(&lp_tx_msg, NULL, thread_tx_msg, NULL);
  if (ret) {
    Serial.println("err-> thread_tx_msg");
  }
  ret = pthread_create(&lp_rx_msg, NULL, thread_rx_msg, NULL);
  if (ret) {
    Serial.println("err-> thread_rx_msg");
  }
}
#endif
