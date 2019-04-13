

#define SPI_HAS_TRANSACTION

#include <mcp_can.h>
#include <SPI.h>

const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN);

bool can_init()
{
  int i;
  int num = 3;
  for (i=0;i<num;i++) {
    if (CAN_OK == CAN.begin(CAN_1000KBPS))
      break;
  }
  return (i<num)?true:false;
}

void can_send(struct CAN_DATA *can)
{
  CAN.sendMsgBuf(can->id, 0, can->len, can->buf);
}

bool can_get(struct CAN_DATA *can)
{
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
     CAN.readMsgBuf(&(can->len), can->buf);
     can->id = CAN.getCanId();
     return true;
  }
  return false;
}
