

#include "def.h"
//#include "lib_str.h"


//-------------------------------------------------
static bool flag_cover = false;
void SysList_CoverSet(bool cover)
{
    flag_cover = cover;
}

__inline void SysList_Init(struct CAN_DATA *list)
{
    for (int i=0;i<CAN_LIST_LEN;i++) {
        (list+i)->active = false;
    }
}

__inline void can_copy(struct CAN_DATA *src,struct CAN_DATA *dst)
{
    int len;
    char *lpcs,*lpcd;
    dst->id = src->id;
    dst->len = src->len;
    len = src->len;
    if (len > 0 && len <=  CAN_DATA_MAX_LEN) {
        lpcs = src->buf;
        lpcd = dst->buf;
        for (int i=0;i<len;i++) {
            *lpcd++ = *lpcs++;
        }
    }
}

static bool SysList_Add(struct CAN_DATA *list,struct CAN_DATA *can)
{
    struct CAN_DATA *lp;
    if (flag_cover) {
        for (int i=0;i<CAN_LIST_LEN;i++) {
            lp = (list+i);
            if (lp->active && lp->id == can->id) {
                can_copy(can,lp);
                lp->num++;
                return true;
            }
        }
    }
    for (int i=0;i<CAN_LIST_LEN;i++) {
        lp = (list+i);
        if (!lp->active) {
            can_copy(can,lp);
            lp->num = 1;
            lp->tag = 0;
            lp->active = true;
            return true;
        }
    }
    return false;
}

__inline void SysList_ReadCan(struct CAN_DATA *list,int index,struct CAN_DATA *can)
{
    can_copy((list+index),can);
}

__inline void SysList_Del(struct CAN_DATA *list,int index)
{
    (list+index)->active = false;
}

__inline void SysList_Del_Id(struct CAN_DATA *list,int id)
{
    for (int i=0;i<CAN_LIST_LEN;i++) {
        if ((list+i)->id == id)
            (list+i)->active = false;
    }
}


//-------------------------------------------------
static struct CAN_DATA sys_list_tx[CAN_LIST_LEN];
static struct CAN_DATA sys_list_rx[CAN_LIST_LEN];

void SysList_TxInit()
{
    SysList_Init(sys_list_tx);
}

void SysList_RxInit()
{
    SysList_Init(sys_list_rx);
}

bool SysList_TxAdd(struct CAN_DATA *can)
{
    return SysList_Add(sys_list_tx,can);
}

bool SysList_RxAdd(struct CAN_DATA *can)
{
    return SysList_Add(sys_list_rx,can);
}

bool SysList_TxIsActive(int index)
{
    return sys_list_tx[index].active;
}

bool SysList_RxIsActive(int index)
{
    return sys_list_rx[index].active;
}

long SysList_TxRead(int index,struct CAN_DATA *can)
{
    SysList_ReadCan(sys_list_tx,index,can);
    return sys_list_tx[index].tag;
}

void SysList_TxUpdateTag(int index,long tm)
{
    sys_list_tx[index].tag = tm;
}

int SysList_RxRead(int index,struct CAN_DATA *can)
{
    SysList_ReadCan(sys_list_rx,index,can);
    return sys_list_rx[index].num;
}

void SysList_TxDel(int index)
{
    SysList_Del(sys_list_tx,index);
}

void SysList_RxDel(int index)
{
    SysList_Del(sys_list_rx,index);
}

void SysList_TxDel_Id(int id)
{
    SysList_Del_Id(sys_list_tx,id);
}

void SysList_RxDel_Id(int id)
{
    SysList_Del_Id(sys_list_rx,id);
}



