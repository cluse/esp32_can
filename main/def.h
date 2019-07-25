
#ifndef _DEF_H_ 
#define _DEF_H_

#define VERSION "CAN_VER V2.3 ESP32"

#define UART_SPEED 115200

#include <stdbool.h>

#define ABS(a,b) (a>b?a-b:b-a)


//---------------------------
#define CAN_LIST_LEN 20
#define CAN_DATA_MAX_LEN 8

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



#endif
