

#include "def.h"
#include "lib_str.h"


//-------------------------------------------------
static bool flag_cover = false;
void SysList_CoverSet(bool cover)
{
    flag_cover = cover;
}

__inline void SysList_Init(struct SYS_CAN *list)
{
    for (int i=0;i<CAN_LIST_LEN;i++) {
        list[i].active = false;
    }
}

static bool SysList_Add(struct SYS_CAN *list,struct CAN_DATA *can)
{
    struct SYS_CAN *lp;
    if (flag_cover) {
        for (int i=0;i<CAN_LIST_LEN;i++) {
            lp = &(list[i]);
            if (lp->active && lp->can.id == can->id) {
                can_data_copy(can,&(lp->can));
                lp->num++;
                return true;
            }
        }
    }
    for (int i=0;i<CAN_LIST_LEN;i++) {
        lp = &(list[i]);
        if (!lp->active) {
            can_data_copy(can,&(lp->can));
            lp->num = 1;
            lp->tag = 0;
            lp->active = true;
            return true;
        }
    }
    return false;
}

__inline void SysList_ReadCan(struct SYS_CAN *list,int index,struct CAN_DATA *can)
{
    can_data_copy(&(list[index].can),can);
}

__inline void SysList_Del(struct SYS_CAN *list,int index)
{
    list[index].active = false;
}

__inline void SysList_Del_Id(struct SYS_CAN *list,int id)
{
    int i;
    struct SYS_CAN *lp;
    for (i=0;i<CAN_LIST_LEN;i++) {
        lp = &list[i];
        if (lp->can.id == id)
            lp->active = false;
    }
}


//-------------------------------------------------
static struct SYS_CAN sys_list_tx[CAN_LIST_LEN];
static struct SYS_CAN sys_list_rx[CAN_LIST_LEN];

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



