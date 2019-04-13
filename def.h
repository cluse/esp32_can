

//---------------------------
struct CAN_DATA {
  int id;
  char len;
  char buf[8];
  unsigned long tm;
};

struct SYS_CAN_DATA
{
  bool active;
  unsigned long tag;
  struct CAN_DATA can;
};


//---------------------------
#define CAN_FRAME_DATA_MAX 8
//can msg id=32 len=5 data=12,34,5,6,a,8 tm=12345 &
//can msg id=8832 len=3 data=12,34,5,6,a,8 tm=1234567 &
//can msg id=8832 len=8 data=12,34,5,6,a,8 tm=1234567 &
//can msg id=8832 len=8 data=12,34,56,6a,af,8a,ff,ee,dd tm=1234567 &
