
#ifndef _DEF_H_ 
#define _DEF_H_

#define VERSION "CAN_VER V2.7 ESP32"

#define UART_SPEED 115200

#include <stdbool.h>

#define ABS(a,b) (a>b?a-b:b-a)


//---------------------------
#define CAN_LIST_LEN 20
#define CAN_DATA_MAX_LEN 8

struct CAN_DATA {
  bool active;
  int num;
  long tag;
  
  int id;
  char len;
  char buf[CAN_DATA_MAX_LEN];
  long tm;
};


#endif
