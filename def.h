
#ifndef _DEF_H_ 
#define _DEF_H_

#define VERSION "CAN_VER V1.1 ESP32"

#define UART_SPEED 115200

#include <stdbool.h>


//---------------------------
#define CAN_DATA_MAX_LEN 8
//can msg id=32 len=5 data=12,34,5,6,a,8 tm=12345 &
//can msg id=8832 len=3 data=12,34,5,6,a,8 tm=1234567 &
//can msg id=8832 len=8 data=12,34,5,6,a,8 tm=1234567 &
//can msg id=8832 len=8 data=12,34,56,6a,af,8a,ff,ee,dd tm=1234567 &

struct CAN_DATA {
  int id;
  char len;
  char buf[CAN_DATA_MAX_LEN];
  long tm;
};

struct SYS_CAN
{
  bool active;
  int num;
  long tag;
  struct CAN_DATA can;
};

//---------------------------
#define CAN_LIST_LEN 20


//---------------------------
#define ABS(a,b) (a>b?a-b:b-a)

#endif
