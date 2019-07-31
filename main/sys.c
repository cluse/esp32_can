

#include <stdio.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "driver/uart.h"
//#include "driver/gpio.h"
#include "def.h"
#include "sys.h"
#include "lib_str.h"
#include "can_list.h"
#include "lib_can.h"
#include "lib_uart.h"
#include "lib_dac.h"

#define COM_BUF_LEN 300
static char com_buf[COM_BUF_LEN+1];
static int com_in;

#define CMD_MAX_LEN 80

static void com_process();
static void output_can_tx(struct CAN_DATA *can);
static void output_can_tx_all();
static void output_can_rx_all();
static int buf_get_can_id(char *buf);
static int buf_get_dec_val(char *buf);


//-------------------------------------
static void com_buf_del(int num)
{
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

static bool flag_monitor_all;
static bool flag_monitor_ss;
static bool flag_monitor_id;
__inline static void flag_monitor_init()
{
    flag_monitor_all = false;
    flag_monitor_ss = false;
    flag_monitor_id = false;
    SysList_CoverSet(false);
}

__inline static void can_monitor_ss_set()
{
    SysList_CoverSet(true);
    flag_monitor_ss = true;
}

static int can_monitor_id0;
__inline static void can_monitor_id_set(int id)
{
    can_monitor_id0 = id;
    flag_monitor_id = true;
}

static bool Is_In_Can_Monitor(int id)
{
    if (flag_monitor_all) {
        return true;
    }
    if (flag_monitor_id) {
        if (can_monitor_id0 == id)
            return true;
    }
    return false;
}


//-------------------------------------
void Sys_Init()
{
    com_in = 0;
    fflush(stdin);
    flag_monitor_init();
    
    if (!esp32_dac_enable(1)) {
        sys_print("err-> esp32_dac_enable(1)");
    }
    if (!esp32_dac_set(1,0)) {
        sys_print("err-> esp32_dac_set(1,0)");
    }
    
    if (!esp32_dac_enable(2)) {
        sys_print("err-> esp32_dac_enable(2)");
    }
    if (!esp32_dac_set(2,0)) {
        sys_print("err-> esp32_dac_set(2,0)");
    }
}

void com_event()
{
    bool f_del = false;
    int index;
    int len,ret;
    len = sys_uart_rx_len();
    if (len > COM_BUF_LEN - com_in) {
        len = COM_BUF_LEN - com_in;
    }
    if (len > 0) {
        ret = sys_uart_rx_bytes(&com_buf[com_in],len);
        if (ret > 0)
            com_in += ret;
    }
    com_buf[com_in] = 0;
    index = index_of_str(com_buf,"???");
    if (index >= 0)
    {
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
            f_del = true;
        }
    }
    else {
        index = index_of_char(com_buf,'&');
        if (index >= 0) {
          com_buf_del(index+1);
        }
        else {
          f_del = true;
        }
    }
    
    if (f_del) {
        if (com_in > CMD_MAX_LEN) {
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
        sys_print_code("err-> tx msg ",ret);
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
      SysList_RxInit();
      flag_monitor_all = true;
    }
    else if (is_str_same(lp_cmd,"unmonitor all")) {
      flag_monitor_init();
      SysList_RxInit();
    }
    else if (is_str_same(lp_cmd,"monitor ss")) {
      flag_monitor_init();
      SysList_RxInit();
      can_monitor_ss_set();
    }
    else if (is_str_same(lp_cmd,"monitor")) {
      can.id = buf_get_can_id(&lp_cmd[8]);
      //Serial.println("monitor id = " + String(id,HEX));
      flag_monitor_init();
      SysList_RxInit();
      can_monitor_id_set(can.id);
    }
    
    if (is_str_same(lp_cmd,"dac")) {
        int chan = buf_get_dec_val(&lp_cmd[3]);
        int val = buf_get_dec_val(&lp_cmd[5]);
        //sys_print_code("dac chan = ",chan);
        //sys_print_code("dac val = ",val);
        esp32_dac_set(chan,val);
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


//-------------------------------------
__inline void output_can_info(char *head,struct CAN_DATA *can)
{
    char tmp_str[CMD_MAX_LEN];
    can_data_to_buf(tmp_str,can);
    //printf("%s %s \n",head,tmp_str);
    sys_print2(head,tmp_str);
}

static void output_can_tx(struct CAN_DATA *can)
{
    output_can_info("can_tx",can);
}

static void output_can_tx_all()
{
    struct CAN_DATA can;
    sys_print("--- output tx list---");
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
    sys_print("--- output rx list---");
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

static int buf_get_dec_val(char *buf)
{
    unsigned long tmp = dec_buf_to_long(buf);
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
                if (esp32_can_tx_msg(&can)) {
                    SysList_TxDel(i);
                    //sys_print_code("real->tx id ",can.id);
                }
                else {
                    sys_print("err-> esp32_can_tx_msg");
                }
            }
            else if (ABS(tm,tag) > can.tm) {
                esp32_can_tx_msg(&can);
            }
        }
    }
}

void rx_process(struct CAN_DATA *pCan,long tm)
{
    pCan->tm = tm;
    if (flag_monitor_ss) {
        SysList_RxAdd(pCan);
    }
    else if (Is_In_Can_Monitor(pCan->id)) {
        output_can_rx(pCan);
    }
}

static int ss_index = 0;
static long ss_tm = 0;
void monitor_ss_process(long tm)
{
    struct CAN_DATA can;
    int num;
    if (!flag_monitor_ss) {
        return;
    }
    if (ABS(ss_tm,tm) < 100) {
        return;
    }
    ss_tm = tm;

    if (SysList_RxIsActive(ss_index)) {
        num = SysList_RxRead(ss_index,&can);
        SysList_RxDel(ss_index);
        can.tm = num;
        output_can_monitor(&can,num);
    }
    ss_index++;
    if (ss_index >= CAN_LIST_LEN) {
        ss_index = 0;
    }
}



