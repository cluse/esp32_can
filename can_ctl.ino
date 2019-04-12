
#include <Arduino.h>
#include "can.h"
#include "str.h"

#define UART_SPEED 115200

const String key_version = "CAN_VER 1.0";


//-------------------------------------------------------
bool flag_can_open;
bool flag_monitor_msg;

#define COM_BUF_LEN 200
char com_index=0;
char com_buf[COM_BUF_LEN];

struct SYS_CAN_DATA
{
  bool active;
  struct CAN_DATA can;
  int cycle;
  unsigned long tm;
};
#define CAN_DATA_BUF_LEN 10
struct SYS_CAN_DATA can_tx_buf[CAN_DATA_BUF_LEN];

struct SYS_CAN_FILTER
{
  bool active;
  int id;
};
#define CAN_FILTER_BUF_LEN 10
struct SYS_CAN_FILTER can_filter_buf[CAN_FILTER_BUF_LEN];


//-------------------------------------------------------
void setup() {
  Serial.begin(UART_SPEED);
  flag_monitor_msg = false;
  flag_can_open = can_init();
  if (!flag_can_open) {
    Serial.println("???-> can't open can dev");
  }
  output_version();
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
  can_event(time_tag_cur);
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

void serialEvent()
{
  while (Serial.available()) {
    char tmp = Serial.read();
    com_buf_add_one(tmp);
  }
}


//-------------------------------------------------------
void com_buf_add_one(char b)
{
    if (com_index < COM_BUF_LEN) {
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
void sys_can_copy(struct SYS_CAN_DATA *src,struct SYS_CAN_DATA *dst)
{
  dst->cycle = src->cycle;
  dst->tm = src->tm;
  dst->can.id = src->can.id;
  dst->can.len = src->can.len;
  for (char i=0;i<8;i++) {
    dst->can.buf[i] = src->can.buf[i];
  }
}

// id(4) + cyc(4) + data(2n)
int buf_get_can_id(char * buf)
{
  int id;
  id = char_to_int(ascii_to_hex(&buf[0],2));
  id <<= 8;
  id |= char_to_int(ascii_to_hex(&buf[2],2));
  return id;
}

int buf_get_can_cyc(char * buf)
{
  return ascii_to_dec_int(&buf[5],4);
}

void buf_get_can_data(char *buf,struct CAN_DATA *can)
{
  char len = 0;
  for (char i=0;i<8;i++) {
    if (!is_hex_char(buf[10 + 2*i]))
      break;
    else
      len++;
  }
  can->id = buf_get_can_id(buf);
  can->len = len;
  for (char i=0;i<len;i++) {
    can->buf[i] = ascii_to_hex(&buf[10 + 2*i],2);
  }
}

struct SYS_CAN_DATA can_tx;
void get_can_cmd_data(char *buf)
{
  struct SYS_CAN_DATA *tmp = &can_tx;
  buf_get_can_data(buf,&(tmp->can));
  tmp->tm = millis();
  int cycle = buf_get_can_cyc(buf);
  cycle /= 10;
  cycle *= 10;
  tmp->cycle = cycle;
  output_can_tx_info(tmp);
  add_can_tx_buf(tmp);
}

#define CMD_MAX_LEN 50
#define CMD_HEAD_TAG "can"
#define CMD_TIAL_TAG '&'
bool analyze_for_can()
{
  if (is_str_same(com_buf,"???")) {
    output_version();
    return false;
  }

  if (is_str_same(com_buf,CMD_HEAD_TAG)) {
    if (com_index > CMD_MAX_LEN) {
      return false;
    }
    
    int index = index_of_char(com_buf,CMD_TIAL_TAG,com_index);
    if (index < 0) {
      return true;
    }

    char *lp_cmd = &com_buf[4];
    if (is_str_same(lp_cmd,"msg")) {
      get_can_cmd_data(&com_buf[8]);
    }

    if (is_str_same(lp_cmd,"del tx all")) {
      //Serial.println("clear tx buf");
      clr_can_tx_buf();
    }
    else if (is_str_same(lp_cmd,"del tx")) {
      int id = buf_get_can_id(&com_buf[11]);
      //Serial.println("clear tx id = " + String(id,HEX));
      del_can_tx_buf(id);
    }

    if (is_str_same(lp_cmd,"filter")) {
      int id = buf_get_can_id(&com_buf[11]);
      add_filter_buf(id);
    }
    else if (is_str_same(lp_cmd,"unfilter all")) {
      clear_filter_buf();
    }

    if (is_str_same(lp_cmd,"monitor all")) {
      flag_monitor_msg = true;
    }
    else if (is_str_same(lp_cmd,"unmonitor all")) {
      flag_monitor_msg = false;
    }

    if (is_str_same(lp_cmd,"get tx msg")) {
      //Serial.println("output tx buf");
      output_all_can_tx();
    }
  }
  return false;
}

void analyze_com_buf()
{
  while(com_index>=3) {
    if (analyze_for_can()) {
      break;
    }
    com_buf_del_one();
  }
}


//-------------------------------------------------------
void add_can_tx_buf(struct SYS_CAN_DATA *can)
{
  struct SYS_CAN_DATA *tmp;
  for (char i=0;i<CAN_DATA_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (tmp->active && tmp->can.id == can->can.id) {
      sys_can_copy(can,tmp);
      return;
    }
  }
  for (char i=0;i<CAN_DATA_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (!tmp->active) {
       tmp->active = true;
       sys_can_copy(can,tmp);
       return;
    }
  }
  Serial.println("??? can tx buf full");
}

void del_can_tx_buf(int id)
{
  struct SYS_CAN_DATA *tmp;
  for (char i=0;i<CAN_DATA_BUF_LEN;i++) {
    tmp = &can_tx_buf[i];
    if (tmp->active && tmp->can.id == id) {
      tmp->active = false;
      return;
    }
  }
}

void clr_can_tx_buf()
{
  for (int i=0;i<CAN_DATA_BUF_LEN;i++) {
    can_tx_buf[i].active = false;
  }
}

void output_can_tx_info(struct SYS_CAN_DATA *can)
{
  String tmp = "can_tx";
  tmp += " id=" + String(can->can.id,HEX);
  tmp += " len=" + String(can->can.len,DEC);
  tmp += " data=";
  for (int i=0;i<can->can.len;i++) {
    tmp += String(can->can.buf[i],HEX);
    tmp += ",";
  }
  tmp += " cycle=" + String(can->cycle,DEC);
  Serial.println(tmp);
}

void output_all_can_tx()
{
  for (int i=0;i<CAN_DATA_BUF_LEN;i++) {
    if (can_tx_buf[i].active) {
      output_can_rx_info(&can_tx_buf[i]);
    }
  }
}

void add_filter_buf(int id)
{
  struct SYS_CAN_FILTER *lp;
  for (int i=0;i<CAN_FILTER_BUF_LEN; i++) {
    lp = &can_filter_buf[i];
    if (!lp->active) {
      lp->id = id;
      lp->active = true;
    }
  }
}

void clear_filter_buf()
{
  struct SYS_CAN_FILTER *lp;
  for (int i=0;i<CAN_FILTER_BUF_LEN; i++) {
    lp = &can_filter_buf[i];
    lp->active = false;
  }
}

void output_can_rx_info(struct SYS_CAN_DATA *can)
{
  String tmp = "can_rx";
  tmp += " id=" + String(can->can.id,HEX);
  tmp += " data=";
  for (int i=0;i<can->can.len;i++) {
    tmp += String(can->can.buf[i],HEX);
    tmp += ",";
  }
  tmp += " tm=" + String(can->tm + 1,DEC);
  Serial.println(tmp);
}

void process_rx_msg(struct SYS_CAN_DATA *can)
{
  if (flag_monitor_msg) {
    if (is_filter_msg(can->can.id)) {
      output_can_rx_info(can);
    }
  }
}

bool is_filter_msg(int id)
{
  bool ret = true;
  struct SYS_CAN_FILTER *lp;
  for (int i=0;i<CAN_FILTER_BUF_LEN; i++) {
    lp = &can_filter_buf[i];
    if (lp->active) {
      ret = false;
      if (lp->id == id)
        return true;
    }
  }
  return ret;
}


//-------------------------------------------------------
inline void can_tx_event(unsigned long tm)
{
  for (int i=0;i<CAN_DATA_BUF_LEN;i++) {
    if (can_tx_buf[i].active) {
      if (tm - can_tx_buf[i].tm >= can_tx_buf[i].cycle) {
        can_tx_buf[i].tm = tm;
        can_send(&can_tx_buf[i].can);

        if (can_tx_buf[i].cycle < 10) {
          can_tx_buf[i].active = false;
        }
      }
    }
  }
}

struct SYS_CAN_DATA can_rx;
inline void can_rx_event(unsigned long tm)
{
  while(can_get(&(can_rx.can))) {
    can_rx.tm = tm;
    can_rx.cycle = 0;
    process_rx_msg(&can_rx);
  }
}

void can_event(unsigned long tm)
{
  can_tx_event(tm);
  can_rx_event(tm);
}
